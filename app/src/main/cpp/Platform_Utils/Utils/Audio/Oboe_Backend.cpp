#include "Oboe_Backend.h"
#include <oboe/Oboe.h>
#include <android/log.h>
#include <atomic>
#include <memory>
#include <cstring>

#define LOG_TAG "ArenaX3_Oboe"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace com {
namespace arenax3 {

class OboeBackendImpl : public oboe::AudioStreamCallback {
public:
    OboeBackendImpl()
        : mStream(nullptr),
          mSampleRate(48000),
          mChannelCount(2),
          mFramesPerCallback(0),
          mDeviceId(oboe::kUnspecified),
          mAudioApi(oboe::AudioApi::Unspecified),
          mFormat(oboe::AudioFormat::Float),
          mStreamState(StreamState::Uninitialized),
          mLatencyTuner(nullptr),
          mIsLatencyTunerEnabled(false),
          mBufferSizeInFrames(0),
          mInputPreset(oboe::InputPreset::VoicePerformance) {
    }

    ~OboeBackendImpl() {
        closeStream();
    }

    oboe::Result openStream(int32_t sampleRate,
                            int32_t channelCount,
                            int32_t framesPerCallback,
                            int32_t deviceId,
                            int32_t audioApi) {
        std::lock_guard<std::mutex> lock(mStreamLock);

        if (mStreamState == StreamState::Started ||
            mStreamState == StreamState::Paused) {
            closeStream();
        }

        mSampleRate = sampleRate > 0 ? sampleRate : 48000;
        mChannelCount = channelCount > 0 ? channelCount : 2;
        mFramesPerCallback = framesPerCallback;
        mDeviceId = deviceId;
        mAudioApi = static_cast<oboe::AudioApi>(audioApi);

        oboe::AudioStreamBuilder builder;
        builder.setDirection(oboe::Direction::Output)
               ->setSampleRate(mSampleRate)
               ->setChannelCount(mChannelCount)
               ->setFormat(mFormat)
               ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
               ->setSharingMode(oboe::SharingMode::Exclusive)
               ->setCallback(this);

        if (mFramesPerCallback > 0) {
            builder.setFramesPerCallback(mFramesPerCallback);
        }

        if (mDeviceId != oboe::kUnspecified) {
            builder.setDeviceId(mDeviceId);
        }

        if (mAudioApi != oboe::AudioApi::Unspecified) {
            builder.setAudioApi(mAudioApi);
        }

        oboe::Result result = builder.openStream(mStream);
        if (result != oboe::Result::OK) {
            LOGE("Failed to open Oboe stream. Error: %s", oboe::convertToText(result));
            mStreamState = StreamState::Error;
            return result;
        }

        mBufferSizeInFrames = mStream->getBufferSizeInFrames();
        mSampleRate = mStream->getSampleRate();
        mChannelCount = mStream->getChannelCount();
        mStreamState = StreamState::Initialized;

        LOGI("Oboe stream opened: sampleRate=%d, channels=%d, bufferSize=%d, API=%s",
             mSampleRate, mChannelCount, mBufferSizeInFrames,
             oboe::convertToText(mStream->getAudioApi()));

        return oboe::Result::OK;
    }

    oboe::Result closeStream() {
        std::lock_guard<std::mutex> lock(mStreamLock);

        if (mStream) {
            stopStream();
            oboe::Result result = mStream->close();
            mStream.reset();
            mStreamState = StreamState::Closed;
            LOGI("Oboe stream closed");
            return result;
        }
        mStreamState = StreamState::Uninitialized;
        return oboe::Result::OK;
    }

    oboe::Result startStream() {
        std::lock_guard<std::mutex> lock(mStreamLock);

        if (!mStream) {
            LOGE("Cannot start stream: stream is null");
            return oboe::Result::ErrorNull;
        }

        if (mStreamState == StreamState::Started) {
            LOGI("Stream already started");
            return oboe::Result::OK;
        }

        oboe::StreamState state = mStream->getState();
        if (state == oboe::StreamState::Starting || state == oboe::StreamState::Started) {
            mStreamState = StreamState::Started;
            return oboe::Result::OK;
        }

        oboe::Result result = mStream->requestStart();
        if (result != oboe::Result::OK) {
            LOGE("Failed to start stream. Error: %s", oboe::convertToText(result));
            mStreamState = StreamState::Error;
            return result;
        }

        mStreamState = StreamState::Started;

        if (mIsLatencyTunerEnabled && mLatencyTuner) {
            mLatencyTuner->reset();
        }

        LOGI("Oboe stream started");
        return oboe::Result::OK;
    }

    oboe::Result stopStream() {
        std::lock_guard<std::mutex> lock(mStreamLock);

        if (!mStream) {
            return oboe::Result::OK;
        }

        oboe::StreamState state = mStream->getState();
        if (state == oboe::StreamState::Stopping || state == oboe::StreamState::Stopped || state == oboe::StreamState::Closed) {
            mStreamState = StreamState::Stopped;
            return oboe::Result::OK;
        }

        if (mIsLatencyTunerEnabled && mLatencyTuner) {
            mLatencyTuner->reset();
        }

        oboe::Result result = mStream->requestStop();
        if (result != oboe::Result::OK) {
            LOGE("Failed to stop stream. Error: %s", oboe::convertToText(result));
        }

        mStreamState = StreamState::Stopped;
        LOGI("Oboe stream stopped");
        return result;
    }

    oboe::Result pauseStream() {
        std::lock_guard<std::mutex> lock(mStreamLock);

        if (!mStream) {
            LOGE("Cannot pause stream: stream is null");
            return oboe::Result::ErrorNull;
        }

        oboe::Result result = mStream->requestPause();
        if (result != oboe::Result::OK) {
            LOGE("Failed to pause stream. Error: %s", oboe::convertToText(result));
            return result;
        }

        mStreamState = StreamState::Paused;
        LOGI("Oboe stream paused");
        return oboe::Result::OK;
    }

    oboe::Result flushStream() {
        std::lock_guard<std::mutex> lock(mStreamLock);

        if (!mStream) {
            return oboe::Result::ErrorNull;
        }

        oboe::Result result = mStream->requestFlush();
        if (result != oboe::Result::OK) {
            LOGE("Failed to flush stream. Error: %s", oboe::convertToText(result));
        }
        return result;
    }

    int64_t getFramesWritten() {
        std::lock_guard<std::mutex> lock(mStreamLock);
        if (!mStream) return -1;
        int64_t framesWritten = mStream->getFramesWritten();
        if (framesWritten < 0) {
            oboe::ResultWithValue<int64_t> result = mStream->getTimestamp(CLOCK_MONOTONIC);
            if (result.error() == oboe::Result::OK) {
                framesWritten = result.value().position;
            }
        }
        return framesWritten;
    }

    int64_t getFramesRead() {
        std::lock_guard<std::mutex> lock(mStreamLock);
        if (!mStream) return -1;
        int64_t framesRead = mStream->getFramesRead();
        if (framesRead < 0) {
            oboe::ResultWithValue<int64_t> result = mStream->getTimestamp(CLOCK_MONOTONIC);
            if (result.error() == oboe::Result::OK) {
                framesRead = result.value().position;
            }
        }
        return framesRead;
    }

    double getLatencyMs() {
        std::lock_guard<std::mutex> lock(mStreamLock);
        if (!mStream) return -1.0;
        oboe::ResultWithValue<double> result = mStream->calculateLatencyMillis();
        if (result.error() != oboe::Result::OK) {
            return -1.0;
        }
        return result.value();
    }

    int32_t getSampleRate() const {
        return mSampleRate;
    }

    int32_t getChannelCount() const {
        return mChannelCount;
    }

    int32_t getBufferSizeInFrames() const {
        return mBufferSizeInFrames;
    }

    int32_t getFramesPerCallback() const {
        return mFramesPerCallback;
    }

    int32_t getStreamState() const {
        return static_cast<int32_t>(mStreamState);
    }

    oboe::AudioApi getAudioApi() const {
        if (mStream) {
            return mStream->getAudioApi();
        }
        return oboe::AudioApi::Unspecified;
    }

    void setCallback(std::function<void(float*, int32_t, int32_t, int32_t)> callback) {
        std::lock_guard<std::mutex> lock(mCallbackLock);
        mDataCallback = std::move(callback);
    }

    void setErrorCallback(std::function<void(int32_t, const std::string&)> callback) {
        std::lock_guard<std::mutex> lock(mCallbackLock);
        mErrorCallback = std::move(callback);
    }

    void enableLatencyTuner(bool enable) {
        std::lock_guard<std::mutex> lock(mStreamLock);
        mIsLatencyTunerEnabled = enable;
        if (enable && mStream && mStreamState == StreamState::Started) {
            if (!mLatencyTuner) {
                mLatencyTuner = std::make_unique<oboe::LatencyTuner>(*mStream);
            }
            mLatencyTuner->reset();
        } else if (!enable && mLatencyTuner) {
            mLatencyTuner.reset();
        }
    }

    void tuneLatency() {
        if (mIsLatencyTunerEnabled && mLatencyTuner) {
            oboe::Result result = mLatencyTuner->tune();
            if (result != oboe::Result::OK) {
                LOGD("LatencyTuner tune result: %s", oboe::convertToText(result));
            }
        }
    }

    oboe::Result setBufferSizeInFrames(int32_t sizeFrames) {
        std::lock_guard<std::mutex> lock(mStreamLock);
        if (!mStream) {
            return oboe::Result::ErrorNull;
        }
        oboe::Result result = mStream->setBufferSizeInFrames(sizeFrames);
        if (result == oboe::Result::OK) {
            mBufferSizeInFrames = mStream->getBufferSizeInFrames();
        }
        return result;
    }

    bool isStreamRunning() const {
        return mStreamState == StreamState::Started;
    }

    bool isStreamOpen() const {
        return mStream != nullptr && mStreamState != StreamState::Closed && mStreamState != StreamState::Uninitialized;
    }

    oboe::Result openInputStream(int32_t sampleRate,
                                 int32_t channelCount,
                                 int32_t framesPerCallback,
                                 int32_t deviceId,
                                 int32_t audioApi) {
        std::lock_guard<std::mutex> lock(mStreamLock);

        if (mInputStreamState == StreamState::Started ||
            mInputStreamState == StreamState::Paused) {
            closeInputStream();
        }

        mInputStreamSampleRate = sampleRate > 0 ? sampleRate : 48000;
        mInputStreamChannelCount = channelCount > 0 ? channelCount : 1;
        mInputStreamFramesPerCallback = framesPerCallback;

        oboe::AudioStreamBuilder builder;
        builder.setDirection(oboe::Direction::Input)
               ->setSampleRate(mInputStreamSampleRate)
               ->setChannelCount(mInputStreamChannelCount)
               ->setFormat(oboe::AudioFormat::Float)
               ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
               ->setSharingMode(oboe::SharingMode::Exclusive)
               ->setInputPreset(mInputPreset)
               ->setCallback(this);

        if (framesPerCallback > 0) {
            builder.setFramesPerCallback(framesPerCallback);
        }

        if (deviceId != oboe::kUnspecified) {
            builder.setDeviceId(deviceId);
        }

        if (audioApi != static_cast<int32_t>(oboe::AudioApi::Unspecified)) {
            builder.setAudioApi(static_cast<oboe::AudioApi>(audioApi));
        }

        oboe::Result result = builder.openStream(mInputStream);
        if (result != oboe::Result::OK) {
            LOGE("Failed to open input stream. Error: %s", oboe::convertToText(result));
            mInputStreamState = StreamState::Error;
            return result;
        }

        mInputStreamSampleRate = mInputStream->getSampleRate();
        mInputStreamChannelCount = mInputStream->getChannelCount();
        mInputStreamBufferSizeInFrames = mInputStream->getBufferSizeInFrames();
        mInputStreamState = StreamState::Initialized;

        LOGI("Oboe input stream opened: sampleRate=%d, channels=%d, bufferSize=%d",
             mInputStreamSampleRate, mInputStreamChannelCount, mInputStreamBufferSizeInFrames);

        return oboe::Result::OK;
    }

    oboe::Result closeInputStream() {
        std::lock_guard<std::mutex> lock(mStreamLock);

        if (mInputStream) {
            stopInputStream();
            oboe::Result result = mInputStream->close();
            mInputStream.reset();
            mInputStreamState = StreamState::Closed;
            LOGI("Oboe input stream closed");
            return result;
        }
        mInputStreamState = StreamState::Uninitialized;
        return oboe::Result::OK;
    }

    oboe::Result startInputStream() {
        std::lock_guard<std::mutex> lock(mStreamLock);

        if (!mInputStream) {
            LOGE("Cannot start input stream: stream is null");
            return oboe::Result::ErrorNull;
        }

        if (mInputStreamState == StreamState::Started) {
            return oboe::Result::OK;
        }

        oboe::Result result = mInputStream->requestStart();
        if (result != oboe::Result::OK) {
            LOGE("Failed to start input stream. Error: %s", oboe::convertToText(result));
            mInputStreamState = StreamState::Error;
            return result;
        }

        mInputStreamState = StreamState::Started;
        LOGI("Oboe input stream started");
        return oboe::Result::OK;
    }

    oboe::Result stopInputStream() {
        std::lock_guard<std::mutex> lock(mStreamLock);

        if (!mInputStream) {
            return oboe::Result::OK;
        }

        oboe::Result result = mInputStream->requestStop();
        if (result != oboe::Result::OK) {
            LOGE("Failed to stop input stream. Error: %s", oboe::convertToText(result));
        }

        mInputStreamState = StreamState::Stopped;
        LOGI("Oboe input stream stopped");
        return result;
    }

    void setInputCallback(std::function<void(const float*, int32_t, int32_t, int32_t)> callback) {
        std::lock_guard<std::mutex> lock(mCallbackLock);
        mInputDataCallback = std::move(callback);
    }

    bool isInputStreamRunning() const {
        return mInputStreamState == StreamState::Started;
    }

    void setInputPreset(int32_t preset) {
        mInputPreset = static_cast<oboe::InputPreset>(preset);
    }

    oboe::Result onAudioReady(oboe::AudioStream *oboeStream,
                              void *audioData,
                              int32_t numFrames) override {
        if (oboeStream->getDirection() == oboe::Direction::Output) {
            return onOutputReady(oboeStream, audioData, numFrames);
        } else {
            return onInputReady(oboeStream, audioData, numFrames);
        }
    }

    oboe::Result onOutputReady(oboe::AudioStream *oboeStream,
                               void *audioData,
                               int32_t numFrames) {
        float *floatData = static_cast<float*>(audioData);
        int32_t channelCount = oboeStream->getChannelCount();

        memset(floatData, 0, static_cast<size_t>(numFrames) * channelCount * sizeof(float));

        std::lock_guard<std::mutex> lock(mCallbackLock);
        if (mDataCallback) {
            mDataCallback(floatData, numFrames, channelCount, oboeStream->getSampleRate());
        }

        if (mIsLatencyTunerEnabled && mLatencyTuner) {
            oboe::Result result = mLatencyTuner->tune();
            if (result != oboe::Result::OK) {
                LOGD("LatencyTuner tune in callback: %s", oboe::convertToText(result));
            }
        }

        return oboe::Result::OK;
    }

    oboe::Result onInputReady(oboe::AudioStream *oboeStream,
                              void *audioData,
                              int32_t numFrames) {
        float *floatData = static_cast<float*>(audioData);
        int32_t channelCount = oboeStream->getChannelCount();

        std::lock_guard<std::mutex> lock(mCallbackLock);
        if (mInputDataCallback) {
            mInputDataCallback(floatData, numFrames, channelCount, oboeStream->getSampleRate());
        }

        return oboe::Result::OK;
    }

    void onErrorAfterClose(oboe::AudioStream *oboeStream, oboe::Result error) override {
        LOGE("Oboe stream error after close: %s", oboe::convertToText(error));

        std::lock_guard<std::mutex> lock(mCallbackLock);
        if (oboeStream->getDirection() == oboe::Direction::Output) {
            mStreamState = StreamState::Error;
        } else {
            mInputStreamState = StreamState::Error;
        }

        if (mErrorCallback) {
            mErrorCallback(static_cast<int32_t>(error), std::string(oboe::convertToText(error)));
        }
    }

    void onErrorBeforeClose(oboe::AudioStream *oboeStream, oboe::Result error) override {
        LOGE("Oboe stream error before close: %s", oboe::convertToText(error));

        std::lock_guard<std::mutex> lock(mCallbackLock);
        if (oboeStream->getDirection() == oboe::Direction::Output) {
            mStreamState = StreamState::Disconnected;
        } else {
            mInputStreamState = StreamState::Disconnected;
        }

        if (mErrorCallback) {
            mErrorCallback(static_cast<int32_t>(error), std::string(oboe::convertToText(error)));
        }
    }

private:
    enum class StreamState : int32_t {
        Uninitialized = 0,
        Initialized = 1,
        Started = 2,
        Paused = 3,
        Stopped = 4,
        Closed = 5,
        Disconnected = 6,
        Error = 7
    };

    std::shared_ptr<oboe::AudioStream> mStream;
    std::shared_ptr<oboe::AudioStream> mInputStream;
    std::unique_ptr<oboe::LatencyTuner> mLatencyTuner;

    int32_t mSampleRate;
    int32_t mChannelCount;
    int32_t mFramesPerCallback;
    int32_t mDeviceId;
    oboe::AudioApi mAudioApi;
    oboe::AudioFormat mFormat;
    StreamState mStreamState;
    StreamState mInputStreamState;
    bool mIsLatencyTunerEnabled;
    int32_t mBufferSizeInFrames;

    int32_t mInputStreamSampleRate;
    int32_t mInputStreamChannelCount;
    int32_t mInputStreamFramesPerCallback;
    int32_t mInputStreamBufferSizeInFrames;
    oboe::InputPreset mInputPreset;

    std::mutex mStreamLock;
    std::mutex mCallbackLock;

    std::function<void(float*, int32_t, int32_t, int32_t)> mDataCallback;
    std::function<void(int32_t, const std::string&)> mErrorCallback;
    std::function<void(const float*, int32_t, int32_t, int32_t)> mInputDataCallback;
};

// Global backend instance
static std::unique_ptr<OboeBackendImpl> gBackend;
static std::mutex gBackendLock;

extern "C" {

JNIEXPORT jint JNICALL
Java_com_arenax3_audio_OboeBackend_nativeOpenStream(
    JNIEnv *env,
    jobject thiz,
    jint sampleRate,
    jint channelCount,
    jint framesPerCallback,
    jint deviceId,
    jint audioApi) {

    std::lock_guard<std::mutex> lock(gBackendLock);
    if (!gBackend) {
        gBackend = std::make_unique<OboeBackendImpl>();
    }

    oboe::Result result = gBackend->openStream(
        static_cast<int32_t>(sampleRate),
        static_cast<int32_t>(channelCount),
        static_cast<int32_t>(framesPerCallback),
        static_cast<int32_t>(deviceId),
        static_cast<int32_t>(audioApi));

    return static_cast<jint>(result);
}

JNIEXPORT jint JNICALL
Java_com_arenax3_audio_OboeBackend_nativeCloseStream(
    JNIEnv *env,
    jobject thiz) {

    std::lock_guard<std::mutex> lock(gBackendLock);
    if (!gBackend) {
        return static_cast<jint>(oboe::Result::ErrorNull);
    }

    oboe::Result result = gBackend->closeStream();
    return static_cast<jint>(result);
}

JNIEXPORT jint JNICALL
Java_com_arenax3_audio_OboeBackend_nativeStartStream(
    JNIEnv *env,
    jobject thiz) {

    std::lock_guard<std::mutex> lock(gBackendLock);
    if (!gBackend) {
        return static_cast<jint>(oboe::Result::ErrorNull);
    }

    oboe::Result result = gBackend->startStream();
    return static_cast<jint>(result);
}

JNIEXPORT jint JNICALL
Java_com_arenax3_audio_OboeBackend_nativeStopStream(
    JNIEnv *env,
    jobject thiz) {

    std::lock_guard<std::mutex> lock(gBackendLock);
    if (!gBackend) {
        return static_cast<jint>(oboe::Result::ErrorNull);
    }

    oboe::Result result = gBackend->stopStream();
    return static_cast<jint>(result);
}

JNIEXPORT jint JNICALL
Java_com_arenax3_audio_OboeBackend_nativePauseStream(
    JNIEnv *env,
    jobject thiz) {

    std::lock_guard<std::mutex> lock(gBackendLock);
    if (!gBackend) {
        return static_cast<jint>(oboe::Result::ErrorNull);
    }

    oboe::Result result = gBackend->pauseStream();
    return static_cast<jint>(result);
}

JNIEXPORT jint JNICALL
Java_com_arenax3_audio_OboeBackend_nativeFlushStream(
    JNIEnv *env,
    jobject thiz) {

    std::lock_guard<std::mutex> lock(gBackendLock);
    if (!gBackend) {
        return static_cast<jint>(oboe::Result::ErrorNull);
    }

    oboe::Result result = gBackend->flushStream();
    return static_cast<jint>(result);
}

JNIEXPORT jlong JNICALL
Java_com_arenax3_audio_OboeBackend_nativeGetFramesWritten(
    JNIEnv *env,
    jobject thiz) {

    std::lock_guard<std::mutex> lock(gBackendLock);
    if (!gBackend) {
        return -1;
    }

    return static_cast<jlong>(gBackend->getFramesWritten());
}

JNIEXPORT jlong JNICALL
Java_com_arenax3_audio_OboeBackend_nativeGetFramesRead(
    JNIEnv *env,
    jobject thiz) {

    std::lock_guard<std::mutex> lock(gBackendLock);
    if (!gBackend) {
        return -1;
    }

    return static_cast<jlong>(gBackend->getFramesRead());
}

JNIEXPORT jdouble JNICALL
Java_com_arenax3_audio_OboeBackend_nativeGetLatencyMs(
    JNIEnv *env,
    jobject thiz) {

    std::lock_guard<std::mutex> lock(gBackendLock);
    if (!gBackend) {
        return -1.0;
    }

    return static_cast<jdouble>(gBackend->getLatencyMs());
}

JNIEXPORT jint JNICALL
Java_com_arenax3_audio_OboeBackend_nativeGetSampleRate(
    JNIEnv *env,
    jobject thiz) {

    std::lock_guard<std::mutex> lock(gBackendLock);
    if (!gBackend) {
        return -1;
    }

    return static_cast<jint>(gBackend->getSampleRate());
}

JNIEXPORT jint JNICALL
Java_com_arenax3_audio_OboeBackend_nativeGetChannelCount(
    JNIEnv *env,
    jobject thiz) {

    std::lock_guard<std::mutex> lock(gBackendLock);
    if (!gBackend) {
        return -1;
    }

    return static_cast<jint>(gBackend->getChannelCount());
}

JNIEXPORT jint JNICALL
Java_com_arenax3_audio_OboeBackend_nativeGetBufferSizeInFrames(
    JNIEnv *env,
    jobject thiz) {

    std::lock_guard<std::mutex> lock(gBackendLock);
    if (!gBackend) {
        return -1;
    }

    return static_cast<jint>(gBackend->getBufferSizeInFrames());
}

JNIEXPORT jint JNICALL
Java_com_arenax3_audio_OboeBackend_nativeGetFramesPerCallback(
    JNIEnv *env,
    jobject thiz) {

    std::lock_guard<std::mutex> lock(gBackendLock);
    if (!gBackend) {
        return -1;
    }

    return static_cast<jint>(gBackend->getFramesPerCallback());
}

JNIEXPORT jint JNICALL
Java_com_arenax3_audio_OboeBackend_nativeGetStreamState(
    JNIEnv *env,
    jobject thiz) {

    std::lock_guard<std::mutex> lock(gBackendLock);
    if (!gBackend) {
        return -1;
    }

    return static_cast<jint>(gBackend->getStreamState());
}

JNIEXPORT jint JNICALL
Java_com_arenax3_audio_OboeBackend_nativeGetAudioApi(
    JNIEnv *env,
    jobject thiz) {

    std::lock_guard<std::mutex> lock(gBackendLock);
    if (!gBackend) {
        return -1;
    }

    return static_cast<jint>(gBackend->getAudioApi());
}

JNIEXPORT void JNICALL
Java_com_arenax3_audio_OboeBackend_nativeEnableLatencyTuner(
    JNIEnv *env,
    jobject thiz,
    jboolean enable) {

    std::lock_guard<std::mutex> lock(gBackendLock);
    if (!gBackend) {
        return;
    }

    gBackend->enableLatencyTuner(enable == JNI_TRUE);
}

JNIEXPORT void JNICALL
Java_com_arenax3_audio_OboeBackend_nativeTuneLatency(
    JNIEnv *env,
    jobject thiz) {

    std::lock_guard<std::mutex> lock(gBackendLock);
    if (!gBackend) {
        return;
    }

    gBackend->tuneLatency();
}

JNIEXPORT jint JNICALL
Java_com_arenax3_audio_OboeBackend_nativeSetBufferSizeInFrames(
    JNIEnv *env,
    jobject thiz,
    jint sizeFrames) {

    std::lock_guard<std::mutex> lock(gBackendLock);
    if (!gBackend) {
        return static_cast<jint>(oboe::Result::ErrorNull);
    }

    oboe::Result result = gBackend->setBufferSizeInFrames(static_cast<int32_t>(sizeFrames));
    return static_cast<jint>(result);
}

JNIEXPORT jboolean JNICALL
Java_com_arenax3_audio_OboeBackend_nativeIsStreamRunning(
    JNIEnv *env,
    jobject thiz) {

    std::lock_guard<std::mutex> lock(gBackendLock);
    if (!gBackend) {
        return JNI_FALSE;
    }

    return gBackend->isStreamRunning() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL
Java_com_arenax3_audio_OboeBackend_nativeOpenInputStream(
    JNIEnv *env,
    jobject thiz,
    jint sampleRate,
    jint channelCount,
    jint framesPerCallback,
    jint deviceId,
    jint audioApi) {

    std::lock_guard<std::mutex> lock(gBackendLock);
    if (!gBackend) {
        gBackend = std::make_unique<OboeBackendImpl>();
    }

    oboe::Result result = gBackend->openInputStream(
        static_cast<int32_t>(sampleRate),
        static_cast<int32_t>(channelCount),
        static_cast<int32_t>(framesPerCallback),
        static_cast<int32_t>(deviceId),
        static_cast<int32_t>(audioApi));

    return static_cast<jint>(result);
}

JNIEXPORT jint JNICALL
Java_com_arenax3_audio_OboeBackend_nativeCloseInputStream(
    JNIEnv *env,
    jobject thiz) {

    std::lock_guard<std::mutex> lock(gBackendLock);
    if (!gBackend) {
        return static_cast<jint>(oboe::Result::ErrorNull);
    }

    oboe::Result result = gBackend->closeInputStream();
    return static_cast<jint>(result);
}

JNIEXPORT jint JNICALL
Java_com_arenax3_audio_OboeBackend_nativeStartInputStream(
    JNIEnv *env,
    jobject thiz) {

    std::lock_guard<std::mutex> lock(gBackendLock);
    if (!gBackend) {
        return static_cast<jint>(oboe::Result::ErrorNull);
    }

    oboe::Result result = gBackend->startInputStream();
    return static_cast<jint>(result);
}

JNIEXPORT jint JNICALL
Java_com_arenax3_audio_OboeBackend_nativeStopInputStream(
    JNIEnv *env,
    jobject thiz) {

    std::lock_guard<std::mutex> lock(gBackendLock);
    if (!gBackend) {
        return static_cast<jint>(oboe::Result::ErrorNull);
    }

    oboe::Result result = gBackend->stopInputStream();
    return static_cast<jint>(result);
}

JNIEXPORT jboolean JNICALL
Java_com_arenax3_audio_OboeBackend_nativeIsInputStreamRunning(
    JNIEnv *env,
    jobject thiz) {

    std::lock_guard<std::mutex> lock(gBackendLock);
    if (!gBackend) {
        return JNI_FALSE;
    }

    return gBackend->isInputStreamRunning() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_arenax3_audio_OboeBackend_nativeSetInputPreset(
    JNIEnv *env,
    jobject thiz,
    jint preset) {

    std::lock_guard<std::mutex> lock(gBackendLock);
    if (!gBackend) {
        return;
    }

    gBackend->setInputPreset(static_cast<int32_t>(preset));
}

JNIEXPORT void JNICALL
Java_com_arenax3_audio_OboeBackend_nativeSetDataCallback(
    JNIEnv *env,
    jobject thiz,
    jobject callback) {

    if (!callback) {
        std::lock_guard<std::mutex> lock(gBackendLock);
        if (gBackend) {
            gBackend->setCallback(nullptr);
        }
        return;
    }

    JavaVM *jvm;
    env->GetJavaVM(&jvm);
    jobject globalCallback = env->NewGlobalRef(callback);

    auto cppCallback = [jvm, globalCallback](float* data, int32_t numFrames, int32_t channelCount, int32_t sampleRate) {
        JNIEnv *env;
        bool attached = false;
        jint result = jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
        if (result == JNI_EDETACHED) {
            result = jvm->AttachCurrentThread(&env, nullptr);
            attached = true;
        }

        if (result == JNI_OK && env) {
            jclass callbackClass = env->GetObjectClass(globalCallback);
            jmethodID methodId = env->GetMethodID(callbackClass, "onAudioData", "([FIII)V");

            if (methodId) {
                jfloatArray jData = env->NewFloatArray(numFrames * channelCount);
                env->SetFloatArrayRegion(jData, 0, numFrames * channelCount, data);
                env->CallVoidMethod(globalCallback, methodId, jData, numFrames, channelCount, sampleRate);
                env->DeleteLocalRef(jData);
            }

            env->DeleteLocalRef(callbackClass);
        }

        if (attached) {
            jvm->DetachCurrentThread();
        }
    };

    std::lock_guard<std::mutex> lock(gBackendLock);
    if (!gBackend) {
        gBackend = std::make_unique<OboeBackendImpl>();
    }
    gBackend->setCallback(std::move(cppCallback));
}

JNIEXPORT void JNICALL
Java_com_arenax3_audio_OboeBackend_nativeSetErrorCallback(
    JNIEnv *env,
    jobject thiz,
    jobject callback) {

    if (!callback) {
        std::lock_guard<std::mutex> lock(gBackendLock);
        if (gBackend) {
            gBackend->setErrorCallback(nullptr);
        }
        return;
    }

    JavaVM *jvm;
    env->GetJavaVM(&jvm);
    jobject globalCallback = env->NewGlobalRef(callback);

    auto cppCallback = [jvm, globalCallback](int32_t errorCode, const std::string& errorMessage) {
        JNIEnv *env;
        bool attached = false;
        jint result = jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
        if (result == JNI_EDETACHED) {
            result = jvm->AttachCurrentThread(&env, nullptr);
            attached = true;
        }

        if (result == JNI_OK && env) {
            jclass callbackClass = env->GetObjectClass(globalCallback);
            jmethodID methodId = env->GetMethodID(callbackClass, "onError", "(ILjava/lang/String;)V");

            if (methodId) {
                jstring jMessage = env->NewStringUTF(errorMessage.c_str());
                env->CallVoidMethod(globalCallback, methodId, static_cast<jint>(errorCode), jMessage);
                env->DeleteLocalRef(jMessage);
            }

            env->DeleteLocalRef(callbackClass);
        }

        if (attached) {
            jvm->DetachCurrentThread();
        }
    };

    std::lock_guard<std::mutex> lock(gBackendLock);
    if (!gBackend) {
        gBackend = std::make_unique<OboeBackendImpl>();
    }
    gBackend->setErrorCallback(std::move(cppCallback));
}

JNIEXPORT void JNICALL
Java_com_arenax3_audio_OboeBackend_nativeSetInputCallback(
    JNIEnv *env,
    jobject thiz,
    jobject callback) {

    if (!callback) {
        std::lock_guard<std::mutex> lock(gBackendLock);
        if (gBackend) {
            gBackend->setInputCallback(nullptr);
        }
        return;
    }

    JavaVM *jvm;
    env->GetJavaVM(&jvm);
    jobject globalCallback = env->NewGlobalRef(callback);

    auto cppCallback = [jvm, globalCallback](const float* data, int32_t numFrames, int32_t channelCount, int32_t sampleRate) {
        JNIEnv *env;
        bool attached = false;
        jint result = jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
        if (result == JNI_EDETACHED) {
            result = jvm->AttachCurrentThread(&env, nullptr);
            attached = true;
        }

        if (result == JNI_OK && env) {
            jclass callbackClass = env->GetObjectClass(globalCallback);
            jmethodID methodId = env->GetMethodID(callbackClass, "onInputData", "([FIII)V");

            if (methodId) {
                jfloatArray jData = env->NewFloatArray(numFrames * channelCount);
                env->SetFloatArrayRegion(jData, 0, numFrames * channelCount, data);
                env->CallVoidMethod(globalCallback, methodId, jData, numFrames, channelCount, sampleRate);
                env->DeleteLocalRef(jData);
            }

            env->DeleteLocalRef(callbackClass);
        }

        if (attached) {
            jvm->DetachCurrentThread();
        }
    };

    std::lock_guard<std::mutex> lock(gBackendLock);
    if (!gBackend) {
        gBackend = std::make_unique<OboeBackendImpl>();
    }
    gBackend->setInputCallback(std::move(cppCallback));
}

JNIEXPORT void JNICALL
Java_com_arenax3_audio_OboeBackend_nativeReleaseCallbacks(
    JNIEnv *env,
    jobject thiz) {

    std::lock_guard<std::mutex> lock(gBackendLock);
    if (gBackend) {
        gBackend->setCallback(nullptr);
        gBackend->setErrorCallback(nullptr);
        gBackend->setInputCallback(nullptr);
    }
}

JNIEXPORT void JNICALL
Java_com_arenax3_audio_OboeBackend_nativeDestroy(
    JNIEnv *env,
    jobject thiz) {

    std::lock_guard<std::mutex> lock(gBackendLock);
    if (gBackend) {
        gBackend->closeStream();
        gBackend->closeInputStream();
        gBackend.reset();
        LOGI("OboeBackend destroyed");
    }
}

} // extern "C"

} // namespace arenax3
} // namespace com