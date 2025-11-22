// cpp
#pragma once

#include <array>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>

namespace ge::helpers {

/* =========================================================================
 * UUID - RFC4122 v4 generator (string form, lowercase, hyphenated)
 * ========================================================================= */
class UUID {
public:
    // Generate a random v4 UUID as a lowercase hyphenated string.
    static std::string generateV4() {
        // generate 16 random bytes
        std::array<uint8_t, 16> bytes;
        {
            static std::mutex m;
            static std::random_device rd;
            static std::mt19937_64 rng(rd());
            std::lock_guard<std::mutex> lock(m);
            auto& gen = rng;
            for (auto &b : bytes) b = static_cast<uint8_t>(gen() & 0xFF);
        }

        // set version (4) and variant (RFC4122)
        bytes[6] = static_cast<uint8_t>((bytes[6] & 0x0F) | 0x40);
        bytes[8] = static_cast<uint8_t>((bytes[8] & 0x3F) | 0x80);

        // format as hex with hyphens 8-4-4-4-12
        static const char* hex = "0123456789abcdef";
        std::string out;
        out.reserve(36);
        auto append_hex = [&](uint8_t b) {
            out.push_back(hex[(b >> 4) & 0xF]);
            out.push_back(hex[b & 0xF]);
        };

        for (size_t i = 0; i < bytes.size(); ++i) {
            append_hex(bytes[i]);
            if (i == 3 || i == 5 || i == 7 || i == 9) out.push_back('-');
        }
        return out;
    }
};

/* =========================================================================
 * Random - convenience wrapper around std::mt19937_64 (thread-safe where needed)
 * ========================================================================= */
class Random {
public:
    Random() : rng_(seedFromDevice()) {}
    explicit Random(uint64_t seed) : rng_(seed) {}

    void seed(uint64_t s) {
        std::lock_guard<std::mutex> lock(mutex_);
        rng_.seed(s);
    }

    // raw 64-bit random
    uint64_t nextU64() {
        std::lock_guard<std::mutex> lock(mutex_);
        return rng_();
    }

    // uniform integer in [lo, hi]
    template<typename Int = int>
    Int uniformInt(Int lo, Int hi) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::uniform_int_distribution<Int> dist(lo, hi);
        return dist(rng_);
    }

    // uniform real in [lo, hi)
    template<typename Real = double>
    Real uniformReal(Real lo = Real(0.0), Real hi = Real(1.0)) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::uniform_real_distribution<Real> dist(lo, hi);
        return dist(rng_);
    }

    // choose a random element from a container (by reference); returns nullptr on empty
    template<typename Container>
    auto* choose(Container& c) {
        if (c.empty()) return static_cast<typename Container::value_type*>(nullptr);
        size_t idx = static_cast<size_t>(uniformInt<size_t>(0, c.size() - 1));
        return &c[idx];
    }

    // shuffle a mutable sequence
    template<typename RandomIt>
    void shuffle(RandomIt first, RandomIt last) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::shuffle(first, last, rng_);
    }

private:
    static uint64_t seedFromDevice() {
        std::random_device rd;
        uint64_t a = (static_cast<uint64_t>(rd()) << 32) ^ static_cast<uint64_t>(rd());
        return a ^ static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    }

    std::mt19937_64 rng_;
    std::mutex mutex_;
};

/* =========================================================================
 * StringUtil - small set of string helpers
 * ========================================================================= */
class StringUtil {
public:
    static inline void ltrim(std::string& s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    }
    static inline void rtrim(std::string& s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    }
    static inline void trim(std::string& s) {
        ltrim(s); rtrim(s);
    }

    static inline std::string toLower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
        return s;
    }

    static inline std::string toUpper(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return static_cast<char>(std::toupper(c)); });
        return s;
    }

    static inline bool startsWith(const std::string& s, const std::string& prefix) {
        return s.size() >= prefix.size() && std::equal(prefix.begin(), prefix.end(), s.begin());
    }

    static inline bool endsWith(const std::string& s, const std::string& suffix) {
        return s.size() >= suffix.size() && std::equal(suffix.rbegin(), suffix.rend(), s.rbegin());
    }

    static inline std::vector<std::string> split(const std::string& s, char delim) {
        std::vector<std::string> out;
        std::string token;
        std::istringstream iss(s);
        while (std::getline(iss, token, delim)) out.push_back(token);
        return out;
    }

    static inline std::string join(const std::vector<std::string>& parts, const std::string& delim) {
        if (parts.empty()) return {};
        std::ostringstream oss;
        oss << parts[0];
        for (size_t i = 1; i < parts.size(); ++i) oss << delim << parts[i];
        return oss.str();
    }
};

/* =========================================================================
 * DateTime - utilities for current time: ISO8601 string and epoch millis
 * Note: uses UTC (Zulu) formatting.
 * ========================================================================= */
class DateTime {
public:
    // returns ISO8601 UTC string like "2025-11-22T12:34:56.789Z"
    static std::string nowISO8601UTC() {
        using clock = std::chrono::system_clock;
        auto now = clock::now();
        auto secs = clock::to_time_t(now);
        std::tm tm{};
    #if defined(_WIN32) || defined(_WIN64)
        gmtime_s(&tm, &secs);
    #else
        gmtime_r(&secs, &tm);
    #endif
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
        oss << '.' << std::setw(3) << std::setfill('0') << ms.count() << 'Z';
        return oss.str();
    }

    // epoch milliseconds (UTC)
    static int64_t nowEpochMillis() {
        using clock = std::chrono::system_clock;
        auto now = clock::now();
        return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
    }

    // convert time_t (assumed UTC) to ISO8601 string (no milliseconds)
    static std::string timeTToISO8601UTC(std::time_t t) {
        std::tm tm{};
    #if defined(_WIN32) || defined(_WIN64)
        gmtime_s(&tm, &t);
    #else
        gmtime_r(&t, &tm);
    #endif
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
        return oss.str();
    }
};

} // namespace ge::helpers
