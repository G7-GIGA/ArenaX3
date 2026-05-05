#ifndef SYS_SAVE_HPP
#define SYS_SAVE_HPP

#include <cstdint>
#include <cstddef>

namespace com {
namespace arenax3 {

// Save data status codes
enum class SaveStatus : int32_t {
    SUCCESS = 0,
    ERROR_NO_STORAGE = -1,
    ERROR_READ_FAILED = -2,
    ERROR_WRITE_FAILED = -3,
    ERROR_DATA_CORRUPTED = -4,
    ERROR_INVALID_HANDLE = -5,
    ERROR_INSUFFICIENT_SPACE = -6,
    ERROR_NOT_INITIALIZED = -7
};

// Save data flags
enum class SaveFlags : uint32_t {
    NONE = 0,
    COMPRESSED = 1 << 0,
    ENCRYPTED = 1 << 1,
    AUTO_BACKUP = 1 << 2,
    CRITICAL = 1 << 3
};

// Save data handle type
using SaveHandle = void*;

// Save data metadata
struct SaveMetadata {
    char save_id[64];
    uint32_t version;
    uint64_t timestamp;
    uint64_t data_size;
    SaveFlags flags;
    uint32_t checksum;
};

// System call functions for save data management

// Initialize save data system
SaveStatus sys_save_initialize();

// Shutdown save data system
SaveStatus sys_save_shutdown();

// Open save data for reading/writing
SaveStatus sys_save_open(const char* save_id, SaveFlags flags, SaveHandle* out_handle);

// Close save data handle
SaveStatus sys_save_close(SaveHandle handle);

// Read data from save file
SaveStatus sys_save_read(SaveHandle handle, void* buffer, size_t size, size_t* bytes_read);

// Write data to save file
SaveStatus sys_save_write(SaveHandle handle, const void* buffer, size_t size, size_t* bytes_written);

// Seek within save file
SaveStatus sys_save_seek(SaveHandle handle, int64_t offset, int whence, uint64_t* new_position);

// Get current position in save file
SaveStatus sys_save_tell(SaveHandle handle, uint64_t* position);

// Get save file size
SaveStatus sys_save_get_size(SaveHandle handle, uint64_t* size);

// Flush pending writes to storage
SaveStatus sys_save_flush(SaveHandle handle);

// Delete save data by ID
SaveStatus sys_save_delete(const char* save_id);

// Get save metadata by ID
SaveStatus sys_save_get_metadata(const char* save_id, SaveMetadata* metadata);

// Set save metadata (version, flags, etc.)
SaveStatus sys_save_set_metadata(SaveHandle handle, const SaveMetadata* metadata);

// Check if save data exists
SaveStatus sys_save_exists(const char* save_id, bool* exists);

// Get available storage space in bytes
SaveStatus sys_save_get_free_space(uint64_t* free_bytes);

// Get total storage capacity
SaveStatus sys_save_get_total_space(uint64_t* total_bytes);

// List all save IDs (max_entries: size of buffer, returns actual count)
SaveStatus sys_save_list(char** save_id_buffer, uint32_t max_entries, uint32_t* actual_count);

// Verify save data integrity (checksum validation)
SaveStatus sys_save_verify(SaveHandle handle, bool* is_valid);

// Compact save data storage (defragmentation)
SaveStatus sys_save_compact();

// Create backup of save data
SaveStatus sys_save_create_backup(const char* save_id);

// Restore save data from backup
SaveStatus sys_save_restore_backup(const char* save_id);

// Export save data to external device
SaveStatus sys_save_export(const char* save_id, const char* export_path);

// Import save data from external device
SaveStatus sys_save_import(const char* save_id, const char* import_path);

// Check if save data is currently open
SaveStatus sys_save_is_open(SaveHandle handle, bool* is_open);

// Get last error message
const char* sys_save_get_last_error();

// Clear error state
void sys_save_clear_error();

} // namespace arenax3
} // namespace com

#endif // SYS_SAVE_HPP