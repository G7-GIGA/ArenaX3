// SPU_Runtime.hpp
// SPU Audio Emulation for ArenaX3
// Copyright (c) 2024 com.arenax3. All rights reserved.

#ifndef SPU_RUNTIME_HPP
#define SPU_RUNTIME_HPP

#include <cstdint>
#include <cstddef>
#include <vector>
#include <array>
#include <functional>
#include <memory>
#include <mutex>
#include <atomic>

namespace com {
namespace arenax3 {

// ============================================================================
// Configuration constants
// ============================================================================

constexpr uint32_t SPU_SAMPLE_RATE = 44100;
constexpr uint32_t SPU_VOICE_COUNT = 24;
constexpr uint32_t SPU_ADPCM_BLOCK_SIZE = 28;
constexpr uint32_t SPU_RAM_SIZE = 512 * 1024;  // 512 KB
constexpr uint32_t SPU_MAX_VOLUME = 0x3FFF;
constexpr uint32_t SPU_REVERB_BUFFER_SIZE = 32768;

// ============================================================================
// Enumerations and bit flags
// ============================================================================

enum class SPU_VoiceMode : uint8_t {
    Normal = 0,
    Loop = 1,
    Noise = 2,
    ADPCM = 3
};

enum class SPU_InterruptType : uint8_t {
    None = 0,
    VoiceDone = 1,
    CaptureFinished = 2,
    ReverbFinished = 3
};

// ============================================================================
// Data structures
// ============================================================================

#pragma pack(push, 1)

struct SPU_ADPCM_Coefficient {
    int16_t coef[2];
};

struct SPU_ADPCM_Block {
    uint8_t data[SPU_ADPCM_BLOCK_SIZE];
    uint8_t predict_nr;
    uint8_t shift_factor;
    int16_t sample1;
    int16_t sample2;
};

struct SPU_Voice_Registers {
    uint32_t start_addr;      // Start address in SPU RAM
    uint32_t current_addr;    // Current read address
    uint32_t loop_addr;       // Loop start address
    uint32_t sample_rate;     // Sample rate (Hz)
    uint8_t volume_left;      // Left volume (0-127)
    uint8_t volume_right;     // Right volume (0-127)
    uint8_t pitch;            // Pitch adjustment
    uint8_t adsr_attack;      // Attack rate
    uint8_t adsr_decay;       // Decay rate
    uint8_t adsr_sustain;     // Sustain level
    uint8_t adsr_release;     // Release rate
    uint8_t adsr_state;       // Current envelope state
    uint16_t adsr_volume;     // Current envelope volume
    bool key_on;              // Key on flag
    bool key_off;             // Key off flag
    bool looping;             // Loop enabled
    bool noise_freq;          // Noise frequency mode
    SPU_VoiceMode mode;       // Voice playback mode
    uint8_t adpcm_coef_index; // Current ADPCM coefficient index
    int16_t adpcm_prev1;      // ADPCM previous sample 1
    int16_t adpcm_prev2;      // ADPCM previous sample 2
};

#pragma pack(pop)

struct SPU_Reverb_Params {
    uint16_t delay;
    uint16_t feedback;
    uint16_t damping;
    uint16_t level;
    bool enabled;
};

struct SPU_Audio_Buffer {
    std::vector<int16_t> left;
    std::vector<int16_t> right;
    size_t samples_available;
    uint32_t sample_rate;
};

// ============================================================================
// Callback types
// ============================================================================

using SPU_InterruptCallback = std::function<void(SPU_InterruptType, uint8_t)>;
using SPU_DMASampleCallback = std::function<void(uint8_t*, uint32_t)>;

// ============================================================================
// Main SPU Runtime Class
// ============================================================================

class SPU_Runtime {
public:
    // Constructor / Destructor
    SPU_Runtime();
    ~SPU_Runtime();
    
    // Prevent copying
    SPU_Runtime(const SPU_Runtime&) = delete;
    SPU_Runtime& operator=(const SPU_Runtime&) = delete;
    
    // Initialization and shutdown
    bool Initialize(uint32_t sample_rate = SPU_SAMPLE_RATE);
    void Shutdown();
    bool IsInitialized() const { return m_initialized; }
    void Reset();
    
    // SPU RAM management
    bool LoadSampleData(uint32_t address, const uint8_t* data, uint32_t size);
    bool ReadSampleData(uint32_t address, uint8_t* buffer, uint32_t size);
    void ClearRAM();
    uint8_t* GetRAMPointer() { return m_spu_ram.data(); }
    const uint8_t* GetRAMPointer() const { return m_spu_ram.data(); }
    
    // Voice control
    bool VoiceOn(uint8_t voice_id);
    bool VoiceOff(uint8_t voice_id);
    void VoiceKill(uint8_t voice_id);
    void AllVoicesOff();
    void AllVoicesKill();
    
    bool SetVoiceStartAddress(uint8_t voice_id, uint32_t address);
    bool SetVoiceLoopAddress(uint8_t voice_id, uint32_t address);
    bool SetVoiceVolume(uint8_t voice_id, uint8_t left, uint8_t right);
    bool SetVoicePitch(uint8_t voice_id, uint8_t pitch);
    bool SetVoiceSampleRate(uint8_t voice_id, uint32_t rate);
    bool SetVoiceMode(uint8_t voice_id, SPU_VoiceMode mode);
    bool SetVoiceADSR(uint8_t voice_id, uint8_t attack, uint8_t decay, 
                      uint8_t sustain, uint8_t release);
    
    // ADPCM management
    void SetADPCMPredictionTable(const SPU_ADPCM_Coefficient* table, uint8_t size);
    bool DecodeADPCMBlock(uint8_t voice_id, const uint8_t* block, 
                          std::vector<int16_t>& output);
    
    // Noise generation
    void SetNoiseFrequency(uint8_t voice_id, bool high_freq);
    void SetNoiseSeed(uint32_t seed);
    
    // Reverb effects
    void SetReverbEnabled(bool enabled);
    void SetReverbParams(const SPU_Reverb_Params& params);
    void ProcessReverb(const int16_t* input_left, const int16_t* input_right,
                       int16_t* output_left, int16_t* output_right, size_t samples);
    
    // Audio rendering
    void RenderAudio(int16_t* output_left, int16_t* output_right, 
                     uint32_t samples_requested);
    void RenderAudio(SPU_Audio_Buffer& buffer, uint32_t samples_requested);
    
    // Capture functions
    bool StartCapture(uint32_t address, uint32_t size);
    bool StopCapture();
    bool IsCapturing() const { return m_capturing; }
    bool GetCapturedData(uint8_t* buffer, uint32_t* size);
    
    // DMA operations
    void SetDMACallback(SPU_DMASampleCallback callback);
    bool TransferDMA(uint32_t spu_addr, uint8_t* external_buf, uint32_t size, bool to_spu);
    
    // Interrupt handling
    void SetInterruptCallback(SPU_InterruptCallback callback);
    void SetVoiceInterrupt(uint8_t voice_id, bool enabled);
    void GenerateInterrupt(SPU_InterruptType type, uint8_t param = 0);
    
    // Status and debugging
    uint32_t GetCurrentSampleCounter() const { return m_sample_counter; }
    bool IsVoiceActive(uint8_t voice_id) const;
    void DumpRegisters() const;
    float GetCPUUsage() const { return m_cpu_usage.load(); }
    
private:
    // Internal rendering methods
    void RenderVoice(uint8_t voice_id, int16_t* out_left, int16_t* out_right, 
                     uint32_t samples, uint32_t& mix_count);
    void RenderVoiceNormal(uint8_t voice_id, int16_t* out_left, int16_t* out_right, 
                           uint32_t samples, uint32_t& mix_count);
    void RenderVoiceADPCM(uint8_t voice_id, int16_t* out_left, int16_t* out_right, 
                          uint32_t samples, uint32_t& mix_count);
    void RenderVoiceNoise(uint8_t voice_id, int16_t* out_left, int16_t* out_right, 
                          uint32_t samples, uint32_t& mix_count);
    
    // Envelope processing
    uint16_t UpdateADSR(SPU_Voice_Registers& reg, uint32_t samples_processed);
    uint16_t ComputeADSRVolume(uint8_t state, uint16_t current, uint8_t rate, 
                               uint8_t sustain_level);
    
    // Sample interpolation
    int16_t InterpolateSample(const uint8_t* ram_base, uint32_t addr, uint8_t pitch);
    int16_t InterpolateADPCM(const SPU_Voice_Registers& reg, uint32_t phase);
    
    // Noise generation
    uint32_t GenerateNoise();
    
    // Reverb processing
    void ProcessReverbMono(const int16_t* input, int16_t* output, int16_t* buffer, 
                           size_t samples);
    
    // Internal state
    std::array<uint8_t, SPU_RAM_SIZE> m_spu_ram;
    std::array<SPU_Voice_Registers, SPU_VOICE_COUNT> m_voices;
    std::array<bool, SPU_VOICE_COUNT> m_voice_interrupt_enabled;
    
    std::vector<int16_t> m_scratch_left;
    std::vector<int16_t> m_scratch_right;
    
    std::array<SPU_ADPCM_Coefficient, 8> m_adpcm_coef_table;
    std::array<int16_t, SPU_REVERB_BUFFER_SIZE> m_reverb_buffer;
    size_t m_reverb_write_pos;
    SPU_Reverb_Params m_reverb_params;
    
    std::unique_ptr<uint8_t[]> m_capture_buffer;
    uint32_t m_capture_address;
    uint32_t m_capture_size;
    uint32_t m_capture_pos;
    std::atomic<bool> m_capturing;
    
    uint32_t m_sample_rate;
    uint32_t m_sample_counter;
    std::atomic<float> m_cpu_usage;
    std::mutex m_mutex;
    bool m_initialized;
    
    // Noise LFSR state
    uint32_t m_noise_seed;
    
    // Callbacks
    SPU_InterruptCallback m_interrupt_cb;
    SPU_DMASampleCallback m_dma_cb;
    
    // Internal helpers
    uint32_t WrapRAMAddress(uint32_t addr) const { return addr & (SPU_RAM_SIZE - 1); }
    uint8_t ReadRAMByte(uint32_t addr) const;
    void WriteRAMByte(uint32_t addr, uint8_t val);
    uint16_t ReadRAMWord(uint32_t addr) const;
    void WriteRAMWord(uint32_t addr, uint16_t val);
    
    void MixSample(int16_t& output, int16_t sample, uint8_t volume, bool clipping);
    void ApplyVolume(int16_t& sample, uint8_t volume);
};

} // namespace arenax3
} // namespace com

#endif // SPU_RUNTIME_HPP