#pragma once

#include <android/log.h>
#include <string>
#include <cstring>

namespace arena {

// ============================================================================
// Log Levels
// ============================================================================
enum class LogLevel : u32 {
    VERBOSE = ANDROID_LOG_VERBOSE,
    DEBUG   = ANDROID_LOG_DEBUG,
    INFO    = ANDROID_LOG_INFO,
    WARN    = ANDROID_LOG_WARN,
    ERROR   = ANDROID_LOG_ERROR,
    FATAL   = ANDROID_LOG_FATAL
};

// ============================================================================
// Logger Class - Android Logcat Wrapper
// ============================================================================
class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    // Set minimum log level (messages below this will be ignored)
    void setMinLevel(LogLevel level) { m_minLevel = level; }
    
    // Get current log level
    LogLevel getMinLevel() const { return m_minLevel; }
    
    // Set log tag prefix for all messages
    void setTagPrefix(const char* prefix) { m_tagPrefix = prefix; }

    // Logging methods
    void log(LogLevel level, const char* tag, const char* format, ...) const;
    
    // Convenience macros (use these instead of direct calls)
    void verbose(const char* tag, const char* format, ...) const;
    void debug(const char* tag, const char* format, ...) const;
    void info(const char* tag, const char* format, ...) const;
    void warn(const char* tag, const char* format, ...) const;
    void error(const char* tag, const char* format, ...) const;
    void fatal(const char* tag, const char* format, ...) const;

private:
    Logger();
    ~Logger() = default;
    
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void logInternal(LogLevel level, const char* tag, const char* message) const;

private:
    LogLevel m_minLevel;
    const char* m_tagPrefix;
};

} // namespace arena

// ============================================================================
// Global Logging Macros (Easier to use)
// ============================================================================
#define ARENA_LOG_VERBOSE(tag, fmt, ...) \
    arena::Logger::getInstance().verbose(tag, fmt, ##__VA_ARGS__)

#define ARENA_LOG_DEBUG(tag, fmt, ...) \
    arena::Logger::getInstance().debug(tag, fmt, ##__VA_ARGS__)

#define ARENA_LOG_INFO(tag, fmt, ...) \
    arena::Logger::getInstance().info(tag, fmt, ##__VA_ARGS__)

#define ARENA_LOG_WARN(tag, fmt, ...) \
    arena::Logger::getInstance().warn(tag, fmt, ##__VA_ARGS__)

#define ARENA_LOG_ERROR(tag, fmt, ...) \
    arena::Logger::getInstance().error(tag, fmt, ##__VA_ARGS__)

#define ARENA_LOG_FATAL(tag, fmt, ...) \
    arena::Logger::getInstance().fatal(tag, fmt, ##__VA_ARGS__)

// Special macro for emulator core logging
#define ARENA_LOG_CORE(level, fmt, ...) \
    arena::Logger::getInstance().log(level, "ArenaX-Core", fmt, ##__VA_ARGS__)