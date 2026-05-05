#pragma once

#include <cstdint>
#include <vector>
#include <stdexcept>

namespace com {
namespace arenax3 {

class ARM64_Emitter {
public:
    ARM64_Emitter() = default;
    ~ARM64_Emitter() = default;

    // Get generated code
    const std::vector<uint8_t>& getCode() const { return m_code; }
    std::vector<uint8_t> releaseCode() { return std::move(m_code); }
    
    // Get current code size
    size_t getCodeSize() const { return m_code.size(); }
    
    // Clear generated code
    void clear() { m_code.clear(); }
    
    // Emit raw instruction
    void emit32(uint32_t instr) {
        m_code.push_back(instr & 0xFF);
        m_code.push_back((instr >> 8) & 0xFF);
        m_code.push_back((instr >> 16) & 0xFF);
        m_code.push_back((instr >> 24) & 0xFF);
    }
    
    // Patch instruction at offset
    void patch32(size_t offset, uint32_t instr) {
        if (offset + 4 > m_code.size()) {
            throw std::runtime_error("Patch out of bounds");
        }
        m_code[offset] = instr & 0xFF;
        m_code[offset + 1] = (instr >> 8) & 0xFF;
        m_code[offset + 2] = (instr >> 16) & 0xFF;
        m_code[offset + 3] = (instr >> 24) & 0xFF;
    }
    
    // --- Data Processing Immediate ---
    void movz(uint32_t reg, uint16_t imm, uint32_t shift = 0) {
        uint32_t instr = 0x52800000 | (reg) | ((imm & 0xFFFF) << 5) | ((shift / 16) << 21);
        emit32(instr);
    }
    
    void movk(uint32_t reg, uint16_t imm, uint32_t shift = 0) {
        uint32_t instr = 0x72800000 | (reg) | ((imm & 0xFFFF) << 5) | ((shift / 16) << 21);
        emit32(instr);
    }
    
    void movn(uint32_t reg, uint16_t imm, uint32_t shift = 0) {
        uint32_t instr = 0x92800000 | (reg) | ((imm & 0xFFFF) << 5) | ((shift / 16) << 21);
        emit32(instr);
    }
    
    // --- Move Register ---
    void mov(uint32_t rd, uint32_t rm) {
        orr(rd, 0x1F, rm); // MOV Rd, Rm = ORR Rd, XZR, Rm
    }
    
    // --- Add/Sub Immediate ---
    void add(uint32_t rd, uint32_t rn, uint32_t imm, uint32_t shift = 0) {
        uint32_t instr = 0x11000000 | (rd) | (rn << 5) | ((imm & 0xFFF) << 10) | ((shift == 12) << 22);
        emit32(instr);
    }
    
    void sub(uint32_t rd, uint32_t rn, uint32_t imm, uint32_t shift = 0) {
        uint32_t instr = 0x51000000 | (rd) | (rn << 5) | ((imm & 0xFFF) << 10) | ((shift == 12) << 22);
        emit32(instr);
    }
    
    void adds(uint32_t rd, uint32_t rn, uint32_t imm, uint32_t shift = 0) {
        uint32_t instr = 0x31000000 | (rd) | (rn << 5) | ((imm & 0xFFF) << 10) | ((shift == 12) << 22);
        emit32(instr);
    }
    
    void subs(uint32_t rd, uint32_t rn, uint32_t imm, uint32_t shift = 0) {
        uint32_t instr = 0x71000000 | (rd) | (rn << 5) | ((imm & 0xFFF) << 10) | ((shift == 12) << 22);
        emit32(instr);
    }
    
    // --- Add/Sub Register ---
    void add_reg(uint32_t rd, uint32_t rn, uint32_t rm, uint32_t extend = 0, uint32_t shift = 0) {
        uint32_t instr = 0x0B000000 | (rd) | (rn << 5) | (rm << 16) | (extend << 13) | (shift << 10);
        emit32(instr);
    }
    
    void sub_reg(uint32_t rd, uint32_t rn, uint32_t rm, uint32_t extend = 0, uint32_t shift = 0) {
        uint32_t instr = 0x4B000000 | (rd) | (rn << 5) | (rm << 16) | (extend << 13) | (shift << 10);
        emit32(instr);
    }
    
    // --- Logical Immediate ---
    void and_imm(uint32_t rd, uint32_t rn, uint32_t imm, uint32_t shift = 0) {
        uint32_t instr = 0x12000000 | (rd) | (rn << 5) | ((imm & 0xFFF) << 10) | ((shift & 0x3F) << 16);
        emit32(instr);
    }
    
    void orr(uint32_t rd, uint32_t rn, uint32_t rm) {
        uint32_t instr = 0x2A000000 | (rd) | (rn << 5) | (rm << 16);
        emit32(instr);
    }
    
    void eor(uint32_t rd, uint32_t rn, uint32_t rm) {
        uint32_t instr = 0x4A000000 | (rd) | (rn << 5) | (rm << 16);
        emit32(instr);
    }
    
    void and_reg(uint32_t rd, uint32_t rn, uint32_t rm) {
        uint32_t instr = 0x0A000000 | (rd) | (rn << 5) | (rm << 16);
        emit32(instr);
    }
    
    // --- Shift Operations ---
    void lsl(uint32_t rd, uint32_t rn, uint32_t shift) {
        add_reg(rd, rn, 0x1F, 0, shift); // LSL = ADD Rd, Rn, Rm, LSL shift with Rm = XZR
    }
    
    void lsr(uint32_t rd, uint32_t rn, uint32_t shift) {
        uint32_t instr = 0x4A000000 | (rd) | (rn << 5) | (0x1F << 16) | (1 << 22) | (shift << 10);
        emit32(instr);
    }
    
    void asr(uint32_t rd, uint32_t rn, uint32_t shift) {
        uint32_t instr = 0x4A000000 | (rd) | (rn << 5) | (0x1F << 16) | (2 << 22) | (shift << 10);
        emit32(instr);
    }
    
    // --- Load/Store Immediate ---
    void ldr(uint32_t rt, uint32_t rn, int32_t offset = 0) {
        if (offset >= 0 && offset < 4096) {
            uint32_t instr = 0x39400000 | (rt) | (rn << 5) | ((offset & 0xFFF) << 10);
            emit32(instr);
        } else if (offset < 0 && offset > -4096) {
            uint32_t instr = 0x39400000 | (rt) | (rn << 5) | ((-offset & 0xFFF) << 10);
            instr |= (1 << 24); // Pre-index with negative
            emit32(instr);
        } else {
            throw std::runtime_error("Offset out of range for LDR");
        }
    }
    
    void str(uint32_t rt, uint32_t rn, int32_t offset = 0) {
        if (offset >= 0 && offset < 4096) {
            uint32_t instr = 0x39000000 | (rt) | (rn << 5) | ((offset & 0xFFF) << 10);
            emit32(instr);
        } else if (offset < 0 && offset > -4096) {
            uint32_t instr = 0x39000000 | (rt) | (rn << 5) | ((-offset & 0xFFF) << 10);
            instr |= (1 << 24);
            emit32(instr);
        } else {
            throw std::runtime_error("Offset out of range for STR");
        }
    }
    
    void ldrh(uint32_t rt, uint32_t rn, uint32_t offset = 0) {
        uint32_t instr = 0x39400000 | (1 << 30) | (rt) | (rn << 5) | ((offset & 0xFFF) << 10);
        emit32(instr);
    }
    
    void strh(uint32_t rt, uint32_t rn, uint32_t offset = 0) {
        uint32_t instr = 0x39000000 | (1 << 30) | (rt) | (rn << 5) | ((offset & 0xFFF) << 10);
        emit32(instr);
    }
    
    void ldrb(uint32_t rt, uint32_t rn, uint32_t offset = 0) {
        uint32_t instr = 0x39400000 | (rt) | (rn << 5) | ((offset & 0xFFF) << 10);
        emit32(instr);
    }
    
    void strb(uint32_t rt, uint32_t rn, uint32_t offset = 0) {
        uint32_t instr = 0x39000000 | (rt) | (rn << 5) | ((offset & 0xFFF) << 10);
        emit32(instr);
    }
    
    // --- Load/Store Literal ---
    void ldr_lit(uint32_t rt, uint32_t label_id) {
        // PC-relative load, label_id is handled by fixup system
        uint32_t instr = 0x58000000 | (rt) | ((label_id & 0x1FFFFF) << 5);
        emit32(instr);
    }
    
    // --- Branch ---
    void b(uint32_t offset) {
        uint32_t instr = 0x14000000 | ((offset & 0x3FFFFFF) >> 2);
        emit32(instr);
    }
    
    void bl(uint32_t offset) {
        uint32_t instr = 0x94000000 | ((offset & 0x3FFFFFF) >> 2);
        emit32(instr);
    }
    
    void br(uint32_t rn) {
        uint32_t instr = 0xD61F0000 | (rn << 5);
        emit32(instr);
    }
    
    void blr(uint32_t rn) {
        uint32_t instr = 0xD63F0000 | (rn << 5);
        emit32(instr);
    }
    
    void ret(uint32_t rn = 0x1E) { // LR is 30, but we use 0x1E for x30
        uint32_t instr = 0xD65F0000 | (rn << 5);
        emit32(instr);
    }
    
    void nop() {
        emit32(0xD503201F);
    }
    
    // --- Conditional Branch ---
    void b_eq(uint32_t offset) {
        uint32_t instr = 0x54000000 | ((offset & 0x1FFFFF) << 3) | (0x0 << 24);
        emit32(instr);
    }
    
    void b_ne(uint32_t offset) {
        uint32_t instr = 0x54000000 | ((offset & 0x1FFFFF) << 3) | (0x1 << 24);
        emit32(instr);
    }
    
    void b_ge(uint32_t offset) {
        uint32_t instr = 0x54000000 | ((offset & 0x1FFFFF) << 3) | (0xA << 24);
        emit32(instr);
    }
    
    void b_lt(uint32_t offset) {
        uint32_t instr = 0x54000000 | ((offset & 0x1FFFFF) << 3) | (0xB << 24);
        emit32(instr);
    }
    
    void b_gt(uint32_t offset) {
        uint32_t instr = 0x54000000 | ((offset & 0x1FFFFF) << 3) | (0xC << 24);
        emit32(instr);
    }
    
    void b_le(uint32_t offset) {
        uint32_t instr = 0x54000000 | ((offset & 0x1FFFFF) << 3) | (0xD << 24);
        emit32(instr);
    }
    
    // --- Compare (CMP) ---
    void cmp_imm(uint32_t rn, uint32_t imm) {
        subs(0x1F, rn, imm);
    }
    
    void cmp_reg(uint32_t rn, uint32_t rm) {
        subs(0x1F, rn, rm);
    }
    
    // --- Load/Store Pair (for stack frames) ---
    void stp_pre(uint32_t rt1, uint32_t rt2, uint32_t rn, int32_t offset = -16) {
        if (offset >= -512 && offset <= 504 && offset % 16 == 0) {
            uint32_t imm = (-offset >> 3) & 0x7F;
            uint32_t instr = 0xA9800000 | (rt1) | (rt2 << 10) | (rn << 5) | (imm << 15);
            emit32(instr);
        } else {
            throw std::runtime_error("Offset out of range for STP pre-index");
        }
    }
    
    void ldp_post(uint32_t rt1, uint32_t rt2, uint32_t rn, int32_t offset = 16) {
        if (offset >= -512 && offset <= 504 && offset % 16 == 0) {
            uint32_t imm = (offset >> 3) & 0x7F;
            uint32_t instr = 0xA8C00000 | (rt1) | (rt2 << 10) | (rn << 5) | (imm << 15);
            emit32(instr);
        } else {
            throw std::runtime_error("Offset out of range for LDP post-index");
        }
    }
    
    // --- More complex loads/stores with register offset ---
    void ldr_reg(uint32_t rt, uint32_t rn, uint32_t rm, uint32_t extend = 0, uint32_t shift = 0) {
        uint32_t instr = 0x38600000 | (rt) | (rn << 5) | (rm << 16) | (extend << 13) | (shift << 12);
        emit32(instr);
    }
    
    void str_reg(uint32_t rt, uint32_t rn, uint32_t rm, uint32_t extend = 0, uint32_t shift = 0) {
        uint32_t instr = 0x38200000 | (rt) | (rn << 5) | (rm << 16) | (extend << 13) | (shift << 12);
        emit32(instr);
    }
    
private:
    std::vector<uint8_t> m_code;
};

} // namespace arenax3
} // namespace com