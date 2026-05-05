/*
 * PACKAGE: com.arenax3
 * FILE: Memory_Manager.cpp
 * TASK: Memory Implementation for ArenaX3 Emulator
 */

#include "Memory_Manager.hpp"
#include <cstring>
#include <stdexcept>
#include <algorithm>

namespace com {
namespace arenax3 {

Memory_Manager::Memory_Manager(size_t memory_size) 
    : memory_size_(memory_size)
    , memory_(nullptr)
    , page_size_(4096)
    , initialized_(false) {
    if (memory_size > 0) {
        allocate_memory(memory_size);
    }
}

Memory_Manager::~Memory_Manager() {
    release_memory();
}

bool Memory_Manager::allocate_memory(size_t size) {
    if (initialized_) {
        release_memory();
    }
    
    memory_size_ = size;
    memory_ = new uint8_t[size];
    
    if (memory_) {
        std::memset(memory_, 0, size);
        initialized_ = true;
        return true;
    }
    
    return false;
}

void Memory_Manager::release_memory() {
    if (memory_) {
        delete[] memory_;
        memory_ = nullptr;
    }
    initialized_ = false;
}

void Memory_Manager::reset() {
    if (initialized_ && memory_) {
        std::memset(memory_, 0, memory_size_);
    }
}

uint8_t Memory_Manager::read8(uint32_t address) const {
    validate_address(address);
    return memory_[address];
}

uint16_t Memory_Manager::read16(uint32_t address) const {
    validate_address(address, 2);
    uint16_t value;
    std::memcpy(&value, &memory_[address], sizeof(value));
    return value;
}

uint32_t Memory_Manager::read32(uint32_t address) const {
    validate_address(address, 4);
    uint32_t value;
    std::memcpy(&value, &memory_[address], sizeof(value));
    return value;
}

uint64_t Memory_Manager::read64(uint32_t address) const {
    validate_address(address, 8);
    uint64_t value;
    std::memcpy(&value, &memory_[address], sizeof(value));
    return value;
}

void Memory_Manager::write8(uint32_t address, uint8_t value) {
    validate_address(address);
    memory_[address] = value;
}

void Memory_Manager::write16(uint32_t address, uint16_t value) {
    validate_address(address, 2);
    std::memcpy(&memory_[address], &value, sizeof(value));
}

void Memory_Manager::write32(uint32_t address, uint32_t value) {
    validate_address(address, 4);
    std::memcpy(&memory_[address], &value, sizeof(value));
}

void Memory_Manager::write64(uint32_t address, uint64_t value) {
    validate_address(address, 8);
    std::memcpy(&memory_[address], &value, sizeof(value));
}

void Memory_Manager::read_block(uint32_t address, void* buffer, size_t size) const {
    validate_address(address, size);
    std::memcpy(buffer, &memory_[address], size);
}

void Memory_Manager::write_block(uint32_t address, const void* buffer, size_t size) {
    validate_address(address, size);
    std::memcpy(&memory_[address], buffer, size);
}

uint8_t Memory_Manager::read8_big(uint32_t address) const {
    return read8(address);
}

uint16_t Memory_Manager::read16_big(uint32_t address) const {
    uint16_t value = read16(address);
    return byteswap16(value);
}

uint32_t Memory_Manager::read32_big(uint32_t address) const {
    uint32_t value = read32(address);
    return byteswap32(value);
}

uint64_t Memory_Manager::read64_big(uint32_t address) const {
    uint64_t value = read64(address);
    return byteswap64(value);
}

void Memory_Manager::write16_big(uint32_t address, uint16_t value) {
    write16(address, byteswap16(value));
}

void Memory_Manager::write32_big(uint32_t address, uint32_t value) {
    write32(address, byteswap32(value));
}

void Memory_Manager::write64_big(uint32_t address, uint64_t value) {
    write64(address, byteswap64(value));
}

uint16_t Memory_Manager::byteswap16(uint16_t value) const {
    return ((value & 0x00FF) << 8) | ((value & 0xFF00) >> 8);
}

uint32_t Memory_Manager::byteswap32(uint32_t value) const {
    return ((value & 0x000000FF) << 24) |
           ((value & 0x0000FF00) << 8)  |
           ((value & 0x00FF0000) >> 8)  |
           ((value & 0xFF000000) >> 24);
}

uint64_t Memory_Manager::byteswap64(uint64_t value) const {
    return ((value & 0x00000000000000FFULL) << 56) |
           ((value & 0x000000000000FF00ULL) << 40) |
           ((value & 0x0000000000FF0000ULL) << 24) |
           ((value & 0x00000000FF000000ULL) << 8)  |
           ((value & 0x000000FF00000000ULL) >> 8)  |
           ((value & 0x0000FF0000000000ULL) >> 24) |
           ((value & 0x00FF000000000000ULL) >> 40) |
           ((value & 0xFF00000000000000ULL) >> 56);
}

void Memory_Manager::validate_address(uint32_t address, size_t size) const {
    if (!initialized_ || !memory_) {
        throw std::runtime_error("Memory not initialized");
    }
    if (address + size > memory_size_) {
        throw std::out_of_range("Memory access out of bounds");
    }
}

void Memory_Manager::dump_memory(uint32_t address, size_t size) const {
    validate_address(address, size);
    
    for (size_t i = 0; i < size; i += 16) {
        fprintf(stderr, "%08X: ", address + (uint32_t)i);
        for (size_t j = 0; j < 16 && (i + j) < size; j++) {
            fprintf(stderr, "%02X ", memory_[address + i + j]);
        }
        fprintf(stderr, "\n");
    }
}

void Memory_Manager::fill_memory(uint32_t address, uint8_t value, size_t size) {
    validate_address(address, size);
    std::memset(&memory_[address], value, size);
}

void Memory_Manager::copy_memory(uint32_t dest, uint32_t src, size_t size) {
    validate_address(dest, size);
    validate_address(src, size);
    std::memmove(&memory_[dest], &memory_[src], size);
}

int8_t Memory_Manager::read_s8(uint32_t address) const {
    return static_cast<int8_t>(read8(address));
}

int16_t Memory_Manager::read_s16(uint32_t address) const {
    return static_cast<int16_t>(read16(address));
}

int32_t Memory_Manager::read_s32(uint32_t address) const {
    return static_cast<int32_t>(read32(address));
}

int64_t Memory_Manager::read_s64(uint32_t address) const {
    return static_cast<int64_t>(read64(address));
}

void Memory_Manager::write_s8(uint32_t address, int8_t value) {
    write8(address, static_cast<uint8_t>(value));
}

void Memory_Manager::write_s16(uint32_t address, int16_t value) {
    write16(address, static_cast<uint16_t>(value));
}

void Memory_Manager::write_s32(uint32_t address, int32_t value) {
    write32(address, static_cast<uint32_t>(value));
}

void Memory_Manager::write_s64(uint32_t address, int64_t value) {
    write64(address, static_cast<uint64_t>(value));
}

float Memory_Manager::read_float(uint32_t address) const {
    uint32_t raw = read32(address);
    float value;
    std::memcpy(&value, &raw, sizeof(value));
    return value;
}

double Memory_Manager::read_double(uint32_t address) const {
    uint64_t raw = read64(address);
    double value;
    std::memcpy(&value, &raw, sizeof(value));
    return value;
}

void Memory_Manager::write_float(uint32_t address, float value) {
    uint32_t raw;
    std::memcpy(&raw, &value, sizeof(raw));
    write32(address, raw);
}

void Memory_Manager::write_double(uint32_t address, double value) {
    uint64_t raw;
    std::memcpy(&raw, &value, sizeof(raw));
    write64(address, raw);
}

bool Memory_Manager::compare_memory(uint32_t address, const void* data, size_t size) const {
    validate_address(address, size);
    return std::memcmp(&memory_[address], data, size) == 0;
}

uint32_t Memory_Manager::search_memory(const void* pattern, size_t pattern_size, 
                                        uint32_t start, uint32_t end) const {
    if (end == 0 || end > memory_size_) {
        end = memory_size_;
    }
    
    if (start >= end || pattern_size == 0 || pattern_size > (end - start)) {
        return 0xFFFFFFFF;
    }
    
    const uint8_t* haystack = memory_ + start;
    size_t haystack_size = end - start;
    const uint8_t* needle = static_cast<const uint8_t*>(pattern);
    
    const uint8_t* result = std::search(haystack, haystack + haystack_size,
                                         needle, needle + pattern_size);
    
    if (result != haystack + haystack_size) {
        return static_cast<uint32_t>(result - memory_);
    }
    
    return 0xFFFFFFFF;
}

size_t Memory_Manager::get_memory_size() const {
    return memory_size_;
}

bool Memory_Manager::is_initialized() const {
    return initialized_;
}

} // namespace arenax3
} // namespace com