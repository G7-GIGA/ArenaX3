#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace com::arenax3 {

class PPU_Interpreter {
public:
    // Registers
    enum class Register : uint8_t {
        PC    = 0x00,  // Program Counter
        SP    = 0x01,  // Stack Pointer
        FLAGS = 0x02,  // Status Flags
        R0    = 0x03,
        R1    = 0x04,
        R2    = 0x05,
        R3    = 0x06,
        R4    = 0x07,
        R5    = 0x08,
        R6    = 0x09,
        R7    = 0x0A,
        R8    = 0x0B,
        R9    = 0x0C,
        R10   = 0x0D,
        R11   = 0x0E,
        R12   = 0x0F,
        R13   = 0x10,
        R14   = 0x11,
        R15   = 0x12
    };

    // PPU Control Registers
    enum class ControlRegister : uint8_t {
        CONTROL   = 0x00,  // PPU Master Control
        STATUS    = 0x01,  // Status Register
        SCANLINE  = 0x02,  // Current Scanline
        CYCLE     = 0x03,  // Current Cycle
        FRAME     = 0x04,  // Frame Counter
        INTERRUPT = 0x05,  // Interrupt Control
        PALETTE   = 0x06,  // Palette Index
        LAYER_EN  = 0x07,  // Layer Enable
        BG_SCROLL = 0x08,  // Background Scroll
        SPRITE_ADDR = 0x09, // Sprite Table Address
        BG_ADDR    = 0x0A,  // Background Table Address
        DISPLAY_W  = 0x0B,  // Display Width
        DISPLAY_H  = 0x0C   // Display Height
    };

    // Status Flags
    enum class Flag : uint8_t {
        ZERO    = 0x01,  // Zero flag
        CARRY   = 0x02,  // Carry flag
        NEG     = 0x04,  // Negative flag
        OVERFLOW = 0x08,  // Overflow flag
        INTERRUPT = 0x10, // Interrupt flag
        HBLANK   = 0x20,  // Horizontal blank
        VBLANK   = 0x40   // Vertical blank
    };

    // PPU Layers
    enum class Layer : uint8_t {
        BG0 = 0x01,
        BG1 = 0x02,
        SPRITE0 = 0x04,
        SPRITE1 = 0x08
    };

    // Pixel format
    struct Color {
        uint8_t r, g, b, a;
        Color(uint8_t red = 0, uint8_t green = 0, uint8_t blue = 0, uint8_t alpha = 255)
            : r(red), g(green), b(blue), a(alpha) {}
    };

    // Scanline callback type
    using ScanlineCallback = std::function<void(int line, const Color* pixels, int width)>;

public:
    PPU_Interpreter();
    ~PPU_Interpreter();

    // Core execution
    void Reset();
    void Execute(uint32_t instructions);
    void Step();
    void RunFrame();

    // Memory access
    uint8_t ReadByte(uint16_t address);
    void WriteByte(uint16_t address, uint8_t value);
    uint16_t ReadWord(uint16_t address);
    void WriteWord(uint16_t address, uint16_t value);

    // Register access
    uint32_t GetRegister(Register reg) const;
    void SetRegister(Register reg, uint32_t value);
    bool GetFlag(Flag flag) const;
    void SetFlag(Flag flag, bool value);

    // Control register access
    uint8_t ReadControlReg(ControlRegister reg);
    void WriteControlReg(ControlRegister reg, uint8_t value);

    // PPU operations
    void RenderScanline(int line, Color* output);
    void UpdateBackground();
    void UpdateSprites();
    void SetPaletteColor(uint8_t index, const Color& color);
    Color GetPaletteColor(uint8_t index) const;

    // Framebuffer access
    const Color* GetFramebuffer() const;
    int GetWidth() const { return m_displayWidth; }
    int GetHeight() const { return m_displayHeight; }

    // Callbacks
    void SetScanlineCallback(ScanlineCallback callback);
    void SetVBlankCallback(std::function<void()> callback);

    // Interrupts
    void TriggerInterrupt(uint8_t interruptId);
    void ClearInterrupt(uint8_t interruptId);

private:
    // Instruction decoding and execution
    void DecodeAndExecute(uint8_t opcode);
    
    // Arithmetic operations
    void ADD(uint8_t dest, uint8_t src);
    void SUB(uint8_t dest, uint8_t src);
    void MUL(uint8_t dest, uint8_t src);
    void DIV(uint8_t dest, uint8_t src);
    void AND(uint8_t dest, uint8_t src);
    void OR(uint8_t dest, uint8_t src);
    void XOR(uint8_t dest, uint8_t src);
    void SHL(uint8_t dest, uint8_t amount);
    void SHR(uint8_t dest, uint8_t amount);
    
    // Data movement
    void MOV(uint8_t dest, uint8_t src);
    void LOAD(uint8_t dest, uint16_t address);
    void STORE(uint16_t address, uint8_t src);
    void PUSH(uint8_t reg);
    void POP(uint8_t reg);
    
    // Control flow
    void JMP(uint16_t address);
    void JMP_COND(uint16_t address, Flag condition);
    void CALL(uint16_t address);
    void RET();
    void IRET();
    void NOP();
    void HALT();
    
    // DMA operations
    void DMA_Transfer(uint16_t source, uint16_t dest, uint16_t count);
    
    // PPU drawing operations
    void DrawPixel(int x, int y, uint8_t colorIndex, Layer layer);
    void DrawRectangle(int x, int y, int w, int h, uint8_t colorIndex);
    void DrawSprite(int x, int y, int spriteId, uint8_t paletteId);
    void DrawText(int x, int y, const std::string& text, uint8_t colorIndex);
    
    // Internal helpers
    void UpdateFlags(uint32_t result, bool updateCarry = false, uint32_t carryIn = 0);
    void HandleVBlank();
    void HandleHBlank();
    void UpdateScanline();

private:
    // Registers
    uint32_t m_registers[19];  // R0-R15 + PC, SP, FLAGS
    
    // Control registers
    uint8_t m_controlRegs[13];
    
    // Memory
    std::vector<uint8_t> m_vram;      // Video RAM (64KB)
    std::vector<uint8_t> m_oam;       // Object Attribute Memory (1KB)
    std::vector<uint8_t> m_cgram;     // Color RAM (512 bytes)
    std::vector<uint8_t> m_systemRam; // System RAM (64KB)
    
    // Framebuffer
    std::vector<Color> m_framebuffer;
    std::vector<Color> m_backgroundBuffer;
    std::vector<Color> m_spriteBuffer;
    
    // PPU state
    uint16_t m_currentScanline;
    uint16_t m_currentCycle;
    uint32_t m_frameCounter;
    uint8_t m_interruptFlags;
    uint8_t m_layerEnable;
    uint8_t m_paletteCache[256];
    
    // Rendering parameters
    int m_displayWidth;
    int m_displayHeight;
    uint16_t m_bgScrollX;
    uint16_t m_bgScrollY;
    uint16_t m_spriteTableAddr;
    uint16_t m_bgTableAddr;
    
    // Color palette
    Color m_palette[256];
    
    // Execution state
    bool m_halted;
    bool m_running;
    uint32_t m_cycleCount;
    
    // Callbacks
    ScanlineCallback m_scanlineCallback;
    std::function<void()> m_vBlankCallback;
    std::vector<std::function<void()>> m_interruptHandlers;
};

} // namespace com::arenax3