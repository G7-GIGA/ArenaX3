#pragma once

#include <cstdint>
#include <cstddef>

namespace arena {

// ============================================================================
// Fixed-width integer types (PS3 compatibility)
// ============================================================================
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

typedef float    f32;
typedef double   f64;

// ============================================================================
// Boolean type
// ============================================================================
typedef bool     b8;

// ============================================================================
// PS3 Specific Types
// ============================================================================
typedef u32      be32_t;   // Big-endian 32-bit
typedef u64      be64_t;   // Big-endian 64-bit
typedef u32      vm_addr_t; // Virtual memory address (PS3: 32-bit)

// ============================================================================
// ArenaX Core Types
// ============================================================================
using byte   = u8;
using word   = u16;
using dword  = u32;
using qword  = u64;

// ============================================================================
// Alignment Macros
// ============================================================================
#define ALIGN(x, a)          (((x) + ((a) - 1)) & ~((a) - 1))
#define IS_ALIGNED(x, a)     (((x) & ((a) - 1)) == 0)
#define PAGE_SIZE            4096
#define PAGE_MASK            (~(PAGE_SIZE - 1))

// ============================================================================
// PS3 Memory Regions
// ============================================================================
namespace memory {
    constexpr vm_addr_t MAIN_RAM_BASE   = 0x00000000;
    constexpr vm_addr_t MAIN_RAM_SIZE   = 0x20000000;  // 512 MB
    constexpr vm_addr_t RSX_MAP_BASE    = 0x40000000;
    constexpr vm_addr_t RSX_MAP_SIZE    = 0x10000000;  // 256 MB
    constexpr vm_addr_t SPU_LS_BASE     = 0x30000000;
    constexpr vm_addr_t SPU_LS_SIZE     = 0x10000000;  // 256 MB
    constexpr vm_addr_t USER_MEM_BASE   = 0x10000000;
    constexpr vm_addr_t USER_MEM_SIZE   = 0x0F000000;  // 240 MB
}

// ============================================================================
// PPU Registers Structure
// ============================================================================
struct PPU_Registers {
    u64 gpr[32];    // General Purpose Registers (R0-R31)
    u64 lr;         // Link Register
    u64 ctr;        // Count Register
    u32 cr;         // Condition Register (8 fields of 4 bits)
    u32 xer;        // Fixed-Point Exception Register
    u32 fpscr;      // Floating-Point Status/Control Register
    u64 vscr;       // Vector Status/Control Register
    u64 vrsave;     // Vector Register Save/Restore
    u64 cia;        // Current Instruction Address
    u64 nia;        // Next Instruction Address
    
    PPU_Registers() {
        reset();
    }
    
    void reset() {
        for (int i = 0; i < 32; i++) gpr[i] = 0;
        lr = ctr = cia = nia = 0;
        cr = xer = fpscr = 0;
        vscr = vrsave = 0;
    }
};

// ============================================================================
// SPU Registers Structure
// ============================================================================
struct SPU_Registers {
    u32 gpr[128][4];   // 128x 128-bit registers (stored as 4x32)
    u32 pc;            // Program Counter
    u32 event_mask;
    u32 event_stat;
    u32 spu_id;
    
    SPU_Registers() {
        reset();
    }
    
    void reset() {
        for (int i = 0; i < 128; i++) {
            for (int j = 0; j < 4; j++) {
                gpr[i][j] = 0;
            }
        }
        pc = event_mask = event_stat = spu_id = 0;
    }
};

// ============================================================================
// Helper Functions
// ============================================================================
inline u32 byteswap32(u32 val) {
    return ((val >> 24) & 0xFF) |
           ((val >> 8)  & 0xFF00) |
           ((val << 8)  & 0xFF0000) |
           ((val << 24) & 0xFF000000);
}

inline u64 byteswap64(u64 val) {
    return ((val >> 56) & 0xFF) |
           ((val >> 40) & 0xFF00) |
           ((val >> 24) & 0xFF0000) |
           ((val >> 8)  & 0xFF000000) |
           ((val << 8)  & 0xFF00000000) |
           ((val << 24) & 0xFF0000000000) |
           ((val << 40) & 0xFF000000000000) |
           ((val << 56) & 0xFF00000000000000);
}

// Endian conversion for PS3 (big-endian to host)
inline u32 be32_to_host(be32_t val) {
    return byteswap32(val);
}

inline be32_t host_to_be32(u32 val) {
    return byteswap32(val);
}

} // namespace arena