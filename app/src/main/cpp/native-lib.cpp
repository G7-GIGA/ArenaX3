#include <jni.h>
#include <string>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <chrono>

namespace com {
namespace arenax3 {

class EmulatorEngine {
private:
    bool initialized = false;
    bool running = false;
    int fps = 0;
    std::mutex mutex;

public:
    EmulatorEngine() = default;
    ~EmulatorEngine() = default;

    jlong initialize() {
        std::lock_guard<std::mutex> lock(mutex);
        initialized = true;
        return reinterpret_cast<jlong>(this);
    }

    void shutdown() {
        std::lock_guard<std::mutex> lock(mutex);
        initialized = false;
        running = false;
    }

    void start() {
        std::lock_guard<std::mutex> lock(mutex);
        if (initialized && !running) {
            running = true;
        }
    }

    void stop() {
        std::lock_guard<std::mutex> lock(mutex);
        running = false;
    }

    jboolean isRunning() {
        std::lock_guard<std::mutex> lock(mutex);
        return running ? JNI_TRUE : JNI_FALSE;
    }

    jstring getVersion(JNIEnv* env) {
        std::string version = "ArenaX3 Emulator v2.0.0";
        return env->NewStringUTF(version.c_str());
    }

    void setGraphicsApi(jint api) {
        // API: 0=OpenGL, 1=Vulkan, 2=Software
        (void)api;
    }

    void setCpuCores(jint cores) {
        (void)cores;
    }

    void setRamSize(jint mb) {
        (void)mb;
    }

    jint getFps() {
        return fps;
    }

    void updateFps() {
        static auto lastTime = std::chrono::steady_clock::now();
        static int frameCount = 0;
        
        frameCount++;
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime).count();
        
        if (elapsed >= 1000) {
            fps = frameCount;
            frameCount = 0;
            lastTime = now;
        }
    }

    void injectTouch(jfloat x, jfloat y, jint action) {
        (void)x;
        (void)y;
        (void)action;
    }

    void injectKey(jint keyCode, jboolean down) {
        (void)keyCode;
        (void)down;
    }
};

static std::unordered_map<jlong, std::unique_ptr<EmulatorEngine>> engines;
static std::mutex enginesMutex;

} // namespace arenax3
} // namespace com

using namespace com::arenax3;

extern "C" {

JNIEXPORT jlong JNICALL
Java_com_arenax3_ArenaX3Emulator_nativeInit(JNIEnv* env, jobject thiz, jstring configPath) {
    (void)thiz;
    (void)configPath;
    
    auto engine = std::make_unique<EmulatorEngine>();
    jlong handle = engine->initialize();
    
    std::lock_guard<std::mutex> lock(enginesMutex);
    engines[handle] = std::move(engine);
    
    return handle;
}

JNIEXPORT void JNICALL
Java_com_arenax3_ArenaX3Emulator_nativeStart(JNIEnv* env, jobject thiz, jlong handle) {
    (void)env;
    (void)thiz;
    
    std::lock_guard<std::mutex> lock(enginesMutex);
    auto it = engines.find(handle);
    if (it != engines.end()) {
        it->second->start();
    }
}

JNIEXPORT void JNICALL
Java_com_arenax3_ArenaX3Emulator_nativeStop(JNIEnv* env, jobject thiz, jlong handle) {
    (void)env;
    (void)thiz;
    
    std::lock_guard<std::mutex> lock(enginesMutex);
    auto it = engines.find(handle);
    if (it != engines.end()) {
        it->second->stop();
    }
}

JNIEXPORT jboolean JNICALL
Java_com_arenax3_ArenaX3Emulator_nativeIsRunning(JNIEnv* env, jobject thiz, jlong handle) {
    (void)env;
    (void)thiz;
    
    std::lock_guard<std::mutex> lock(enginesMutex);
    auto it = engines.find(handle);
    if (it != engines.end()) {
        return it->second->isRunning();
    }
    return JNI_FALSE;
}

JNIEXPORT jstring JNICALL
Java_com_arenax3_ArenaX3Emulator_nativeGetVersion(JNIEnv* env, jobject thiz, jlong handle) {
    (void)thiz;
    
    std::lock_guard<std::mutex> lock(enginesMutex);
    auto it = engines.find(handle);
    if (it != engines.end()) {
        return it->second->getVersion(env);
    }
    return env->NewStringUTF("Unknown");
}

JNIEXPORT void JNICALL
Java_com_arenax3_ArenaX3Emulator_nativeSetGraphicsApi(JNIEnv* env, jobject thiz, jlong handle, jint api) {
    (void)env;
    (void)thiz;
    
    std::lock_guard<std::mutex> lock(enginesMutex);
    auto it = engines.find(handle);
    if (it != engines.end()) {
        it->second->setGraphicsApi(api);
    }
}

JNIEXPORT void JNICALL
Java_com_arenax3_ArenaX3Emulator_nativeSetCpuCores(JNIEnv* env, jobject thiz, jlong handle, jint cores) {
    (void)env;
    (void)thiz;
    
    std::lock_guard<std::mutex> lock(enginesMutex);
    auto it = engines.find(handle);
    if (it != engines.end()) {
        it->second->setCpuCores(cores);
    }
}

JNIEXPORT void JNICALL
Java_com_arenax3_ArenaX3Emulator_nativeSetRamSize(JNIEnv* env, jobject thiz, jlong handle, jint sizeMB) {
    (void)env;
    (void)thiz;
    
    std::lock_guard<std::mutex> lock(enginesMutex);
    auto it = engines.find(handle);
    if (it != engines.end()) {
        it->second->setRamSize(sizeMB);
    }
}

JNIEXPORT jint JNICALL
Java_com_arenax3_ArenaX3Emulator_nativeGetFps(JNIEnv* env, jobject thiz, jlong handle) {
    (void)env;
    (void)thiz;
    
    std::lock_guard<std::mutex> lock(enginesMutex);
    auto it = engines.find(handle);
    if (it != engines.end()) {
        return it->second->getFps();
    }
    return 0;
}

JNIEXPORT void JNICALL
Java_com_arenax3_ArenaX3Emulator_nativeInjectTouch(JNIEnv* env, jobject thiz, jlong handle, jfloat x, jfloat y, jint action) {
    (void)env;
    (void)thiz;
    
    std::lock_guard<std::mutex> lock(enginesMutex);
    auto it = engines.find(handle);
    if (it != engines.end()) {
        it->second->injectTouch(x, y, action);
    }
}

JNIEXPORT void JNICALL
Java_com_arenax3_ArenaX3Emulator_nativeInjectKey(JNIEnv* env, jobject thiz, jlong handle, jint keyCode, jboolean down) {
    (void)env;
    (void)thiz;
    
    std::lock_guard<std::mutex> lock(enginesMutex);
    auto it = engines.find(handle);
    if (it != engines.end()) {
        it->second->injectKey(keyCode, down);
    }
}

JNIEXPORT void JNICALL
Java_com_arenax3_ArenaX3Emulator_nativeDispose(JNIEnv* env, jobject thiz, jlong handle) {
    (void)env;
    (void)thiz;
    
    std::lock_guard<std::mutex> lock(enginesMutex);
    auto it = engines.find(handle);
    if (it != engines.end()) {
        it->second->shutdown();
        engines.erase(it);
    }
}

JNIEXPORT void JNICALL
Java_com_arenax3_ArenaX3Emulator_nativeUpdateFrame(JNIEnv* env, jobject thiz, jlong handle) {
    (void)env;
    (void)thiz;
    
    std::lock_guard<std::mutex> lock(enginesMutex);
    auto it = engines.find(handle);
    if (it != engines.end()) {
        it->second->updateFps();
    }
}

} // extern "C"