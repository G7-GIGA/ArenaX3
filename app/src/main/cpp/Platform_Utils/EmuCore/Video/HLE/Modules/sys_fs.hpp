#ifndef SYS_FS_HPP
#define SYS_FS_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace com::arenax3 {

// File open modes
enum class FileMode : uint32_t {
    READ      = 0x0001,
    WRITE     = 0x0002,
    APPEND    = 0x0004,
    CREATE    = 0x0008,
    TRUNCATE  = 0x0010,
    EXCLUSIVE = 0x0020
};

// File seek origins
enum class SeekWhence : uint32_t {
    SET = 0,
    CUR = 1,
    END = 2
};

// File permissions
enum class FilePerm : uint32_t {
    OWNER_READ   = 0400,
    OWNER_WRITE  = 0200,
    OWNER_EXEC   = 0100,
    GROUP_READ   = 0040,
    GROUP_WRITE  = 0020,
    GROUP_EXEC   = 0010,
    OTHER_READ   = 0004,
    OTHER_WRITE  = 0002,
    OTHER_EXEC   = 0001,
    DEFAULT      = 0644
};

// File system entry types
enum class EntryType : uint32_t {
    UNKNOWN = 0,
    FILE    = 1,
    DIR     = 2,
    SYMLINK = 3,
    FIFO    = 4,
    SOCKET  = 5,
    CHARDEV = 6,
    BLOCKDEV = 7
};

// File attributes
struct FileAttributes {
    uint64_t size;
    uint64_t blocks;
    uint64_t atime;  // access time (seconds since epoch)
    uint64_t mtime;  // modify time
    uint64_t ctime;  // status change time
    uint32_t uid;
    uint32_t gid;
    uint16_t mode;
    EntryType type;
    uint32_t link_count;
};

// Directory entry
struct DirEntry {
    std::string name;
    uint64_t inode;
    EntryType type;
    uint64_t size;
};

// File system statistics
struct FsStats {
    uint64_t total_bytes;
    uint64_t free_bytes;
    uint64_t available_bytes;
    uint64_t total_inodes;
    uint64_t free_inodes;
    uint64_t block_size;
    uint32_t name_max;
};

// File descriptor handle type
using FileHandle = int32_t;
constexpr FileHandle INVALID_HANDLE = -1;

class SysFS {
public:
    // File operations
    static FileHandle open(const std::string& path, uint32_t mode, uint32_t perms = static_cast<uint32_t>(FilePerm::DEFAULT));
    static int32_t close(FileHandle fd);
    static int64_t read(FileHandle fd, void* buffer, uint64_t count);
    static int64_t write(FileHandle fd, const void* buffer, uint64_t count);
    static int64_t seek(FileHandle fd, int64_t offset, SeekWhence whence);
    static int64_t tell(FileHandle fd);
    static int32_t sync(FileHandle fd);
    static int32_t datasync(FileHandle fd);
    
    // Directory operations
    static int32_t mkdir(const std::string& path, uint32_t perms = static_cast<uint32_t>(FilePerm::DEFAULT));
    static int32_t rmdir(const std::string& path);
    static int32_t opendir(const std::string& path);
    static int32_t closedir(int32_t dir_fd);
    static int32_t readdir(int32_t dir_fd, DirEntry& entry);
    static int32_t rewinddir(int32_t dir_fd);
    static int32_t seekdir(int32_t dir_fd, uint64_t pos);
    static uint64_t telldir(int32_t dir_fd);
    
    // File metadata
    static int32_t stat(const std::string& path, FileAttributes& attr);
    static int32_t lstat(const std::string& path, FileAttributes& attr);
    static int32_t fstat(FileHandle fd, FileAttributes& attr);
    static int32_t access(const std::string& path, uint32_t mode);
    static int32_t chmod(const std::string& path, uint32_t mode);
    static int32_t fchmod(FileHandle fd, uint32_t mode);
    static int32_t chown(const std::string& path, uint32_t uid, uint32_t gid);
    static int32_t fchown(FileHandle fd, uint32_t uid, uint32_t gid);
    
    // Link operations
    static int32_t link(const std::string& oldpath, const std::string& newpath);
    static int32_t symlink(const std::string& target, const std::string& linkpath);
    static int32_t readlink(const std::string& path, std::string& target, uint64_t max_size);
    static int32_t unlink(const std::string& path);
    static int32_t rename(const std::string& oldpath, const std::string& newpath);
    
    // Extended operations
    static int32_t truncate(const std::string& path, uint64_t size);
    static int32_t ftruncate(FileHandle fd, uint64_t size);
    static int32_t statfs(const std::string& path, FsStats& stats);
    static int32_t statvfs(const std::string& path, FsStats& stats);
    
    // Working directory
    static int32_t getcwd(std::string& path, uint64_t max_size);
    static int32_t chdir(const std::string& path);
    static int32_t fchdir(FileHandle fd);
    
    // File descriptor operations
    static FileHandle dup(FileHandle old_fd);
    static FileHandle dup2(FileHandle old_fd, FileHandle new_fd);
    static int32_t fcntl_getfl(FileHandle fd);
    static int32_t fcntl_setfl(FileHandle fd, int32_t flags);
    
    // Utility functions
    static bool exists(const std::string& path);
    static bool is_file(const std::string& path);
    static bool is_directory(const std::string& path);
    static bool is_symlink(const std::string& path);
    static int64_t file_size(const std::string& path);
};

} // namespace com::arenax3

#endif // SYS_FS_HPP