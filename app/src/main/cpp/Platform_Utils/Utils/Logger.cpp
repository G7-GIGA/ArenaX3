#include "Logger.hpp"
#include <cstdarg>
#include <cstdio>

namespace arena {

Logger::Logger() 
    : m_minLevel(LogLevel::DEBUG)
    , m_tagPrefix("ArenaX") {
}

void Logger::log(LogLevel level, const char* tag, const char* format, ...) const {
    // Check if this log level should be filtered
    if (static_cast<int>(level) < static_cast<int>(m_minLevel)) {
        return;
    }
    
    // Format the message
    char buffer[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    // Build full tag with prefix
    char fullTag[256];
    if (tag && tag[0] != '\0') {
        snprintf(fullTag, sizeof(fullTag), "%s-%s", m_tagPrefix, tag);
    } else {
        snprintf(fullTag, sizeof(fullTag), "%s", m_tagPrefix);
    }
    
    // Send to Android logcat
    logInternal(level, fullTag, buffer);
}

void Logger::logInternal(LogLevel level, const char* tag, const char* message) const {
    switch (level) {
        case LogLevel::VERBOSE:
            __android_log_write(ANDROID_LOG_VERBOSE, tag, message);
            break;
        case LogLevel::DEBUG:
            __android_log_write(ANDROID_LOG_DEBUG, tag, message);
            break;
        case LogLevel::INFO:
            __android_log_write(ANDROID_LOG_INFO, tag, message);
            break;
        case LogLevel::WARN:
            __android_log_write(ANDROID_LOG_WARN, tag, message);
            break;
        case LogLevel::ERROR:
            __android_log_write(ANDROID_LOG_ERROR, tag, message);
            break;
        case LogLevel::FATAL:
            __android_log_write(ANDROID_LOG_FATAL, tag, message);
            break;
    }
}

void Logger::verbose(const char* tag, const char* format, ...) const {
    if (static_cast<int>(LogLevel::VERBOSE) < static_cast<int>(m_minLevel)) return;
    
    char buffer[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    char fullTag[256];
    snprintf(fullTag, sizeof(fullTag), "%s-%s", m_tagPrefix, tag ? tag : "");
    logInternal(LogLevel::VERBOSE, fullTag, buffer);
}

void Logger::debug(const char* tag, const char* format, ...) const {
    if (static_cast<int>(LogLevel::DEBUG) < static_cast<int>(m_minLevel)) return;
    
    char buffer[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    char fullTag[256];
    snprintf(fullTag, sizeof(fullTag), "%s-%s", m_tagPrefix, tag ? tag : "");
    logInternal(LogLevel::DEBUG, fullTag, buffer);
}

void Logger::info(const char* tag, const char* format, ...) const {
    if (static_cast<int>(LogLevel::INFO) < static_cast<int>(m_minLevel)) return;
    
    char buffer[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    char fullTag[256];
    snprintf(fullTag, sizeof(fullTag), "%s-%s", m_tagPrefix, tag ? tag : "");
    logInternal(LogLevel::INFO, fullTag, buffer);
}

void Logger::warn(const char* tag, const char* format, ...) const {
    if (static_cast<int>(LogLevel::WARN) < static_cast<int>(m_minLevel)) return;
    
    char buffer[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    char fullTag[256];
    snprintf(fullTag, sizeof(fullTag), "%s-%s", m_tagPrefix, tag ? tag : "");
    logInternal(LogLevel::WARN, fullTag, buffer);
}

void Logger::error(const char* tag, const char* format, ...) const {
    if (static_cast<int>(LogLevel::ERROR) < static_cast<int>(m_minLevel)) return;
    
    char buffer[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    char fullTag[256];
    snprintf(fullTag, sizeof(fullTag), "%s-%s", m_tagPrefix, tag ? tag : "");
    logInternal(LogLevel::ERROR, fullTag, buffer);
}

void Logger::fatal(const char* tag, const char* format, ...) const {
    if (static_cast<int>(LogLevel::FATAL) < static_cast<int>(m_minLevel)) return;
    
    char buffer[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    char fullTag[256];
    snprintf(fullTag, sizeof(fullTag), "%s-%s", m_tagPrefix, tag ? tag : "");
    logInternal(LogLevel::FATAL, fullTag, buffer);
}

} // namespace arena