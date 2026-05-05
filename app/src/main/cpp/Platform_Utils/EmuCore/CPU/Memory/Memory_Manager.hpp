#ifndef MEMORY_MANAGER_HPP
#define MEMORY_MANAGER_HPP

#include <cstdint>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>

namespace arena {

class MemoryManager {
public:
    // Mmap constants
    static constexpr size_t RAM_SIZE  = 256 * 1024 * 1024;  // 256 MB
    static constexpr size_t VRAM_SIZE = 256 * 1024 * 1024;  // 256 MB

    // Constructor / Destructor
    MemoryManager();
    ~MemoryManager();

    // Deleted copy/move to protect mmap resources
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;
    MemoryManager(MemoryManager&&) = delete;
    MemoryManager& operator=(MemoryManager&&) = delete;

    // Write functions (Big-Endian)
    void write8(uint32_t address, uint8_t value);
    void write16(uint32_t address, uint16_t value);
    void write32(uint32_t address, uint32_t value);
    void write64(uint32_t address, uint64_t value);

    // Read functions (Big-Endian)
    uint8_t  read8(uint32_t address) const;
    uint16_t read16(uint32_t address) const;
    uint32_t read32(uint32_t address) const;
    uint64_t read64(uint32_t address) const;

    // Direct access to mapped memory (use with caution)
    uint8_t* get_ram_ptr();
    uint8_t* get_vram_ptr();

    const uint8_t* get_ram_ptr() const;
    const uint8_t* get_vram_ptr() const;

    // Bounds checking
    bool is_ram_address(uint32_t address) const;
    bool is_vram_address(uint32_t address) const;

private:
    uint8_t* ram;   // Mapped RAM
    uint8_t* vram;  // Mapped VRAM

    // Helper to resolve address to appropriate memory region
    uint8_t* resolve_address(uint32_t address);
    const uint8_t* resolve_address(uint32_t address) const;

    // Helper to validate alignment
    void check_alignment(uint32_t address, size_t size) const;
};

} // namespace arena

#endif // MEMORY_MANAGER_HPP