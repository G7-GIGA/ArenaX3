#pragma once

#include <memory>
#include <string>
#include <atomic>
#include "Platform_Utils/Utils/Types.hpp"
#include "Platform_Utils/Utils/Logger.hpp"

namespace arena {

// Forward declarations for modular components
class PPU_Interpreter;
class SPU_Runtime;
class Memory_Manager;
class Vulkan_Renderer;

// ============================================================================
// Emulator State
// ============================================================================
enum class EmulatorState : u32 {
    UNINITIALIZED = 0,
    INITIALIZED,
    RUNNING,
    PAUSED,
    STOPPED,
    ERROR
};

// ============================================================================
// Emulator Class (Singleton)
// ============================================================================
class Emulator {
public:
    // Singleton instance access
    static Emulator& getInstance();
    
    // Delete copy/move constructors
    Emulator(const Emulator&) = delete;
    Emulator& operator=(const Emulator&) = delete;
    
    // Lifecycle methods
    bool Initialize();
    void Shutdown();
    void Stop();
    
    // Game loading
    bool LoadExecutable(const std::string& path);
    
    // State management
    EmulatorState getState() const { return m_state; }
    bool isRunning() const { return m_state == EmulatorState::RUNNING; }
    
    // Component accessors (for future implementation)
    PPU_Interpreter* getPPU() const { return m_ppu.get(); }
    SPU_Runtime* getSPU() const { return m_spu.get(); }
    Memory_Manager* getMemory() const { return m_memory.get(); }
    Vulkan_Renderer* getGPU() const { return m_gpu.get(); }
    
    // Performance metrics
    f64 getAverageFPS() const { return m_averageFPS; }
    u64 getTotalInstructions() const { return m_totalInstructions; }
    
private:
    // Private constructor for singleton
    Emulator() = default;
    ~Emulator() = default;
    
    // Internal methods
    void reset();
    bool initComponents();
    void cleanupComponents();
    void updatePerformanceMetrics(f64 deltaTime);
    
    // Component pointers (placeholders - will be implemented later)
    std::unique_ptr<PPU_Interpreter> m_ppu;
    std::unique_ptr<SPU_Runtime> m_spu;
    std::unique_ptr<Memory_Manager> m_memory;
    std::unique_ptr<Vulkan_Renderer> m_gpu;
    
    // State
    std::atomic<EmulatorState> m_state{EmulatorState::UNINITIALIZED};
    std::string m_currentGame;
    
    // Performance tracking
    std::chrono::steady_clock::time_point m_lastFrameTime;
    f64 m_averageFPS{0.0};
    u64 m_totalInstructions{0};
    u32 m_frameCount{0};
};

} // namespace arena