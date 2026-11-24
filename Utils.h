// Utils.h - Console input helpers with validation, plus small string utilities.
#pragma once
#include <cctype>
#include <iostream>
#include <stdexcept>
#include <string>
#include "ADT.h"

namespace ums {

// Thrown when standard input is closed (Ctrl+Z / Ctrl+D / end of piped input).
struct InputEnded : std::runtime_error {
    InputEnded() : std::runtime_error("input ended") {}
};

inline std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char)s[a])) ++a;
    while (b > a && std::isspace((unsigned char)s[b - 1])) --b;
    return s.substr(a, b - a);
}
inline std::string upperCopy(const std::string& s) {
    std::string r = s;
    for (size_t i = 0; i < r.size(); ++i) r[i] = (char)std::toupper((unsigned char)r[i]);
    return r;
}
// Field separators used by the text-file layer must never appear inside data.
inline std::string sanitize(const std::string& s) {
    std::string r = trim(s);
    for (size_t i = 0; i < r.size(); ++i)
        if (r[i] == '|' || r[i] == '\n' || r[i] == '\r') r[i] = '/';
    return r;
}
inline std::string clip(const std::string& s, size_t w) { return s.size() <= w ? s : s.substr(0, w - 1) + "~"; }

inline bool parseIntStrict(const std::string& s, int& out) {
    if (s.empty()) return false;
    try {
        size_t pos = 0;
        long v = std::stol(s, &pos);
        if (pos != s.size() || v < -2000000000L || v > 2000000000L) return false;
        out = (int)v;
        return true;
    } catch (...) {
        return false;
    }
}
inline bool parseDoubleStrict(const std::string& s, double& out) {
    if (s.empty()) return false;
    try {
        size_t pos = 0;
        double v = std::stod(s, &pos);
        if (pos != s.size()) return false;
        out = v;
        return true;
    } catch (...) {
        return false;
    }
}
// Accepts "3081" or "25K-3081".
inline bool parseRoll(const std::string& s, int& out) {
    std::string t = trim(s);
    size_t dash = t.rfind('-');
    if (dash != std::string::npos && dash > 0) t = t.substr(dash + 1);  // strip a prefix like "25K-"; a leading '-' is not a prefix
    return parseIntStrict(t, out) && out > 0;
}

// Splits on a delimiter, keeping empty fields.
inline DynArray<std::string> splitStr(const std::string& s, char delim) {
    DynArray<std::string> parts;
    std::string cur;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == delim) { parts.push_back(cur); cur.clear(); }
        else cur += s[i];
    }
    parts.push_back(cur);
    return parts;
}

inline std::string readLine(const std::string& prompt) {
    std::cout << prompt << std::flush;
    std::string s;
    if (!std::getline(std::cin, s)) throw InputEnded();
    return trim(s);
}
inline int readInt(const std::string& prompt, int lo, int hi) {
    while (true) {
        int v;
        if (parseIntStrict(readLine(prompt), v) && v >= lo && v <= hi) return v;
        std::cout << "  Please enter a whole number between " << lo << " and " << hi << ".\n";
    }
}
inline double readDouble(const std::string& prompt, double lo, double hi) {
    while (true) {
        double v;
        if (parseDoubleStrict(readLine(prompt), v) && v >= lo && v <= hi) return v;
        std::cout << "  Please enter a number between " << lo << " and " << hi << ".\n";
    }
}
inline int readRoll(const std::string& prompt) {
    while (true) {
        int v;
        if (parseRoll(readLine(prompt), v)) return v;
        std::cout << "  Enter a roll number such as 3081 or 25K-3081.\n";
    }
}
inline std::string readNonEmpty(const std::string& prompt) {
    while (true) {
        std::string s = sanitize(readLine(prompt));
        if (!s.empty()) return s;
        std::cout << "  This field cannot be empty.\n";
    }
}
inline bool askYesNo(const std::string& prompt) {
    while (true) {
        std::string s = upperCopy(readLine(prompt + " (y/n): "));
        if (s == "Y" || s == "YES") return true;
        if (s == "N" || s == "NO") return false;
    }
}

}  // namespace ums
