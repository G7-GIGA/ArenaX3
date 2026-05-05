#ifndef OBOE_BACKEND_HPP
#define OBOE_BACKEND_HPP

#include <oboe/Oboe.h>
#include <atomic>
#include <functional>
#include <memory>
#include <vector>
#include <mutex>

namespace com {
namespace arenax3 {

class OboeBackend : public oboe::AudioStreamCallback {
public:
    using AudioCallback = std::function<void(const float* input, float* output, int32_t numFrames)>;

    struct Config {
        int32_t sampleRate = 48000;
        int32_t framesPerBurst = 64;      // Low-latency burst size
        oboe::AudioFormat format = oboe::AudioFormat::Float;
        oboe::ChannelCount channelCount = oboe::ChannelCount::Stereo;
        oboe::PerformanceMode perfMode = oboe::PerformanceMode::LowLatency;
        oboe::SharingMode sharingMode = oboe::SharingMode::Exclusive;
        oboe::Direction direction = oboe::Direction::Output; // Or InputOutput for full duplex
        int32_t bufferSizeInBursts = 2;    // 2 bursts for ultra-low latency
    };

    explicit OboeBackend(const Config& config = Config());
    ~OboeBackend();

    // Delete copy/move
    OboeBackend(const OboeBackend&) = delete;
    OboeBackend& operator=(const OboeBackend&) = delete;

    bool start();
    bool stop();
    bool isRunning() const { return mIsRunning.load(); }

    void setCallback(AudioCallback callback);
    void setInputEnabled(bool enable);
    bool isInputEnabled() const { return mInputEnabled.load(); }

    // Latency control
    bool setBufferSize(int32_t requestedFrames);
    int32_t getBufferSize() const;
    int32_t getFramesPerBurst() const { return mConfig.framesPerBurst; }
    double getActualLatencyMillis() const;

    // Device info
    oboe::AudioStream* getStream() { return mStream.get(); }
    bool isAAudioSupported() const;

private:
    // Oboe callback implementation
    oboe::DataCallbackResult onAudioReady(oboe::AudioStream* stream, void* audioData, int32_t numFrames) override;
    void onErrorBeforeClose(oboe::AudioStream* stream, oboe::Result error) override;
    void onErrorAfterClose(oboe::AudioStream* stream, oboe::Result error) override;

    bool openStream();
    void closeStream();
    oboe::Result setupStreamParameters(std::shared_ptr<oboe::AudioStreamBuilder>& builder);
    void logStreamInfo() const;

    Config mConfig;
    std::shared_ptr<oboe::AudioStream> mStream;
    std::atomic<bool> mIsRunning{false};
    std::atomic<bool> mInputEnabled{false};
    AudioCallback mUserCallback;
    mutable std::mutex mCallbackMutex;

    // Duplex buffers (if needed for full duplex)
    std::vector<float> mInputBuffer;
    std::vector<float> mOutputBuffer;
    std::mutex mBufferMutex;
};

} // namespace arenax3
} // namespace com

#endif // OBOE_BACKEND_HPP