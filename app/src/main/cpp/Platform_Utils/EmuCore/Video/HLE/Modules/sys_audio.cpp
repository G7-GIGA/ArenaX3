#include <cmath>
#include <cstring>
#include <algorithm>
#include <vector>
#include <mutex>

namespace com {
namespace arenax3 {

// =============================================
// Internal types
// =============================================
using i16 = short;
using i32 = int;
using u32 = unsigned int;
using f32 = float;

constexpr f32 PI = 3.14159265358979323846f;
constexpr u32 SAMPLE_RATE = 48000;
constexpr u32 MAX_CHANNELS = 8;
constexpr u32 MAX_ACTIVE_VOICES = 64;
constexpr u32 MAX_SOUNDS = 256;
constexpr u32 MAX_MIX_SOURCES = 16;

// =============================================
// WAV header parsing
// =============================================
struct WavInfo {
    u32 dataSize = 0;
    u32 sampleRate = 0;
    u16 channels = 0;
    u16 bitsPerSample = 0;
    const i16* sampleData = nullptr;
};

static bool ParseWav(const void* raw, u32 size, WavInfo& out) {
    const u8* data = static_cast<const u8*>(raw);
    if (size < 44) return false;
    if (std::memcmp(data, "RIFF", 4) != 0) return false;
    if (std::memcmp(data + 8, "WAVE", 4) != 0) return false;
    if (std::memcmp(data + 12, "fmt ", 4) != 0) return false;

    u32 fmtSize = *reinterpret_cast<const u32*>(data + 16);
    if (fmtSize < 16) return false;

    u16 audioFormat = *reinterpret_cast<const u16*>(data + 20);
    if (audioFormat != 1) return false; // PCM only

    out.channels = *reinterpret_cast<const u16*>(data + 22);
    out.sampleRate = *reinterpret_cast<const u32*>(data + 24);
    out.bitsPerSample = *reinterpret_cast<const u16*>(data + 34);

    u32 pos = 36;
    while (pos + 8 <= size) {
        if (std::memcmp(data + pos, "data", 4) == 0) {
            out.dataSize = *reinterpret_cast<const u32*>(data + pos + 4);
            if (pos + 8 + out.dataSize > size) return false;
            out.sampleData = reinterpret_cast<const i16*>(data + pos + 8);
            return true;
        }
        u32 chunkSize = *reinterpret_cast<const u32*>(data + pos + 4);
        pos += 8 + chunkSize;
    }
    return false;
}

// =============================================
// Sound resource
// =============================================
struct Sound {
    const i16* data = nullptr;
    u32 numFrames = 0;
    u32 sampleRate = 0;
    u16 channels = 0;
    f32 volume = 1.0f;
    f32 pan = 0.0f;
    bool loop = false;
    bool active = false;
};

// =============================================
// Active voice (playback instance)
// =============================================
struct Voice {
    i32 soundId = -1;
    f32 position = 0.0f;         // sample position as float for interpolation
    f32 volume = 1.0f;
    f32 pan = 0.0f;
    f32 pitch = 1.0f;
    bool playing = false;
    bool paused = false;
};

// =============================================
// Mix source (submix input, e.g. mic echo)
// =============================================
struct MixSource {
    const f32* input = nullptr;
    u32 numFrames = 0;
    f32 volume = 1.0f;
    f32 pan = 0.0f;
    bool active = false;
};

// =============================================
// ArenaX3 Audio System
// =============================================
class AudioSystem {
public:
    static AudioSystem& Instance() {
        static AudioSystem sys;
        return sys;
    }

    bool Initialize() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (initialized_) return true;
        initialized_ = true;
        masterVolume_ = 1.0f;
        return true;
    }

    void Shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        initialized_ = false;
        for (auto& s : sounds_) s.active = false;
        for (auto& v : voices_) v.playing = false;
    }

    // ------------------------
    // Sound management
    // ------------------------
    i32 LoadSound(const void* wavData, u32 size) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!initialized_) return -1;

        WavInfo info;
        if (!ParseWav(wavData, size, info)) return -1;
        if (info.channels > 2) return -2;

        i32 slot = -1;
        for (u32 i = 0; i < MAX_SOUNDS; ++i) {
            if (!sounds_[i].active) { slot = static_cast<i32>(i); break; }
        }
        if (slot < 0) return -3;

        Sound& s = sounds_[slot];
        s.data = info.sampleData;
        s.numFrames = info.dataSize / (info.channels * (info.bitsPerSample / 8));
        s.sampleRate = info.sampleRate;
        s.channels = info.channels;
        s.volume = 1.0f;
        s.pan = 0.0f;
        s.loop = false;
        s.active = true;
        return slot;
    }

    void UnloadSound(i32 soundId) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (soundId < 0 || soundId >= static_cast<i32>(MAX_SOUNDS)) return;
        sounds_[soundId].active = false;
        // Stop voices using this sound
        for (auto& v : voices_) {
            if (v.soundId == soundId) v.playing = false;
        }
    }

    void SetSoundVolume(i32 soundId, f32 vol) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (soundId < 0 || soundId >= static_cast<i32>(MAX_SOUNDS)) return;
        if (!sounds_[soundId].active) return;
        sounds_[soundId].volume = std::max(0.0f, std::min(vol, 1.0f));
    }

    void SetSoundLoop(i32 soundId, bool loop) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (soundId < 0 || soundId >= static_cast<i32>(MAX_SOUNDS)) return;
        if (!sounds_[soundId].active) return;
        sounds_[soundId].loop = loop;
    }

    // ------------------------
    // Voice / Playback
    // ------------------------
    i32 Play(i32 soundId, f32 volume, f32 pan, f32 pitch, bool paused) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!initialized_) return -1;
        if (soundId < 0 || soundId >= static_cast<i32>(MAX_SOUNDS)) return -1;
        if (!sounds_[soundId].active) return -1;

        i32 slot = -1;
        for (u32 i = 0; i < MAX_ACTIVE_VOICES; ++i) {
            if (!voices_[i].playing) { slot = static_cast<i32>(i); break; }
        }
        if (slot < 0) return -1;

        voices_[slot].soundId = soundId;
        voices_[slot].position = 0.0f;
        voices_[slot].volume = std::max(0.0f, std::min(volume, 1.0f));
        voices_[slot].pan = std::max(-1.0f, std::min(pan, 1.0f));
        voices_[slot].pitch = std::max(0.1f, std::min(pitch, 4.0f));
        voices_[slot].playing = true;
        voices_[slot].paused = paused;
        return slot;
    }

    void Stop(i32 voiceId) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (voiceId < 0 || voiceId >= static_cast<i32>(MAX_ACTIVE_VOICES)) return;
        voices_[voiceId].playing = false;
    }

    void Pause(i32 voiceId) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (voiceId < 0 || voiceId >= static_cast<i32>(MAX_ACTIVE_VOICES)) return;
        if (voices_[voiceId].playing) voices_[voiceId].paused = true;
    }

    void Resume(i32 voiceId) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (voiceId < 0 || voiceId >= static_cast<i32>(MAX_ACTIVE_VOICES)) return;
        if (voices_[voiceId].playing) voices_[voiceId].paused = false;
    }

    bool IsPlaying(i32 voiceId) const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (voiceId < 0 || voiceId >= static_cast<i32>(MAX_ACTIVE_VOICES)) return false;
        return voices_[voiceId].playing && !voices_[voiceId].paused;
    }

    void SetVoiceVolume(i32 voiceId, f32 vol) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (voiceId < 0 || voiceId >= static_cast<i32>(MAX_ACTIVE_VOICES)) return;
        voices_[voiceId].volume = std::max(0.0f, std::min(vol, 1.0f));
    }

    void SetVoicePan(i32 voiceId, f32 pan) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (voiceId < 0 || voiceId >= static_cast<i32>(MAX_ACTIVE_VOICES)) return;
        voices_[voiceId].pan = std::max(-1.0f, std::min(pan, 1.0f));
    }

    void SetVoicePitch(i32 voiceId, f32 pitch) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (voiceId < 0 || voiceId >= static_cast<i32>(MAX_ACTIVE_VOICES)) return;
        voices_[voiceId].pitch = std::max(0.1f, std::min(pitch, 4.0f));
    }

    // ------------------------
    // Mix sources (e.g. mic input pass-through)
    // ------------------------
    i32 CreateMixSource() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (u32 i = 0; i < MAX_MIX_SOURCES; ++i) {
            if (!mixSources_[i].active) {
                mixSources_[i].active = true;
                mixSources_[i].volume = 1.0f;
                mixSources_[i].pan = 0.0f;
                mixSources_[i].input = nullptr;
                mixSources_[i].numFrames = 0;
                return static_cast<i32>(i);
            }
        }
        return -1;
    }

    void DestroyMixSource(i32 sourceId) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (sourceId < 0 || sourceId >= static_cast<i32>(MAX_MIX_SOURCES)) return;
        mixSources_[sourceId].active = false;
    }

    void SetMixSourceData(i32 sourceId, const f32* input, u32 numFrames) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (sourceId < 0 || sourceId >= static_cast<i32>(MAX_MIX_SOURCES)) return;
        if (!mixSources_[sourceId].active) return;
        mixSources_[sourceId].input = input;
        mixSources_[sourceId].numFrames = numFrames;
    }

    void SetMixSourceVolume(i32 sourceId, f32 vol) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (sourceId < 0 || sourceId >= static_cast<i32>(MAX_MIX_SOURCES)) return;
        if (!mixSources_[sourceId].active) return;
        mixSources_[sourceId].volume = std::max(0.0f, std::min(vol, 1.0f));
    }

    void SetMixSourcePan(i32 sourceId, f32 pan) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (sourceId < 0 || sourceId >= static_cast<i32>(MAX_MIX_SOURCES)) return;
        if (!mixSources_[sourceId].active) return;
        mixSources_[sourceId].pan = std::max(-1.0f, std::min(pan, 1.0f));
    }

    // ------------------------
    // Master control
    // ------------------------
    void SetMasterVolume(f32 vol) {
        std::lock_guard<std::mutex> lock(mutex_);
        masterVolume_ = std::max(0.0f, std::min(vol, 1.0f));
    }

    f32 GetMasterVolume() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return masterVolume_;
    }

    // ------------------------
    // Main processing callback (called from audio thread)
    // output: interleaved stereo f32 buffer
    // numFrames: number of stereo frame pairs
    // ------------------------
    void Process(f32* output, u32 numFrames) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!initialized_) {
            std::memset(output, 0, numFrames * 2 * sizeof(f32));
            return;
        }

        // Clear buffer
        std::memset(output, 0, numFrames * 2 * sizeof(f32));

        // Mix all active voices
        for (u32 vIdx = 0; vIdx < MAX_ACTIVE_VOICES; ++vIdx) {
            Voice& v = voices_[vIdx];
            if (!v.playing || v.paused) continue;
            if (v.soundId < 0 || v.soundId >= static_cast<i32>(MAX_SOUNDS)) continue;
            Sound& s = sounds_[v.soundId];
            if (!s.active || !s.data) continue;
            if (s.numFrames == 0) continue;

            f32 step = (static_cast<f32>(s.sampleRate) / static_cast<f32>(SAMPLE_RATE)) * v.pitch;
            f32 soundVol = v.volume * s.volume;
            f32 effectiveVol = soundVol;
            f32 pan = std::max(-1.0f, std::min(s.pan + v.pan, 1.0f));
            f32 leftGain = (pan <= 0.0f) ? 1.0f : (1.0f - pan);
            f32 rightGain = (pan >= 0.0f) ? 1.0f : (1.0f + pan);

            f32 pos = v.position;
            u32 maxFrames = s.numFrames;

            for (u32 frame = 0; frame < numFrames; ++frame) {
                u32 idx0 = static_cast<u32>(pos);
                f32 frac = pos - static_cast<f32>(idx0);
                u32 idx1 = idx0 + 1;

                f32 sampleMono = 0.0f;
                if (s.channels == 1) {
                    i16 s0 = s.data[idx0];
                    i16 s1 = (idx1 < maxFrames) ? s.data[idx1] : s0;
                    sampleMono = (static_cast<f32>(s0) + frac * static_cast<f32>(s1 - s0)) / 32768.0f;
                } else if (s.channels == 2) {
                    u32 i0 = idx0 * 2;
                    u32 i1 = idx1 * 2;
                    i16 L0 = s.data[i0];
                    i16 R0 = s.data[i0 + 1];
                    f32 mono0 = (static_cast<f32>(L0) + static_cast<f32>(R0)) * 0.5f / 32768.0f;
                    if (idx1 < maxFrames) {
                        i16 L1 = s.data[i1];
                        i16 R1 = s.data[i1 + 1];
                        f32 mono1 = (static_cast<f32>(L1) + static_cast<f32>(R1)) * 0.5f / 32768.0f;
                        sampleMono = mono0 + frac * (mono1 - mono0);
                    } else {
                        sampleMono = mono0;
                    }
                }

                f32 outL = sampleMono * effectiveVol * leftGain;
                f32 outR = sampleMono * effectiveVol * rightGain;
                output[frame * 2 + 0] += outL;
                output[frame * 2 + 1] += outR;

                pos += step;
                if (pos >= static_cast<f32>(maxFrames)) {
                    if (s.loop) {
                        pos = std::fmod(pos, static_cast<f32>(maxFrames));
                    } else {
                        v.playing = false;
                        break;
                    }
                }
            }
            v.position = pos;
        }

        // Mix active mix sources (external mono input)
        for (u32 mIdx = 0; mIdx < MAX_MIX_SOURCES; ++mIdx) {
            MixSource& src = mixSources_[mIdx];
            if (!src.active || !src.input) continue;
            u32 avail = std::min(numFrames, src.numFrames);
            f32 vol = src.volume;
            f32 pan = std::max(-1.0f, std::min(src.pan, 1.0f));
            f32 leftGain = (pan <= 0.0f) ? 1.0f : (1.0f - pan);
            f32 rightGain = (pan >= 0.0f) ? 1.0f : (1.0f + pan);
            for (u32 i = 0; i < avail; ++i) {
                output[i * 2 + 0] += src.input[i] * vol * leftGain;
                output[i * 2 + 1] += src.input[i] * vol * rightGain;
            }
        }

        // Apply master volume and soft clip
        for (u32 i = 0; i < numFrames * 2; ++i) {
            f32 val = output[i] * masterVolume_;
            if (val > 1.0f) val = 1.0f;
            else if (val < -1.0f) val = -1.0f;
            output[i] = val;
        }
    }

private:
    AudioSystem() = default;
    ~AudioSystem() = default;

    mutable std::mutex mutex_;
    bool initialized_ = false;
    f32 masterVolume_ = 1.0f;

    Sound sounds_[MAX_SOUNDS];
    Voice voices_[MAX_ACTIVE_VOICES];
    MixSource mixSources_[MAX_MIX_SOURCES];
};

// =============================================
// Public C API wrapper (optional)
// =============================================
extern "C" {

int ax3_audio_init() {
    return AudioSystem::Instance().Initialize() ? 0 : -1;
}

void ax3_audio_shutdown() {
    AudioSystem::Instance().Shutdown();
}

int ax3_load_sound(const void* data, unsigned int size) {
    return AudioSystem::Instance().LoadSound(data, size);
}

void ax3_unload_sound(int id) {
    AudioSystem::Instance().UnloadSound(id);
}

int ax3_play(int soundId, float vol, float pan, float pitch, int paused) {
    return AudioSystem::Instance().Play(soundId, vol, pan, pitch, paused != 0);
}

void ax3_stop(int voiceId) {
    AudioSystem::Instance().Stop(voiceId);
}

void ax3_set_master_volume(float vol) {
    AudioSystem::Instance().SetMasterVolume(vol);
}

void ax3_process_audio(float* output, unsigned int numFrames) {
    AudioSystem::Instance().Process(output, numFrames);
}

} // extern "C"

} // namespace arenax3
} // namespace com