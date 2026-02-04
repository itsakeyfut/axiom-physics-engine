#include "axiom/core/assert.hpp"

#include <atomic>
#include <cstdio>
#include <cstdlib>

// Platform-specific headers for stack trace
#ifdef AXIOM_PLATFORM_WINDOWS
// clang-format off
#include <windows.h>  // Must be included before dbghelp.h
#include <dbghelp.h>
// clang-format on
#pragma comment(lib, "dbghelp.lib")
#elif defined(AXIOM_PLATFORM_LINUX) || defined(AXIOM_PLATFORM_MACOS)
#include <cxxabi.h>
#include <dlfcn.h>
#include <execinfo.h>
#endif

namespace axiom::core {

namespace {

// Global assertion handler (atomic for thread safety)
std::atomic<AssertHandler> g_assertHandler{nullptr};

// Maximum stack trace depth
constexpr int MAX_STACK_TRACE_DEPTH = 64;

/// Capture and print stack trace (platform-specific)
void printStackTrace() noexcept {
#ifdef AXIOM_PLATFORM_WINDOWS
    // Windows implementation using CaptureStackBackTrace
    void* stack[MAX_STACK_TRACE_DEPTH];
    HANDLE process = GetCurrentProcess();

    // Initialize symbol handler
    SymInitialize(process, nullptr, TRUE);
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);

    // Capture stack trace
    WORD frames = CaptureStackBackTrace(0, MAX_STACK_TRACE_DEPTH, stack, nullptr);

    fprintf(stderr, "\nStack trace:\n");

    // Print each frame
    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
    SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(buffer);
    symbol->MaxNameLen = MAX_SYM_NAME;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    for (WORD i = 0; i < frames; i++) {
        DWORD64 address = reinterpret_cast<DWORD64>(stack[i]);

        if (SymFromAddr(process, address, nullptr, symbol)) {
            // Try to get line information
            DWORD displacement;
            IMAGEHLP_LINE64 line;
            line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

            if (SymGetLineFromAddr64(process, address, &displacement, &line)) {
                fprintf(stderr, "  [%d] %s (%s:%lu)\n", i, symbol->Name, line.FileName,
                        line.LineNumber);
            } else {
                fprintf(stderr, "  [%d] %s (address: 0x%llx)\n", i, symbol->Name, address);
            }
        } else {
            fprintf(stderr, "  [%d] <unknown> (address: 0x%llx)\n", i, address);
        }
    }

    SymCleanup(process);

#elif defined(AXIOM_PLATFORM_LINUX) || defined(AXIOM_PLATFORM_MACOS)
    // Linux/macOS implementation using backtrace
    void* stack[MAX_STACK_TRACE_DEPTH];
    int frames = backtrace(stack, MAX_STACK_TRACE_DEPTH);
    char** symbols = backtrace_symbols(stack, frames);

    fprintf(stderr, "\nStack trace:\n");

    for (int i = 0; i < frames; i++) {
        // Try to demangle C++ symbols
        Dl_info info;
        if (dladdr(stack[i], &info) && info.dli_sname) {
            int status = 0;
            char* demangled = abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, &status);

            if (status == 0 && demangled) {
                fprintf(stderr, "  [%d] %s\n", i, demangled);
                free(demangled);
            } else {
                fprintf(stderr, "  [%d] %s\n", i, info.dli_sname);
            }
        } else if (symbols) {
            fprintf(stderr, "  [%d] %s\n", i, symbols[i]);
        } else {
            fprintf(stderr, "  [%d] <unknown>\n", i);
        }
    }

    free(symbols);
#else
    // Fallback: no stack trace support
    fprintf(stderr, "\nStack trace: <not available on this platform>\n");
#endif
}

}  // anonymous namespace

void setAssertHandler(AssertHandler handler) noexcept {
    g_assertHandler.store(handler, std::memory_order_relaxed);
}

void defaultAssertHandler(const char* expr, const char* msg, const char* file, int line) noexcept {
    // Print assertion failure information
    fprintf(stderr, "\n");
    fprintf(stderr,
            "================================================================================\n");
    fprintf(stderr, "ASSERTION FAILED\n");
    fprintf(stderr,
            "================================================================================\n");
    fprintf(stderr, "Expression: %s\n", expr);
    if (msg && msg[0] != '\0') {
        fprintf(stderr, "Message:    %s\n", msg);
    }
    fprintf(stderr, "Location:   %s:%d\n", file, line);

    // Print stack trace
    printStackTrace();

    fprintf(stderr,
            "================================================================================\n");
    fflush(stderr);

    // Abort the program
    std::abort();
}

void assertionFailed(const char* expr, const char* msg, const char* file, int line) noexcept {
    // Get the current handler (thread-safe)
    AssertHandler handler = g_assertHandler.load(std::memory_order_relaxed);

    if (handler) {
        // Call custom handler
        handler(expr, msg, file, line);
        // If custom handler returns, still abort
        std::abort();
    } else {
        // Call default handler (which will abort)
        defaultAssertHandler(expr, msg, file, line);
    }
}

}  // namespace axiom::core
