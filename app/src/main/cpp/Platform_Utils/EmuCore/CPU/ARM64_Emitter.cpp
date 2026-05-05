#include "ARM64_Emitter.h"
#include <cstring>
#include <algorithm>

namespace com {
namespace arenax3 {

ARM64_Emitter::ARM64_Emitter(uint8_t* buffer, size_t capacity)
    : m_buffer(buffer), m_capacity(capacity), m_position(0) {}

void ARM64_Emitter::Reset() {
    m_position = 0;
}

size_t ARM64_Emitter::GetPosition() const {
    return m_position;
}

bool ARM64_Emitter::SetPosition(size_t pos) {
    if (pos > m_capacity) return false;
    m_position = pos;
    return true;
}

uint8_t* ARM64_Emitter::GetBuffer() const {
    return m_buffer;
}

size_t ARM64_Emitter::GetCapacity() const {
    return m_capacity;
}

bool ARM64_Emitter::HasSpace(size_t bytes) const {
    return m_position + bytes <= m_capacity;
}

void ARM64_Emitter::Emit8(uint8_t value) {
    if (HasSpace(1)) {
        m_buffer[m_position++] = value;
    }
}

void ARM64_Emitter::Emit16(uint16_t value) {
    if (HasSpace(2)) {
        memcpy(m_buffer + m_position, &value, 2);
        m_position += 2;
    }
}

void ARM64_Emitter::Emit32(uint32_t value) {
    if (HasSpace(4)) {
        memcpy(m_buffer + m_position, &value, 4);
        m_position += 4;
    }
}

void ARM64_Emitter::Emit64(uint64_t value) {
    if (HasSpace(8)) {
        memcpy(m_buffer + m_position, &value, 8);
        m_position += 8;
    }
}

void ARM64_Emitter::EmitBytes(const uint8_t* data, size_t size) {
    if (HasSpace(size)) {
        memcpy(m_buffer + m_position, data, size);
        m_position += size;
    }
}

uint32_t ARM64_Emitter::EncodeR(uint8_t opcode, uint8_t Rd, uint8_t Rn, uint8_t Rm) {
    return (opcode << 24) | (Rm << 16) | (Rn << 8) | Rd;
}

uint32_t ARM64_Emitter::EncodeI(uint8_t opcode, uint8_t Rd, uint8_t Rn, uint16_t imm12) {
    return (opcode << 24) | (imm12 << 12) | (Rn << 8) | Rd;
}

uint32_t ARM64_Emitter::EncodeBranch(uint8_t opcode, int32_t offset) {
    uint32_t imm26 = static_cast<uint32_t>(offset >> 2) & 0x3FFFFFF;
    return (opcode << 24) | imm26;
}

uint32_t ARM64_Emitter::EnodeBranchReg(uint8_t opcode, uint8_t Rn) {
    return (opcode << 24) | (Rn << 8) | 0x1F;
}

void ARM64_Emitter::MOV(uint8_t Rd, uint16_t imm16) {
    uint32_t instr = 0x52800000 | (imm16 << 5) | Rd;
    Emit32(instr);
}

void ARM64_Emitter::MOVK(uint8_t Rd, uint16_t imm16, uint8_t shift) {
    uint32_t instr = 0x72800000 | ((shift / 16) << 21) | (imm16 << 5) | Rd;
    Emit32(instr);
}

void ARM64_Emitter::ADD(uint8_t Rd, uint8_t Rn, uint16_t imm12) {
    Emit32(EncodeI(0x11, Rd, Rn, imm12));
}

void ARM64_Emitter::SUB(uint8_t Rd, uint8_t Rn, uint16_t imm12) {
    Emit32(EncodeI(0x51, Rd, Rn, imm12));
}

void ARM64_Emitter::ADDS(uint8_t Rd, uint8_t Rn, uint16_t imm12) {
    Emit32(EncodeI(0x31, Rd, Rn, imm12));
}

void ARM64_Emitter::SUBS(uint8_t Rd, uint8_t Rn, uint16_t imm12) {
    Emit32(EncodeI(0x71, Rd, Rn, imm12));
}

void ARM64_Emitter::AND(uint8_t Rd, uint8_t Rn, uint16_t imm12) {
    Emit32(EncodeI(0x12, Rd, Rn, imm12));
}

void ARM64_Emitter::ORR(uint8_t Rd, uint8_t Rn, uint16_t imm12) {
    Emit32(EncodeI(0x32, Rd, Rn, imm12));
}

void ARM64_Emitter::EOR(uint8_t Rd, uint8_t Rn, uint16_t imm12) {
    Emit32(EncodeI(0x52, Rd, Rn, imm12));
}

void ARM64_Emitter::LDR(uint8_t Rt, uint8_t Rn, uint16_t imm12) {
    Emit32(EncodeI(0x39, Rt, Rn, imm12));
}

void ARM64_Emitter::STR(uint8_t Rt, uint8_t Rn, uint16_t imm12) {
    Emit32(EncodeI(0x79, Rt, Rn, imm12));
}

void ARM64_Emitter::LDRB(uint8_t Rt, uint8_t Rn, uint16_t imm12) {
    Emit32(EncodeI(0x39, Rt, Rn, imm12) | (1 << 30));
}

void ARM64_Emitter::STRB(uint8_t Rt, uint8_t Rn, uint16_t imm12) {
    Emit32(EncodeI(0x79, Rt, Rn, imm12) | (1 << 30));
}

void ARM64_Emitter::LDRH(uint8_t Rt, uint8_t Rn, uint16_t imm12) {
    Emit32(EncodeI(0x79, Rt, Rn, imm12) & ~(1 << 22));
}

void ARM64_Emitter::STRH(uint8_t Rt, uint8_t Rn, uint16_t imm12) {
    Emit32(EncodeI(0x79, Rt, Rn, imm12) | (1 << 30) | (1 << 22));
}

void ARM64_Emitter::ADDR(uint8_t Rd, uint8_t Rn, uint8_t Rm) {
    Emit32(EncodeR(0x8B, Rd, Rn, Rm));
}

void ARM64_Emitter::SUBR(uint8_t Rd, uint8_t Rn, uint8_t Rm) {
    Emit32(EncodeR(0x8B, Rd, Rn, Rm) | (1 << 30));
}

void ARM64_Emitter::MUL(uint8_t Rd, uint8_t Rn, uint8_t Rm) {
    Emit32(0x1B007C00 | (Rm << 16) | (Rn << 5) | Rd);
}

void ARM64_Emitter::SMULL(uint8_t Rd, uint8_t Rn, uint8_t Rm) {
    Emit32(0x9B207C00 | (Rm << 16) | (Rn << 5) | Rd);
}

void ARM64_Emitter::UMULL(uint8_t Rd, uint8_t Rn, uint8_t Rm) {
    Emit32(0x9BA07C00 | (Rm << 16) | (Rn << 5) | Rd);
}

void ARM64_Emitter::CMP(uint8_t Rn, uint16_t imm12) {
    Emit32(EncodeI(0x71, 0x1F, Rn, imm12));
}

void ARM64_Emitter::CMPR(uint8_t Rn, uint8_t Rm) {
    Emit32(EncodeR(0x6B, 0x1F, Rn, Rm));
}

void ARM64_Emitter::B(int32_t offset) {
    uint32_t imm26 = (static_cast<uint32_t>(offset >> 2) & 0x3FFFFFF);
    Emit32(0x14000000 | imm26);
}

void ARM64_Emitter::BL(int32_t offset) {
    uint32_t imm26 = (static_cast<uint32_t>(offset >> 2) & 0x3FFFFFF);
    Emit32(0x94000000 | imm26);
}

void ARM64_Emitter::BEQ(int32_t offset) {
    uint32_t imm19 = (static_cast<uint32_t>(offset >> 2) & 0x7FFFF);
    Emit32(0x54000000 | (imm19 << 5) | 0x0);
}

void ARM64_Emitter::BNE(int32_t offset) {
    uint32_t imm19 = (static_cast<uint32_t>(offset >> 2) & 0x7FFFF);
    Emit32(0x54000000 | (imm19 << 5) | 0x1);
}

void ARM64_Emitter::BLT(int32_t offset) {
    uint32_t imm19 = (static_cast<uint32_t>(offset >> 2) & 0x7FFFF);
    Emit32(0x54000000 | (imm19 << 5) | 0xB);
}

void ARM64_Emitter::BGT(int32_t offset) {
    uint32_t imm19 = (static_cast<uint32_t>(offset >> 2) & 0x7FFFF);
    Emit32(0x54000000 | (imm19 << 5) | 0xC);
}

void ARM64_Emitter::BLE(int32_t offset) {
    uint32_t imm19 = (static_cast<uint32_t>(offset >> 2) & 0x7FFFF);
    Emit32(0x54000000 | (imm19 << 5) | 0xD);
}

void ARM64_Emitter::BGE(int32_t offset) {
    uint32_t imm19 = (static_cast<uint32_t>(offset >> 2) & 0x7FFFF);
    Emit32(0x54000000 | (imm19 << 5) | 0xA);
}

void ARM64_Emitter::BR(uint8_t Rn) {
    Emit32(0xD61F0000 | (Rn << 5));
}

void ARM64_Emitter::BLR(uint8_t Rn) {
    Emit32(0xD63F0000 | (Rn << 5));
}

void ARM64_Emitter::RET(uint8_t Rn) {
    Emit32(0xD65F0000 | (Rn << 5));
}

void ARM64_Emitter::NOP() {
    Emit32(0xD503201F);
}

void ARM64_Emitter::HINT(uint8_t imm7) {
    Emit32(0xD503201F | ((imm7 & 0x7F) << 5));
}

void ARM64_Emitter::YIELD() {
    Emit32(0xD503209F);
}

void ARM64_Emitter::WFE() {
    Emit32(0xD503205F);
}

void ARM64_Emitter::WFI() {
    Emit32(0xD503207F);
}

void ARM64_Emitter::SEVL() {
    Emit32(0xD50320BF);
}

void ARM64_Emitter::DSB(uint8_t option) {
    Emit32(0xD503309F | ((option & 0xF) << 8));
}

void ARM64_Emitter::DMB(uint8_t option) {
    Emit32(0xD50330BF | ((option & 0xF) << 8));
}

void ARM64_Emitter::ISB() {
    Emit32(0xD50330DF);
}

} // namespace arenax3
} // namespace com