// Logger.h
#pragma once

#include <iostream>
#include <mutex>

namespace ge::helpers {

    // Minimal thread-safe logger interface
    class Logger {
      public:
        enum class Level { Trace, Debug, Info, Warn, Error };

        static Logger& instance() {
            static Logger l;
            return l;
        }

        void setLevel(Level lvl) { level_.store(static_cast<int>(lvl)); }

        template<typename... Args>
        void log(Level lvl, const std::string& fmt, Args&&... args) {
            if (static_cast<int>(lvl) < level_.load()) return;
            std::lock_guard<std::mutex> guard(mutex_);
            // Very simple formatting using operator<<; replace with fmtlib or similar if available
            std::cout << "[" << levelToString(lvl) << "] " << fmt << "\n";
            // Note: expand to formatted output in real code
        }

      private:
        Logger() : level_(static_cast<int>(Level::Info)) {}
        const char* levelToString(Level l) const {
            switch (l) {
            case Level::Trace: return "TRACE";
            case Level::Debug: return "DEBUG";
            case Level::Info:  return "INFO";
            case Level::Warn:  return "WARN";
            case Level::Error: return "ERROR";
            default: return "UNK";
            }
        }
        std::atomic<int> level_;
        std::mutex mutex_;
    };

    // Assertion helper: logs an error and aborts when the condition is false.
    inline void Assert(bool condition, const std::string& message = "Assertion failed") {
        if (!condition) {
            Logger::instance().log(Logger::Level::Error, message);
            std::abort();
        }
    }

} // namespace ge::helpers

// Macro for convenient use; disabled when NDEBUG is defined.
#ifndef NDEBUG
#define GE_ASSERT(cond, msg) \
    ((cond) ? (void)0 : ::ge::helpers::Assert(false, (msg)))
#else
#define GE_ASSERT(cond, msg) ((void)0)
#endif