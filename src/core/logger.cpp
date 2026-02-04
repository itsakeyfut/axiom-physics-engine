#include "axiom/core/logger.hpp"

#include <algorithm>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>

#ifdef AXIOM_PLATFORM_WINDOWS
#include <windows.h>
#else
#include <unistd.h>  // For isatty
#endif

namespace axiom::core {

namespace {

/// ANSI color codes for terminal output
namespace ansi {
constexpr const char* Reset = "\033[0m";
constexpr const char* Gray = "\033[90m";
constexpr const char* Cyan = "\033[36m";
constexpr const char* Green = "\033[32m";
constexpr const char* Yellow = "\033[33m";
constexpr const char* Red = "\033[31m";
constexpr const char* BrightRed = "\033[91m";
}  // namespace ansi

/// Check if the terminal supports ANSI color codes
bool supportsAnsiColors() {
#ifdef AXIOM_PLATFORM_WINDOWS
    // Windows 10+ supports ANSI colors if virtual terminal processing is enabled
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD mode = 0;
    if (!GetConsoleMode(hOut, &mode)) {
        return false;
    }

    // Try to enable virtual terminal processing
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    return SetConsoleMode(hOut, mode) != 0;
#else
    // On Unix-like systems, check if stdout is a terminal
    // Most modern terminals support ANSI colors
    return isatty(fileno(stdout)) != 0;
#endif
}

/// Get current timestamp as a formatted string
std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tm_buf;
#ifdef AXIOM_PLATFORM_WINDOWS
    localtime_s(&tm_buf, &time);
#else
    localtime_r(&time, &tm_buf);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

/// Format a log message with timestamp and category
std::string formatLogMessage(LogLevel level, const char* category, const char* message) {
    std::ostringstream oss;
    oss << "[" << getCurrentTimestamp() << "] ";
    oss << "[" << logLevelToString(level) << "] ";
    oss << "[" << category << "] ";
    oss << message;
    return oss.str();
}

}  // anonymous namespace

//=============================================================================
// LogLevel Utilities
//=============================================================================

const char* logLevelToString(LogLevel level) noexcept {
    switch (level) {
    case LogLevel::Trace:
        return "TRACE";
    case LogLevel::Debug:
        return "DEBUG";
    case LogLevel::Info:
        return "INFO ";
    case LogLevel::Warning:
        return "WARN ";
    case LogLevel::Error:
        return "ERROR";
    case LogLevel::Fatal:
        return "FATAL";
    default:
        return "UNKNOWN";
    }
}

//=============================================================================
// ConsoleLogSink
//=============================================================================

ConsoleLogSink::ConsoleLogSink(bool useColors) : useColors_(useColors && supportsAnsiColors()) {}

void ConsoleLogSink::write(LogLevel level, const char* category, const char* message) {
    std::string formatted = formatLogMessage(level, category, message);

    if (useColors_) {
        const char* color = ansi::Reset;
        switch (level) {
        case LogLevel::Trace:
            color = ansi::Gray;
            break;
        case LogLevel::Debug:
            color = ansi::Cyan;
            break;
        case LogLevel::Info:
            color = ansi::Green;
            break;
        case LogLevel::Warning:
            color = ansi::Yellow;
            break;
        case LogLevel::Error:
            color = ansi::Red;
            break;
        case LogLevel::Fatal:
            color = ansi::BrightRed;
            break;
        }

        fprintf(stdout, "%s%s%s\n", color, formatted.c_str(), ansi::Reset);
    } else {
        fprintf(stdout, "%s\n", formatted.c_str());
    }
}

void ConsoleLogSink::flush() {
    fflush(stdout);
}

//=============================================================================
// FileLogSink
//=============================================================================

FileLogSink::FileLogSink(const std::string& filename, size_t maxFileSize, size_t maxFiles)
    : filename_(filename),
      maxFileSize_(maxFileSize),
      maxFiles_(maxFiles),
      currentSize_(0),
      fileHandle_(nullptr) {
    openFile();
}

FileLogSink::~FileLogSink() {
    if (fileHandle_) {
        fclose(static_cast<FILE*>(fileHandle_));
    }
}

void FileLogSink::openFile() {
    FILE* file = nullptr;

#ifdef AXIOM_PLATFORM_WINDOWS
    fopen_s(&file, filename_.c_str(), "a");
#else
    file = fopen(filename_.c_str(), "a");
#endif

    fileHandle_ = file;

    if (file) {
        // Get current file size
        fseek(file, 0, SEEK_END);
        currentSize_ = static_cast<size_t>(ftell(file));
    }
}

void FileLogSink::rotate() {
    if (!fileHandle_) {
        return;
    }

    // Close current file
    fclose(static_cast<FILE*>(fileHandle_));
    fileHandle_ = nullptr;

    // Rotate existing log files
    namespace fs = std::filesystem;
    try {
        // Delete oldest file if it exists
        if (maxFiles_ > 0) {
            std::string oldestFile = filename_ + "." + std::to_string(maxFiles_);
            if (fs::exists(oldestFile)) {
                fs::remove(oldestFile);
            }
        }

        // Rename existing files
        for (size_t i = maxFiles_; i > 0; --i) {
            std::string oldFile = (i == 1) ? filename_ : filename_ + "." + std::to_string(i - 1);
            std::string newFile = filename_ + "." + std::to_string(i);

            if (fs::exists(oldFile)) {
                fs::rename(oldFile, newFile);
            }
        }
    } catch (...) {
        // Ignore filesystem errors during rotation
    }

    // Open new file
    currentSize_ = 0;
    openFile();
}

void FileLogSink::write(LogLevel level, const char* category, const char* message) {
    if (!fileHandle_) {
        return;
    }

    FILE* file = static_cast<FILE*>(fileHandle_);
    std::string formatted = formatLogMessage(level, category, message);

    int result = fprintf(file, "%s\n", formatted.c_str());
    if (result > 0) {
        currentSize_ += static_cast<size_t>(result);
    }

    // Check if rotation is needed
    if (maxFileSize_ > 0 && currentSize_ >= maxFileSize_) {
        rotate();
    }
}

void FileLogSink::flush() {
    if (fileHandle_) {
        fflush(static_cast<FILE*>(fileHandle_));
    }
}

//=============================================================================
// Logger
//=============================================================================

Logger::Logger() : globalLevel_(LogLevel::Info) {
    // Add default console sink
    addSink(std::make_shared<ConsoleLogSink>());
}

Logger::~Logger() {
    flush();
    clearSinks();
}

Logger& Logger::getInstance() noexcept {
    static Logger instance;
    return instance;
}

void Logger::setLevel(LogLevel level) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    globalLevel_ = level;
}

LogLevel Logger::getLevel() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return globalLevel_;
}

void Logger::setCategoryLevel(const std::string& category, LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    categoryLevels_[category] = level;
}

LogLevel Logger::getCategoryLevel(const std::string& category) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = categoryLevels_.find(category);
    return (it != categoryLevels_.end()) ? it->second : globalLevel_;
}

void Logger::addSink(std::shared_ptr<LogSink> sink) {
    if (!sink) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    sinks_.push_back(std::move(sink));
}

void Logger::removeSink(std::shared_ptr<LogSink> sink) {
    if (!sink) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    sinks_.erase(std::remove(sinks_.begin(), sinks_.end(), sink), sinks_.end());
}

void Logger::clearSinks() {
    std::lock_guard<std::mutex> lock(mutex_);
    sinks_.clear();
}

bool Logger::shouldLog(LogLevel level, const char* category) const noexcept {
    // Check category-specific level first
    auto it = categoryLevels_.find(category);
    LogLevel threshold = (it != categoryLevels_.end()) ? it->second : globalLevel_;

    return static_cast<int>(level) >= static_cast<int>(threshold);
}

void Logger::log(LogLevel level, const char* category, const char* format, ...) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);

    // Early exit if this log level is filtered out
    if (!shouldLog(level, category)) {
        return;
    }

    // Format the message using variadic arguments
    va_list args;
    va_start(args, format);

    // Determine required buffer size
    va_list args_copy;
    va_copy(args_copy, args);

    // Suppress format-nonliteral warning for vsnprintf with dynamic format strings
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif

    int size = vsnprintf(nullptr, 0, format, args_copy);
    va_end(args_copy);

    if (size < 0) {
        va_end(args);
        return;
    }

    // Allocate buffer and format message
    std::vector<char> buffer(static_cast<size_t>(size) + 1);
    vsnprintf(buffer.data(), buffer.size(), format, args);

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

    va_end(args);

    // Write to all sinks
    const char* message = buffer.data();
    for (auto& sink : sinks_) {
        if (sink) {
            sink->write(level, category, message);
        }
    }
}

void Logger::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& sink : sinks_) {
        if (sink) {
            sink->flush();
        }
    }
}

}  // namespace axiom::core
