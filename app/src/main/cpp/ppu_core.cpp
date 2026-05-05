/*
 * PACKAGE: com.arena
 * FILE: PPU_Core.cpp
 * TASK: Full Implementation & Opcode Fix
 */

#include "PPU_Core.h"
#include <cstdio>
#include <cstring>
#include <inttypes.h>

// Register definitions
#define GPR_COUNT 32
#define FPR_COUNT 32

// Condition Register bit masks
#define CR0_SHIFT 0
#define CR1_SHIFT 4
#define CR2_SHIFT 8
#define CR3_SHIFT 12
#define CR4_SHIFT 16
#define CR5_SHIFT 20
#define CR6_SHIFT 24
#define CR7_SHIFT 28

// Exception vectors
#define EXCEPTION_SYSTEM_RESET 0x100
#define EXCEPTION_MACHINE_CHECK 0x200
#define EXCEPTION_DSI 0x300
#define EXCEPTION_ISI 0x400
#define EXCEPTION_EXTERNAL 0x500
#define EXCEPTION_ALIGNMENT 0x600
#define EXCEPTION_PROGRAM 0x700
#define EXCEPTION_FPU_UNAVAILABLE 0x800
#define EXCEPTION_DECREMENTER 0x900
#define EXCEPTION_SYSTEM_CALL 0xC00
#define EXCEPTION_TRACE 0xD00
#define EXCEPTION_FPU_ASSIST 0xE00

class PPU_Core {
private:
    // Registers
    uint64_t gpr[GPR_COUNT];      // General Purpose Registers
    uint64_t fpr[FPR_COUNT];      // Floating Point Registers
    uint64_t pc;                   // Program Counter
    uint64_t lr;                   // Link Register
    uint64_t ctr;                  // Count Register
    uint32_t cr;                   // Condition Register
    uint32_t xer;                  // Fixed-Point Exception Register
    uint32_t fpscr;                // Floating Point Status/Control Register
    
    // Special Purpose Registers (SPRs)
    uint64_t srr0;                 // Machine Status Save/Restore Register 0
    uint64_t srr1;                 // Machine Status Save/Restore Register 1
    uint64_t dar;                  // Data Address Register
    uint32_t dsisr;                // Data Storage Interrupt Status Register
    
    // Memory
    uint8_t* memory;
    size_t memory_size;
    
    // Execution control
    bool running;
    bool trace_enabled;
    
    // Helper functions
    uint32_t read_instruction(uint64_t address) {
        if (address + 3 >= memory_size) {
            printf("ERROR: Instruction fetch at 0x%016" PRIx64 " exceeds memory bounds\n", address);
            return 0;
        }
        return (memory[address] << 24) | (memory[address + 1] << 16) | 
               (memory[address + 2] << 8) | memory[address + 3];
    }
    
    uint64_t read_uint64(uint64_t address) {
        if (address + 7 >= memory_size) {
            printf("ERROR: 64-bit read at 0x%016" PRIx64 " exceeds memory bounds\n", address);
            return 0;
        }
        return ((uint64_t)memory[address] << 56) | ((uint64_t)memory[address + 1] << 48) |
               ((uint64_t)memory[address + 2] << 40) | ((uint64_t)memory[address + 3] << 32) |
               ((uint64_t)memory[address + 4] << 24) | ((uint64_t)memory[address + 5] << 16) |
               ((uint64_t)memory[address + 6] << 8) | memory[address + 7];
    }
    
    void write_uint64(uint64_t address, uint64_t value) {
        if (address + 7 >= memory_size) {
            printf("ERROR: 64-bit write at 0x%016" PRIx64 " exceeds memory bounds\n", address);
            return;
        }
        memory[address] = (value >> 56) & 0xFF;
        memory[address + 1] = (value >> 48) & 0xFF;
        memory[address + 2] = (value >> 40) & 0xFF;
        memory[address + 3] = (value >> 32) & 0xFF;
        memory[address + 4] = (value >> 24) & 0xFF;
        memory[address + 5] = (value >> 16) & 0xFF;
        memory[address + 6] = (value >> 8) & 0xFF;
        memory[address + 7] = value & 0xFF;
    }
    
    void update_cr0(uint64_t result) {
        // Update CR0 based on result
        cr &= ~0xF0000000; // Clear CR0 bits
        if (result == 0) cr |= (1 << (31 - CR0_SHIFT));
        if (result >> 63) cr |= (1 << (31 - (CR0_SHIFT + 1)));
        // LT, GT, SO bits would be set based on XER etc.
    }
    
    void handle_exception(uint32_t vector, uint64_t srr1_value) {
        srr0 = pc;
        srr1 = srr1_value;
        pc = vector;
        printf("EXCEPTION: Vector 0x%03X, PC=0x%016" PRIx64 "\n", vector, srr0);
    }
    
public:
    PPU_Core(uint8_t* mem, size_t mem_size) : memory(mem), memory_size(mem_size) {
        reset();
    }
    
    void reset() {
        memset(gpr, 0, sizeof(gpr));
        memset(fpr, 0, sizeof(fpr));
        pc = 0x100;  // PowerPC reset vector
        lr = 0;
        ctr = 0;
        cr = 0;
        xer = 0;
        fpscr = 0;
        srr0 = 0;
        srr1 = 0;
        dar = 0;
        dsisr = 0;
        running = true;
        trace_enabled = false;
    }
    
    void execute_instruction(uint32_t instr) {
        uint32_t opcode = instr >> 26;  // Primary opcode (bits 0-5)
        uint32_t extended_op = instr & 0x3FF;  // For extended opcodes
        
        if (trace_enabled) {
            printf("TRACE: PC=0x%016" PRIx64 " Opcode=0x%08X\n", pc, instr);
        }
        
        // Main opcode dispatch
        switch (opcode) {
            case 0x10: // PowerPC opcode 0x10 - Multi-function group
                // Use sub-switch for extended opcode
                switch ((instr >> 1) & 0x3FF) {
                    case 0x00: // Add example
                        printf("ADD instruction at PC=0x%016" PRIx64 "\n", pc);
                        break;
                    case 0x01: // Subtract example
                        printf("SUB instruction at PC=0x%016" PRIx64 "\n", pc);
                        break;
                    default:
                        printf("UNKNOWN extended opcode 0x%03X under main 0x10 at PC=0x%016" PRIx64 "\n", 
                               extended_op, pc);
                        break;
                }
                break;
                
            case 0x11: // PowerPC opcode 0x11 - Multi-function group
                switch ((instr >> 1) & 0x3FF) {
                    case 0x02: // Multiply example
                        printf("MUL instruction at PC=0x%016" PRIx64 "\n", pc);
                        break;
                    case 0x03: // Divide example
                        printf("DIV instruction at PC=0x%016" PRIx64 "\n", pc);
                        break;
                    default:
                        printf("UNKNOWN extended opcode 0x%03X under main 0x11 at PC=0x%016" PRIx64 "\n", 
                               extended_op, pc);
                        break;
                }
                break;
                
            case 0x12: // PowerPC opcode 0x12 - Multi-function group
                switch ((instr >> 1) & 0x3FF) {
                    case 0x04: // AND example
                        printf("AND instruction at PC=0x%016" PRIx64 "\n", pc);
                        break;
                    case 0x05: // OR example
                        printf("OR instruction at PC=0x%016" PRIx64 "\n", pc);
                        break;
                    default:
                        printf("UNKNOWN extended opcode 0x%03X under main 0x12 at PC=0x%016" PRIx64 "\n", 
                               extended_op, pc);
                        break;
                }
                break;
                
            case 0x13: // System call
                printf("SYSCALL at PC=0x%016" PRIx64 "\n", pc);
                handle_exception(EXCEPTION_SYSTEM_CALL, 0);
                return;
                
            case 0x14: // Branch conditional
                execute_branch_conditional(instr);
                break;
                
            default:
                // Unknown opcode - program exception
                printf("UNKNOWN opcode 0x%02X at PC=0x%016" PRIx64 "\n", opcode, pc);
                handle_exception(EXCEPTION_PROGRAM, 0);
                break;
        }
        
        pc += 4;  // Move to next instruction
    }
    
    void execute_branch_conditional(uint32_t instr) {
        uint32_t bo = (instr >> 21) & 0x1F;  // Branch options
        uint32_t bi = (instr >> 16) & 0x1F;  // Condition register bit
        uint32_t bd = (instr & 0xFFFC);      // Branch displacement
        uint32_t lk = (instr >> 1) & 0x1;    // Link bit
        
        int64_t offset = (int64_t)(int16_t)bd;
        uint64_t target = pc + offset;
        
        // Check condition based on BO field
        bool condition_true = true;
        
        if ((bo & 0x10) == 0) {  // Check CR bit
            uint32_t cr_bit = (cr >> (31 - bi)) & 1;
            condition_true = ((bo & 0x08) != 0) ? cr_bit : !cr_bit;
        }
        
        if ((bo & 0x04) == 0) {  // Check counter
            condition_true = condition_true && (ctr != 0);
        }
        
        if (condition_true) {
            if (lk) {
                lr = pc + 4;  // Save return address
            }
            pc = target - 4;  // -4 because we'll add +4 after the call
            printf("BRANCH taken to 0x%016" PRIx64 ", LR=0x%016" PRIx64 "\n", target, lr);
        } else {
            printf("BRANCH not taken\n");
        }
    }
    
    // Load instructions
    void execute_lwz(uint32_t instr) { // Load Word and Zero
        uint32_t rd = (instr >> 21) & 0x1F;
        uint32_t ra = (instr >> 16) & 0x1F;
        int16_t imm = (int16_t)(instr & 0xFFFF);
        
        uint64_t ea = (ra ? gpr[ra] : 0) + imm;
        uint32_t val = read_uint64(ea) & 0xFFFFFFFF;
        gpr[rd] = val;
        
        printf("lwz r%d, %d(r%d) -> r%d=0x%08X, EA=0x%016" PRIx64 "\n", 
               rd, imm, ra, rd, val, ea);
    }
    
    void execute_ld(uint32_t instr) { // Load Doubleword
        uint32_t rd = (instr >> 21) & 0x1F;
        uint32_t ra = (instr >> 16) & 0x1F;
        int16_t imm = (int16_t)(instr & 0xFFFF);
        
        uint64_t ea = (ra ? gpr[ra] : 0) + imm;
        gpr[rd] = read_uint64(ea);
        
        printf("ld r%d, %d(r%d) -> r%d=0x%016" PRIx64 ", EA=0x%016" PRIx64 "\n", 
               rd, imm, ra, rd, gpr[rd], ea);
    }
    
    // Store instructions
    void execute_stw(uint32_t instr) { // Store Word
        uint32_t rs = (instr >> 21) & 0x1F;
        uint32_t ra = (instr >> 16) & 0x1F;
        int16_t imm = (int16_t)(instr & 0xFFFF);
        
        uint64_t ea = (ra ? gpr[ra] : 0) + imm;
        write_uint64(ea, gpr[rs] & 0xFFFFFFFF);
        
        printf("stw r%d, %d(r%d) -> Store 0x%08X at EA=0x%016" PRIx64 "\n", 
               rs, imm, ra, (uint32_t)gpr[rs], ea);
    }
    
    void execute_std(uint32_t instr) { // Store Doubleword
        uint32_t rs = (instr >> 21) & 0x1F;
        uint32_t ra = (instr >> 16) & 0x1F;
        int16_t imm = (int16_t)(instr & 0xFFFF);
        
        uint64_t ea = (ra ? gpr[ra] : 0) + imm;
        write_uint64(ea, gpr[rs]);
        
        printf("std r%d, %d(r%d) -> Store 0x%016" PRIx64 " at EA=0x%016" PRIx64 "\n", 
               rs, imm, ra, gpr[rs], ea);
    }
    
    // Fixed-point arithmetic
    void execute_add(uint32_t instr) { // Add
        uint32_t rd = (instr >> 21) & 0x1F;
        uint32_t ra = (instr >> 16) & 0x1F;
        uint32_t rb = (instr >> 11) & 0x1F;
        
        gpr[rd] = gpr[ra] + gpr[rb];
        update_cr0(gpr[rd]);
        
        printf("add r%d, r%d, r%d -> r%d=0x%016" PRIx64 "\n", 
               rd, ra, rb, rd, gpr[rd]);
    }
    
    void execute_and(uint32_t instr) { // AND
        uint32_t rd = (instr >> 21) & 0x1F;
        uint32_t ra = (instr >> 16) & 0x1F;
        uint32_t rb = (instr >> 11) & 0x1F;
        
        gpr[rd] = gpr[ra] & gpr[rb];
        update_cr0(gpr[rd]);
        
        printf("and r%d, r%d, r%d -> r%d=0x%016" PRIx64 "\n", 
               rd, ra, rb, rd, gpr[rd]);
    }
    
    // Main execution loop
    void run() {
        printf("PPU_Core execution started, PC=0x%016" PRIx64 "\n", pc);
        
        while (running) {
            // Check for interrupts/decrementer here
            
            if (pc >= memory_size || pc + 3 >= memory_size) {
                printf("ERROR: PC out of bounds (0x%016" PRIx64 "), halting\n", pc);
                break;
            }
            
            uint32_t instruction = read_instruction(pc);
            execute_instruction(instruction);
            
            // Add trace point checking here if needed
            // if (trace_enabled && pc == breakpoint) {}
        }
        
        printf("PPU_Core execution halted\n");
    }
    
    // Register access methods
    uint64_t get_gpr(int idx) const { return (idx < GPR_COUNT) ? gpr[idx] : 0; }
    void set_gpr(int idx, uint64_t val) { if (idx < GPR_COUNT) gpr[idx] = val; }
    
    uint64_t get_pc() const { return pc; }
    void set_pc(uint64_t new_pc) { pc = new_pc; }
    
    uint64_t get_lr() const { return lr; }
    void set_lr(uint64_t new_lr) { lr = new_lr; }
    
    uint64_t get_ctr() const { return ctr; }
    void set_ctr(uint64_t new_ctr) { ctr = new_ctr; }
    
    uint32_t get_cr() const { return cr; }
    void set_cr(uint32_t new_cr) { cr = new_cr; }
    
    uint32_t get_xer() const { return xer; }
    void set_xer(uint32_t new_xer) { xer = new_xer; }
    
    void enable_trace(bool enable) { trace_enabled = enable; }
    void halt() { running = false; }
};

// Example usage - this would be in a separate file normally
/*
int main() {
    // Create 256MB of memory
    const size_t MEM_SIZE = 256 * 1024 * 1024;
    uint8_t* memory = new uint8_t[MEM_SIZE];
    memset(memory, 0, MEM_SIZE);
    
    // Create PPU core
    PPU_Core ppu(memory, MEM_SIZE);
    
    // Add a simple test program at address 0x100
    // Example: nop (0x60000000) and trap (0x7C000008)
    memory[0x100] = 0x60; memory[0x101] = 0x00; memory[0x102] = 0x00; memory[0x103] = 0x00;
    memory[0x104] = 0x7C; memory[0x105] = 0x00; memory[0x106] = 0x00; memory[0x107] = 0x08;
    
    // Enable trace for debugging
    ppu.enable_trace(true);
    
    // Run the core
    ppu.run();
    
    delete[] memory;
    return 0;
}
*/