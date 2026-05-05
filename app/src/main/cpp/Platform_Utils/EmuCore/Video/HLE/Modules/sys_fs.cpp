// sys_fs.cpp
// ArenaX3 - File System Management System
// Copyright (c) 2024 ArenaX3 Team

#include "sys_fs.h"
#include <cstring>
#include <ctime>
#include <algorithm>

namespace com {
namespace arenax3 {

// FileSystem Implementation
FileSystem::FileSystem() : m_initialized(false), m_currentPath("/") {
    memset(&m_stats, 0, sizeof(FileSystemStats));
}

FileSystem::~FileSystem() {
    if (m_initialized) {
        Shutdown();
    }
}

bool FileSystem::Initialize(const char* rootPath) {
    if (m_initialized) {
        return false;
    }
    
    m_rootPath = rootPath;
    m_currentPath = "/";
    m_initialized = true;
    m_stats.totalOpenFiles = 0;
    m_stats.totalOperations = 0;
    m_stats.totalBytesRead = 0;
    m_stats.totalBytesWritten = 0;
    
    return true;
}

void FileSystem::Shutdown() {
    if (!m_initialized) {
        return;
    }
    
    // Close all open files
    for (auto& pair : m_openFiles) {
        if (pair.second) {
            fclose(pair.second);
        }
    }
    m_openFiles.clear();
    
    m_initialized = false;
}

bool FileSystem::Mount(const char* device, const char* mountPoint) {
    if (!m_initialized) return false;
    
    MountInfo info;
    info.device = device;
    info.mountPoint = mountPoint;
    info.type = "native";
    m_mounts[mountPoint] = info;
    
    return true;
}

bool FileSystem::Unmount(const char* mountPoint) {
    if (!m_initialized) return false;
    
    auto it = m_mounts.find(mountPoint);
    if (it != m_mounts.end()) {
        m_mounts.erase(it);
        return true;
    }
    return false;
}

FileHandle FileSystem::Open(const char* path, uint32_t flags) {
    if (!m_initialized) return INVALID_HANDLE;
    
    std::string fullPath = ResolvePath(path);
    const char* mode = nullptr;
    
    switch (flags & 0x03) {
        case FILE_READ:
            mode = "rb";
            break;
        case FILE_WRITE:
            mode = "wb";
            break;
        case FILE_APPEND:
            mode = "ab";
            break;
        case FILE_READ_WRITE:
            mode = "r+b";
            break;
        default:
            return INVALID_HANDLE;
    }
    
    FILE* file = fopen(fullPath.c_str(), mode);
    if (!file) {
        return INVALID_HANDLE;
    }
    
    FileHandle handle = ++m_nextHandle;
    m_openFiles[handle] = file;
    m_stats.totalOpenFiles++;
    m_stats.totalOperations++;
    
    return handle;
}

bool FileSystem::Close(FileHandle handle) {
    if (!m_initialized) return false;
    
    auto it = m_openFiles.find(handle);
    if (it == m_openFiles.end()) {
        return false;
    }
    
    int result = fclose(it->second);
    m_openFiles.erase(it);
    m_stats.totalOpenFiles--;
    m_stats.totalOperations++;
    
    return (result == 0);
}

size_t FileSystem::Read(FileHandle handle, void* buffer, size_t size) {
    if (!m_initialized) return 0;
    
    auto it = m_openFiles.find(handle);
    if (it == m_openFiles.end()) {
        return 0;
    }
    
    size_t bytesRead = fread(buffer, 1, size, it->second);
    m_stats.totalBytesRead += bytesRead;
    m_stats.totalOperations++;
    
    return bytesRead;
}

size_t FileSystem::Write(FileHandle handle, const void* buffer, size_t size) {
    if (!m_initialized) return 0;
    
    auto it = m_openFiles.find(handle);
    if (it == m_openFiles.end()) {
        return 0;
    }
    
    size_t bytesWritten = fwrite(buffer, 1, size, it->second);
    m_stats.totalBytesWritten += bytesWritten;
    m_stats.totalOperations++;
    
    return bytesWritten;
}

bool FileSystem::Seek(FileHandle handle, int64_t offset, SeekOrigin origin) {
    if (!m_initialized) return false;
    
    auto it = m_openFiles.find(handle);
    if (it == m_openFiles.end()) {
        return false;
    }
    
    int whence;
    switch (origin) {
        case SEEK_SET: whence = SEEK_SET; break;
        case SEEK_CUR: whence = SEEK_CUR; break;
        case SEEK_END: whence = SEEK_END; break;
        default: return false;
    }
    
    int result = fseek(it->second, offset, whence);
    m_stats.totalOperations++;
    
    return (result == 0);
}

int64_t FileSystem::Tell(FileHandle handle) {
    if (!m_initialized) return -1;
    
    auto it = m_openFiles.find(handle);
    if (it == m_openFiles.end()) {
        return -1;
    }
    
    return ftell(it->second);
}

bool FileSystem::Exists(const char* path) {
    if (!m_initialized) return false;
    
    std::string fullPath = ResolvePath(path);
    FILE* file = fopen(fullPath.c_str(), "rb");
    if (file) {
        fclose(file);
        return true;
    }
    return false;
}

bool FileSystem::Delete(const char* path) {
    if (!m_initialized) return false;
    
    std::string fullPath = ResolvePath(path);
    m_stats.totalOperations++;
    
    return (remove(fullPath.c_str()) == 0);
}

bool FileSystem::Rename(const char* oldPath, const char* newPath) {
    if (!m_initialized) return false;
    
    std::string fullOldPath = ResolvePath(oldPath);
    std::string fullNewPath = ResolvePath(newPath);
    m_stats.totalOperations++;
    
    return (rename(fullOldPath.c_str(), fullNewPath.c_str()) == 0);
}

bool FileSystem::CreateDirectory(const char* path) {
    if (!m_initialized) return false;
    
    std::string fullPath = ResolvePath(path);
    m_stats.totalOperations++;
    
#ifdef _WIN32
    return (_mkdir(fullPath.c_str()) == 0);
#else
    return (mkdir(fullPath.c_str(), 0755) == 0);
#endif
}

bool FileSystem::RemoveDirectory(const char* path) {
    if (!m_initialized) return false;
    
    std::string fullPath = ResolvePath(path);
    m_stats.totalOperations++;
    
#ifdef _WIN32
    return (_rmdir(fullPath.c_str()) == 0);
#else
    return (rmdir(fullPath.c_str()) == 0);
#endif
}

std::vector<std::string> FileSystem::ListDirectory(const char* path) {
    std::vector<std::string> result;
    
    if (!m_initialized) return result;
    
    std::string fullPath = ResolvePath(path);
    // Implementation would use opendir/readdir on POSIX or FindFirstFile/FindNextFile on Windows
    // Simplified for example
    
    return result;
}

std::string FileSystem::GetCurrentDirectory() const {
    return m_currentPath;
}

bool FileSystem::SetCurrentDirectory(const char* path) {
    if (!m_initialized) return false;
    
    std::string testPath = ResolvePath(path);
    // Verify directory exists (simplified)
    m_currentPath = path;
    return true;
}

std::string FileSystem::ResolvePath(const std::string& path) const {
    if (path.empty()) {
        return m_rootPath + m_currentPath;
    }
    
    if (path[0] == '/') {
        // Absolute path
        return m_rootPath + path;
    } else {
        // Relative path
        return m_rootPath + m_currentPath + "/" + path;
    }
}

FileSystemStats FileSystem::GetStats() const {
    return m_stats;
}

void FileSystem::ResetStats() {
    m_stats.totalOperations = 0;
    m_stats.totalBytesRead = 0;
    m_stats.totalBytesWritten = 0;
}

// File Implementation
File::File() : m_fs(nullptr), m_handle(INVALID_HANDLE), m_isOpen(false) {}

File::File(FileSystem* fs, const char* path, uint32_t flags) 
    : m_fs(fs), m_handle(INVALID_HANDLE), m_isOpen(false) {
    Open(fs, path, flags);
}

File::~File() {
    if (m_isOpen) {
        Close();
    }
}

bool File::Open(FileSystem* fs, const char* path, uint32_t flags) {
    if (!fs || m_isOpen) return false;
    
    m_fs = fs;
    m_handle = m_fs->Open(path, flags);
    
    if (m_handle != INVALID_HANDLE) {
        m_isOpen = true;
        return true;
    }
    
    return false;
}

bool File::Close() {
    if (!m_isOpen || !m_fs) return false;
    
    bool result = m_fs->Close(m_handle);
    if (result) {
        m_isOpen = false;
        m_handle = INVALID_HANDLE;
    }
    
    return result;
}

size_t File::Read(void* buffer, size_t size) {
    if (!m_isOpen || !m_fs) return 0;
    return m_fs->Read(m_handle, buffer, size);
}

size_t File::Write(const void* buffer, size_t size) {
    if (!m_isOpen || !m_fs) return 0;
    return m_fs->Write(m_handle, buffer, size);
}

bool File::Seek(int64_t offset, SeekOrigin origin) {
    if (!m_isOpen || !m_fs) return false;
    return m_fs->Seek(m_handle, offset, origin);
}

int64_t File::Tell() const {
    if (!m_isOpen || !m_fs) return -1;
    return m_fs->Tell(m_handle);
}

bool File::IsOpen() const {
    return m_isOpen;
}

} // namespace arenax3
} // namespace com