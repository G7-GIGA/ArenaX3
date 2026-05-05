// Virtual_Memory.hpp
// ArenaX3 Virtual Memory Management System
// Copyright (c) 2024 ArenaX3 Project. All rights reserved.

#ifndef ARENAX3_VIRTUAL_MEMORY_HPP
#define ARENAX3_VIRTUAL_MEMORY_HPP

#include <cstddef>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <optional>

namespace com {
namespace arenax3 {

// Page size configuration (4KB default)
constexpr size_t DEFAULT_PAGE_SIZE = 4096;

// Memory protection flags
enum class MemoryProtection : uint32_t {
    NONE      = 0,
    READ      = 1 << 0,
    WRITE     = 1 << 1,
    EXECUTE   = 1 << 2,
    READ_WRITE = READ | WRITE,
    READ_EXEC = READ | EXECUTE,
    RWX       = READ | WRITE | EXECUTE
};

// Page status flags
enum class PageStatus : uint8_t {
    FREE,
    ALLOCATED,
    RESERVED,
    COMMITTED,
    GUARD
};

// Virtual address structure
struct VirtualAddress {
    uint64_t value;
    
    VirtualAddress() : value(0) {}
    explicit VirtualAddress(uint64_t addr) : value(addr) {}
    
    operator uint64_t() const { return value; }
    bool operator==(const VirtualAddress& other) const { return value == other.value; }
    
    size_t page_index(size_t page_size) const {
        return static_cast<size_t>(value / page_size);
    }
    
    size_t offset(size_t page_size) const {
        return static_cast<size_t>(value % page_size);
    }
};

// Memory page information
struct PageInfo {
    VirtualAddress address;
    size_t size;
    PageStatus status;
    MemoryProtection protection;
    bool is_present;
    bool is_dirty;
    bool is_swapped;
    
    PageInfo() : size(0), status(PageStatus::FREE), protection(MemoryProtection::NONE),
                 is_present(false), is_dirty(false), is_swapped(false) {}
};

// Page table entry
struct PageTableEntry {
    uint64_t physical_frame : 40;
    uint64_t present : 1;
    uint64_t writable : 1;
    uint64_t user_accessible : 1;
    uint64_t write_through : 1;
    uint64_t cache_disabled : 1;
    uint64_t accessed : 1;
    uint64_t dirty : 1;
    uint64_t global : 1;
    uint64_t reserved : 16;
    
    PageTableEntry() : physical_frame(0), present(0), writable(0), user_accessible(0),
                       write_through(0), cache_disabled(0), accessed(0), dirty(0),
                       global(0), reserved(0) {}
    
    uint64_t get_physical_address() const {
        return physical_frame * DEFAULT_PAGE_SIZE;
    }
    
    void set_physical_address(uint64_t phys_addr) {
        physical_frame = phys_addr / DEFAULT_PAGE_SIZE;
    }
};

// Virtual memory region
struct MemoryRegion {
    VirtualAddress base;
    size_t size;
    MemoryProtection protection;
    PageStatus status;
    
    MemoryRegion() : size(0), protection(MemoryProtection::NONE), status(PageStatus::FREE) {}
    
    VirtualAddress end() const {
        return VirtualAddress(base.value + size);
    }
    
    bool contains(VirtualAddress addr) const {
        return addr.value >= base.value && addr.value < base.value + size;
    }
    
    bool overlaps(const MemoryRegion& other) const {
        return !(end().value <= other.base.value || other.end().value <= base.value);
    }
};

// Virtual memory manager class
class VirtualMemoryManager {
public:
    VirtualMemoryManager(size_t page_size = DEFAULT_PAGE_SIZE);
    ~VirtualMemoryManager();
    
    // Disable copy
    VirtualMemoryManager(const VirtualMemoryManager&) = delete;
    VirtualMemoryManager& operator=(const VirtualMemoryManager&) = delete;
    
    // Memory allocation
    std::optional<VirtualAddress> allocate_region(size_t size, MemoryProtection protection);
    std::optional<VirtualAddress> allocate_at_address(VirtualAddress address, size_t size, MemoryProtection protection);
    bool deallocate_region(VirtualAddress address);
    
    // Page operations
    bool commit_pages(VirtualAddress address, size_t num_pages);
    bool decommit_pages(VirtualAddress address, size_t num_pages);
    bool protect_pages(VirtualAddress address, size_t num_pages, MemoryProtection protection);
    
    // Page table operations
    bool map_page(VirtualAddress virtual_addr, uint64_t physical_addr, MemoryProtection protection);
    bool unmap_page(VirtualAddress virtual_addr);
    std::optional<uint64_t> get_physical_address(VirtualAddress virtual_addr);
    
    // Memory operations
    bool read_memory(VirtualAddress address, void* buffer, size_t size);
    bool write_memory(VirtualAddress address, const void* buffer, size_t size);
    
    // Query operations
    std::optional<PageInfo> query_page(VirtualAddress address) const;
    std::vector<MemoryRegion> get_regions() const;
    size_t get_total_committed_pages() const;
    size_t get_total_reserved_pages() const;
    
    // Page fault handling
    bool handle_page_fault(VirtualAddress fault_address, MemoryProtection access_type);
    
    // Swap operations
    bool swap_out_page(VirtualAddress address);
    bool swap_in_page(VirtualAddress address);
    
    // Flush operations
    void flush_tlb();
    void flush_region(VirtualAddress address, size_t size);
    
private:
    const size_t page_size_;
    const size_t page_shift_;
    const size_t page_mask_;
    
    std::unordered_map<VirtualAddress, PageTableEntry> page_table_;
    std::vector<MemoryRegion> regions_;
    std::unordered_map<VirtualAddress, std::vector<uint8_t>> swap_space_;
    
    mutable std::mutex mtx_;
    
    VirtualAddress next_free_address_;
    
    // Helper methods
    bool is_page_aligned(VirtualAddress address) const;
    size_t align_to_page_size(size_t size) const;
    VirtualAddress align_to_page_boundary(VirtualAddress address) const;
    bool validate_region(VirtualAddress address, size_t size) const;
    bool validate_protection_change(VirtualAddress address, size_t num_pages, MemoryProtection new_protection) const;
    bool update_page_protection(VirtualAddress virtual_addr, MemoryProtection protection);
    
    // Platform-specific operations
    bool platform_map_pages(VirtualAddress virtual_addr, uint64_t physical_addr, size_t num_pages, MemoryProtection protection);
    bool platform_unmap_pages(VirtualAddress virtual_addr, size_t num_pages);
    bool platform_protect_pages(VirtualAddress virtual_addr, size_t num_pages, MemoryProtection protection);
    uint64_t platform_allocate_physical_frame();
    void platform_free_physical_frame(uint64_t physical_addr);
    
    // Internal allocation helpers
    std::optional<MemoryRegion> find_free_region(size_t size);
    void merge_free_regions();
    void split_region(const MemoryRegion& region, VirtualAddress split_point);
};

} // namespace arenax3
} // namespace com

#endif // ARENAX3_VIRTUAL_MEMORY_HPP