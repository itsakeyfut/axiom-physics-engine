#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace axiom::core {

/// Log severity levels
enum class LogLevel {
    Trace,    ///< Detailed diagnostic information
    Debug,    ///< Debug information
    Info,     ///< Informational messages
    Warning,  ///< Warning messages
    Error,    ///< Error messages
    Fatal     ///< Fatal errors (program termination)
};

/// Convert LogLevel to string representation
/// @param level The log level to convert
/// @return String representation of the log level
const char* logLevelToString(LogLevel level) noexcept;

/// Abstract interface for log output sinks
/// Sinks are responsible for writing log messages to a specific destination
/// (e.g., console, file, network)
class LogSink {
public:
    virtual ~LogSink() = default;

    /// Write a log message to the sink
    /// @param level The severity level of the message
    /// @param category The category/module name (e.g., "GPU", "Physics")
    /// @param message The formatted log message
    virtual void write(LogLevel level, const char* category, const char* message) = 0;

    /// Flush any buffered output
    virtual void flush() {}
};

/// Console log sink with ANSI color support
/// Writes log messages to stdout with color coding based on severity level
class ConsoleLogSink : public LogSink {
public:
    /// Constructor
    /// @param useColors Enable ANSI color codes (auto-detected by default)
    explicit ConsoleLogSink(bool useColors = true);

    void write(LogLevel level, const char* category, const char* message) override;
    void flush() override;

private:
    bool useColors_;
};

/// File log sink with optional rotation
/// Writes log messages to a file with optional size-based rotation
class FileLogSink : public LogSink {
public:
    /// Constructor
    /// @param filename Path to the log file
    /// @param maxFileSize Maximum file size before rotation (0 = no rotation)
    /// @param maxFiles Maximum number of rotated files to keep
    explicit FileLogSink(const std::string& filename, size_t maxFileSize = 0,
                         size_t maxFiles = 3);

    ~FileLogSink() override;

    void write(LogLevel level, const char* category, const char* message) override;
    void flush() override;

private:
    void rotate();
    void openFile();

    std::string filename_;
    size_t maxFileSize_;
    size_t maxFiles_;
    size_t currentSize_;
    void* fileHandle_;  // FILE* (opaque to avoid including <cstdio> in header)
};

/// Thread-safe logger singleton
/// Manages multiple log sinks and provides category-based log filtering
class Logger {
public:
    /// Get the singleton instance
    /// @return Reference to the global logger instance
    static Logger& getInstance() noexcept;

    // Delete copy/move constructors and assignment operators
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    /// Set the global log level
    /// Messages below this level will be filtered out for all categories
    /// @param level The minimum log level to display
    void setLevel(LogLevel level) noexcept;

    /// Get the current global log level
    /// @return The current minimum log level
    LogLevel getLevel() const noexcept;

    /// Set the log level for a specific category
    /// @param category The category name (e.g., "GPU", "Physics")
    /// @param level The minimum log level for this category
    void setCategoryLevel(const std::string& category, LogLevel level);

    /// Get the log level for a specific category
    /// @param category The category name
    /// @return The log level for this category (or global level if not set)
    LogLevel getCategoryLevel(const std::string& category) const;

    /// Add a log sink
    /// @param sink Shared pointer to the sink to add
    void addSink(std::shared_ptr<LogSink> sink);

    /// Remove a log sink
    /// @param sink Shared pointer to the sink to remove
    void removeSink(std::shared_ptr<LogSink> sink);

    /// Remove all log sinks
    void clearSinks();

    /// Log a message with formatting
    /// @param level The severity level
    /// @param category The category/module name
    /// @param format Format string (printf-style)
    /// @param ... Format arguments
    void log(LogLevel level, const char* category, const char* format, ...) noexcept;

    /// Flush all sinks
    void flush();

private:
    Logger();
    ~Logger();

    bool shouldLog(LogLevel level, const char* category) const noexcept;

    mutable std::mutex mutex_;
    LogLevel globalLevel_;
    std::unordered_map<std::string, LogLevel> categoryLevels_;
    std::vector<std::shared_ptr<LogSink>> sinks_;
};

}  // namespace axiom::core

//=============================================================================
// Logging Macros
//=============================================================================

// Suppress warnings about GNU ##__VA_ARGS__ extension (supported by all major compilers)
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

// Helper macro to handle optional variadic arguments portably
// MSVC, GCC, and Clang all support ##__VA_ARGS__ extension
#define AXIOM_LOG_IMPL(level, category, format, ...)                                              \
    ::axiom::core::Logger::getInstance().log(level, category, format, ##__VA_ARGS__)

/// Log a trace message
/// @param category The category/module name (e.g., "GPU", "Physics")
/// @param format Format string (printf-style)
/// @param ... Format arguments
#define AXIOM_LOG_TRACE(category, format, ...)                                                    \
    do {                                                                                           \
        AXIOM_LOG_IMPL(::axiom::core::LogLevel::Trace, category, format, __VA_ARGS__);            \
    } while (false)

/// Log a debug message
/// @param category The category/module name (e.g., "GPU", "Physics")
/// @param format Format string (printf-style)
/// @param ... Format arguments
#define AXIOM_LOG_DEBUG(category, format, ...)                                                    \
    do {                                                                                           \
        AXIOM_LOG_IMPL(::axiom::core::LogLevel::Debug, category, format, __VA_ARGS__);            \
    } while (false)

/// Log an info message
/// @param category The category/module name (e.g., "GPU", "Physics")
/// @param format Format string (printf-style)
/// @param ... Format arguments
#define AXIOM_LOG_INFO(category, format, ...)                                                     \
    do {                                                                                           \
        AXIOM_LOG_IMPL(::axiom::core::LogLevel::Info, category, format, __VA_ARGS__);             \
    } while (false)

/// Log a warning message
/// @param category The category/module name (e.g., "GPU", "Physics")
/// @param format Format string (printf-style)
/// @param ... Format arguments
#define AXIOM_LOG_WARN(category, format, ...)                                                     \
    do {                                                                                           \
        AXIOM_LOG_IMPL(::axiom::core::LogLevel::Warning, category, format, __VA_ARGS__);          \
    } while (false)

/// Log an error message
/// @param category The category/module name (e.g., "GPU", "Physics")
/// @param format Format string (printf-style)
/// @param ... Format arguments
#define AXIOM_LOG_ERROR(category, format, ...)                                                    \
    do {                                                                                           \
        AXIOM_LOG_IMPL(::axiom::core::LogLevel::Error, category, format, __VA_ARGS__);            \
    } while (false)

/// Log a fatal message
/// @param category The category/module name (e.g., "GPU", "Physics")
/// @param format Format string (printf-style)
/// @param ... Format arguments
#define AXIOM_LOG_FATAL(category, format, ...)                                                    \
    do {                                                                                           \
        AXIOM_LOG_IMPL(::axiom::core::LogLevel::Fatal, category, format, __VA_ARGS__);            \
    } while (false)

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
