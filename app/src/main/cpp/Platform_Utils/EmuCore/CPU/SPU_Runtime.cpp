// SPU_Runtime.cpp
// SPU Runtime implementation for ArenaX3
// Namespace: com::arenax3

#include "SPU_Runtime.h"
#include <cstring>
#include <algorithm>

namespace com {
namespace arenax3 {

// ============================================================================
// SPUContext Implementation
// ============================================================================

SPUContext::SPUContext()
    : initialized(false)
    , running(false)
    , spu_id(0)
    , dma_tag_id(0)
{
    memset(&program_info, 0, sizeof(SPUProgramInfo));
    memset(&performance_stats, 0, sizeof(SPUPerformanceStats));
}

SPUContext::~SPUContext() {
    if (running) {
        stop();
    }
    if (initialized) {
        shutdown();
    }
}

bool SPUContext::initialize(uint32_t id, const SPUConfig& config) {
    if (initialized) {
        return false;
    }
    
    spu_id = id;
    spu_config = config;
    dma_tag_id = id;
    
    // Initialize local store memory (256KB typical for SPU)
    local_store.resize(config.local_store_size);
    memset(local_store.data(), 0, config.local_store_size);
    
    // Initialize register file
    registers.fill(0);
    
    // Initialize performance counters
    memset(&performance_stats, 0, sizeof(SPUPerformanceStats));
    performance_stats.spu_id = id;
    
    initialized = true;
    
    return true;
}

void SPUContext::shutdown() {
    if (!initialized) {
        return;
    }
    
    stop();
    local_store.clear();
    registers.fill(0);
    initialized = false;
}

bool SPUContext::load_program(const void* program_data, size_t program_size) {
    if (!initialized || !program_data || program_size == 0) {
        return false;
    }
    
    if (program_size > local_store.size()) {
        return false;
    }
    
    memcpy(local_store.data(), program_data, program_size);
    program_info.program_address = 0;
    program_info.program_size = program_size;
    program_info.is_loaded = true;
    
    return true;
}

bool SPUContext::start() {
    if (!initialized || !program_info.is_loaded || running) {
        return false;
    }
    
    running = true;
    program_info.is_running = true;
    performance_stats.start_time = get_current_time();
    
    return true;
}

void SPUContext::stop() {
    if (!running) {
        return;
    }
    
    running = false;
    program_info.is_running = false;
    performance_stats.end_time = get_current_time();
}

bool SPUContext::execute_dma_get(uint64_t ea, uint32_t ls_addr, uint32_t size) {
    if (!initialized || !running) {
        return false;
    }
    
    if (ls_addr + size > local_store.size()) {
        return false;
    }
    
    performance_stats.dma_transfers++;
    performance_stats.dma_bytes_transferred += size;
    
    // This would trigger actual DMA transfer in real hardware
    // For simulation, we mark the operation as pending
    
    return true;
}

bool SPUContext::execute_dma_put(uint64_t ea, uint32_t ls_addr, uint32_t size) {
    if (!initialized || !running) {
        return false;
    }
    
    if (ls_addr + size > local_store.size()) {
        return false;
    }
    
    performance_stats.dma_transfers++;
    performance_stats.dma_bytes_transferred += size;
    
    return true;
}

void SPUContext::write_register(uint32_t reg_index, uint128_t value) {
    if (reg_index < NUM_SPU_REGISTERS) {
        registers[reg_index] = value;
    }
}

uint128_t SPUContext::read_register(uint32_t reg_index) const {
    if (reg_index < NUM_SPU_REGISTERS) {
        return registers[reg_index];
    }
    return uint128_t{0};
}

void SPUContext::write_local_store(uint32_t offset, const void* data, uint32_t size) {
    if (offset + size <= local_store.size() && data) {
        memcpy(local_store.data() + offset, data, size);
    }
}

void SPUContext::read_local_store(uint32_t offset, void* data, uint32_t size) const {
    if (offset + size <= local_store.size() && data) {
        memcpy(data, local_store.data() + offset, size);
    }
}

SPUPerformanceStats SPUContext::get_performance_stats() const {
    SPUPerformanceStats stats = performance_stats;
    if (running) {
        stats.current_run_time = get_current_time() - stats.start_time;
    }
    return stats;
}

void SPUContext::reset_performance_stats() {
    memset(&performance_stats, 0, sizeof(SPUPerformanceStats));
    performance_stats.spu_id = spu_id;
    if (running) {
        performance_stats.start_time = get_current_time();
    }
}

double SPUContext::get_current_time() const {
    // Return current time in microseconds
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration<double, std::micro>(duration).count();
}

// ============================================================================
// SPURuntime Implementation
// ============================================================================

SPURuntime::SPURuntime()
    : initialized(false)
    , num_active_spus(0)
{
}

SPURuntime::~SPURuntime() {
    if (initialized) {
        shutdown();
    }
}

bool SPURuntime::initialize(const SPURuntimeConfig& config) {
    if (initialized) {
        return false;
    }
    
    runtime_config = config;
    contexts.resize(config.max_spu_count);
    
    for (uint32_t i = 0; i < config.max_spu_count; ++i) {
        contexts[i] = std::make_unique<SPUContext>();
        SPUConfig spu_config;
        spu_config.spu_id = i;
        spu_config.local_store_size = config.default_local_store_size;
        spu_config.dma_tag_id = i;
        contexts[i]->initialize(i, spu_config);
    }
    
    initialized = true;
    
    return true;
}

void SPURuntime::shutdown() {
    if (!initialized) {
        return;
    }
    
    for (auto& ctx : contexts) {
        if (ctx) {
            ctx->shutdown();
        }
    }
    
    contexts.clear();
    num_active_spus = 0;
    initialized = false;
}

bool SPURuntime::create_spu_context(uint32_t spu_id, const SPUConfig& config) {
    if (!initialized || spu_id >= runtime_config.max_spu_count) {
        return false;
    }
    
    if (contexts[spu_id]->is_initialized()) {
        return false;
    }
    
    contexts[spu_id]->initialize(spu_id, config);
    num_active_spus++;
    
    return true;
}

bool SPURuntime::destroy_spu_context(uint32_t spu_id) {
    if (!initialized || spu_id >= runtime_config.max_spu_count) {
        return false;
    }
    
    if (!contexts[spu_id]->is_initialized()) {
        return false;
    }
    
    if (contexts[spu_id]->is_running()) {
        contexts[spu_id]->stop();
    }
    
    contexts[spu_id]->shutdown();
    num_active_spus--;
    
    return true;
}

bool SPURuntime::load_program(uint32_t spu_id, const void* program_data, size_t program_size) {
    if (!initialized || spu_id >= runtime_config.max_spu_count) {
        return false;
    }
    
    return contexts[spu_id]->load_program(program_data, program_size);
}

bool SPURuntime::start_spu(uint32_t spu_id) {
    if (!initialized || spu_id >= runtime_config.max_spu_count) {
        return false;
    }
    
    return contexts[spu_id]->start();
}

bool SPURuntime::stop_spu(uint32_t spu_id) {
    if (!initialized || spu_id >= runtime_config.max_spu_count) {
        return false;
    }
    
    contexts[spu_id]->stop();
    return true;
}

bool SPURuntime::send_dma_get(uint32_t spu_id, uint64_t ea, uint32_t ls_addr, uint32_t size) {
    if (!initialized || spu_id >= runtime_config.max_spu_count) {
        return false;
    }
    
    return contexts[spu_id]->execute_dma_get(ea, ls_addr, size);
}

bool SPURuntime::send_dma_put(uint32_t spu_id, uint64_t ea, uint32_t ls_addr, uint32_t size) {
    if (!initialized || spu_id >= runtime_config.max_spu_count) {
        return false;
    }
    
    return contexts[spu_id]->execute_dma_put(ea, ls_addr, size);
}

bool SPURuntime::write_spu_register(uint32_t spu_id, uint32_t reg_index, uint128_t value) {
    if (!initialized || spu_id >= runtime_config.max_spu_count) {
        return false;
    }
    
    contexts[spu_id]->write_register(reg_index, value);
    return true;
}

uint128_t SPURuntime::read_spu_register(uint32_t spu_id, uint32_t reg_index) const {
    if (!initialized || spu_id >= runtime_config.max_spu_count) {
        return uint128_t{0};
    }
    
    return contexts[spu_id]->read_register(reg_index);
}

bool SPURuntime::write_local_store(uint32_t spu_id, uint32_t offset, const void* data, uint32_t size) {
    if (!initialized || spu_id >= runtime_config.max_spu_count) {
        return false;
    }
    
    contexts[spu_id]->write_local_store(offset, data, size);
    return true;
}

bool SPURuntime::read_local_store(uint32_t spu_id, uint32_t offset, void* data, uint32_t size) const {
    if (!initialized || spu_id >= runtime_config.max_spu_count) {
        return false;
    }
    
    contexts[spu_id]->read_local_store(offset, data, size);
    return true;
}

SPUPerformanceStats SPURuntime::get_spu_stats(uint32_t spu_id) const {
    if (!initialized || spu_id >= runtime_config.max_spu_count) {
        SPUPerformanceStats empty_stats{};
        empty_stats.spu_id = spu_id;
        return empty_stats;
    }
    
    return contexts[spu_id]->get_performance_stats();
}

void SPURuntime::reset_all_stats() {
    for (auto& ctx : contexts) {
        if (ctx && ctx->is_initialized()) {
            ctx->reset_performance_stats();
        }
    }
}

bool SPURuntime::is_spu_running(uint32_t spu_id) const {
    if (!initialized || spu_id >= runtime_config.max_spu_count) {
        return false;
    }
    
    return contexts[spu_id]->is_running();
}

uint32_t SPURuntime::get_active_spu_count() const {
    return num_active_spus;
}

void SPURuntime::wait_for_spu_completion(uint32_t spu_id) {
    if (!initialized || spu_id >= runtime_config.max_spu_count) {
        return;
    }
    
    while (contexts[spu_id]->is_running()) {
        // Yield or wait - platform dependent
        std::this_thread::yield();
    }
}

void SPURuntime::wait_for_all_spus() {
    for (uint32_t i = 0; i < runtime_config.max_spu_count; ++i) {
        if (contexts[i] && contexts[i]->is_initialized() && contexts[i]->is_running()) {
            wait_for_spu_completion(i);
        }
    }
}

} // namespace arenax3
} // namespace com