// Config.h
#pragma once

#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <vector>
#include <cstdlib>

namespace ge::helpers {

class Config {
public:
    Config() = default;

    // Load simple key=value file. Comments start with '#' or ';'.
    // Returns true on success, optional error message in err.
    bool loadFromFile(const std::string& path, std::string* err = nullptr) {
        std::ifstream ifs(path);
        if (!ifs) {
            if (err) *err = "Failed to open file: " + path;
            return false;
        }

        std::unordered_map<std::string, std::string> tmp;
        std::string line;
        size_t lineno = 0;
        while (std::getline(ifs, line)) {
            ++lineno;
            trim(line);
            if (line.empty()) continue;
            if (line[0] == '#' || line[0] == ';') continue;

            auto eq = line.find('=');
            if (eq == std::string::npos) {
                // ignore malformed lines
                continue;
            }
            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq + 1);
            rtrim(key);
            ltrim(val);
            trim(key);
            trim(val);
            if (!key.empty()) tmp[key] = val;
        }

        {
            std::unique_lock lock(mutex_);
            data_.swap(tmp);
        }
        return true;
    }

    // Save current key/value pairs to file (simple, no sections).
    bool saveToFile(const std::string& path, std::string* err = nullptr) const {
        std::ofstream ofs(path, std::ios::trunc);
        if (!ofs) {
            if (err) *err = "Failed to open file for write: " + path;
            return false;
        }

        std::shared_lock lock(mutex_);
        for (const auto& kv : data_) {
            ofs << kv.first << " = " << kv.second << "\n";
        }
        return true;
    }

    // Generic getters. Environment variable overrides a stored value if present.
    std::string getString(const std::string& key, const std::string& def = "") const {
        if (const char* env = std::getenv(key.c_str())) return std::string(env);
        std::shared_lock lock(mutex_);
        auto it = data_.find(key);
        return it != data_.end() ? it->second : def;
    }

    int getInt(const std::string& key, int def = 0) const {
        std::string s = getString(key);
        if (s.empty()) return def;
        try {
            return std::stoi(s);
        } catch (...) {
            return def;
        }
    }

    double getDouble(const std::string& key, double def = 0.0) const {
        std::string s = getString(key);
        if (s.empty()) return def;
        try {
            return std::stod(s);
        } catch (...) {
            return def;
        }
    }

    bool getBool(const std::string& key, bool def = false) const {
        std::string s = getString(key);
        if (s.empty()) return def;
        toLower(s);
        if (s == "1" || s == "true" || s == "yes" || s == "on") return true;
        if (s == "0" || s == "false" || s == "no" || s == "off") return false;
        return def;
    }

    // Set or overwrite a value
    void set(const std::string& key, const std::string& value) {
        std::unique_lock lock(mutex_);
        data_[key] = value;
    }

    void set(const std::string& key, int value) { set(key, std::to_string(value)); }
    void set(const std::string& key, double value) { set(key, std::to_string(value)); }
    void set(const std::string& key, bool value) { set(key, value ? "true" : "false"); }

    bool has(const std::string& key) const {
        std::shared_lock lock(mutex_);
        return data_.find(key) != data_.end();
    }

    bool erase(const std::string& key) {
        std::unique_lock lock(mutex_);
        return data_.erase(key) > 0;
    }

    void clear() {
        std::unique_lock lock(mutex_);
        data_.clear();
    }

    std::vector<std::string> keys() const {
        std::shared_lock lock(mutex_);
        std::vector<std::string> out;
        out.reserve(data_.size());
        for (const auto& kv : data_) out.push_back(kv.first);
        return out;
    }

private:
    static inline void ltrim(std::string& s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c){ return !std::isspace(c); }));
    }
    static inline void rtrim(std::string& s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c){ return !std::isspace(c); }).base(), s.end());
    }
    static inline void trim(std::string& s) { ltrim(s); rtrim(s); }
    static inline void toLower(std::string& s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
    }

    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, std::string> data_;
};

} // namespace ge::helpers
