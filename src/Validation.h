// Validation.h - input checking + small string helpers (no std::to_string /
// std::stoi so it also builds on older MinGW versions).
#ifndef VALIDATION_H
#define VALIDATION_H

#include <string>

namespace validate {

const int MIN_ID       = 1;
const int MAX_ID       = 999999999;
const int MIN_AGE      = 0;
const int MAX_AGE      = 120;
const int MIN_SEVERITY = 1;
const int MAX_SEVERITY = 5;
const int MAX_NAME_LEN = 50;

inline std::string toStr(long long v) {
    if (v == 0) return "0";
    bool neg = v < 0;
    unsigned long long u = neg ? (unsigned long long)(-(v + 1)) + 1ULL : (unsigned long long)v;
    std::string s;
    while (u > 0) { s.insert(s.begin(), (char)('0' + (int)(u % 10))); u /= 10; }
    if (neg) s.insert(s.begin(), '-');
    return s;
}

// Strict parse: digits only (no sign, spaces, decimals), within [lo, hi].
inline bool parseInteger(const std::string& s, long long lo, long long hi, long long& out) {
    if (s.empty() || s.size() > 18) return false;
    long long v = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] < '0' || s[i] > '9') return false;
        v = v * 10 + (s[i] - '0');
    }
    if (v < lo || v > hi) return false;
    out = v;
    return true;
}

inline std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r' || s[a] == '\n')) ++a;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r' || s[b - 1] == '\n')) --b;
    return s.substr(a, b - a);
}

// Name: 1..50 chars, letters, spaces, '.', '\'' and '-' only; at least one letter.
inline bool validName(const std::string& n) {
    if (n.empty() || (int)n.size() > MAX_NAME_LEN) return false;
    bool hasLetter = false;
    for (size_t i = 0; i < n.size(); ++i) {
        char c = n[i];
        bool letter = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
        if (letter) hasLetter = true;
        else if (c != ' ' && c != '.' && c != '\'' && c != '-') return false;
    }
    return hasLetter;
}

inline bool validId(long long v)       { return v >= MIN_ID && v <= MAX_ID; }
inline bool validAge(long long v)      { return v >= MIN_AGE && v <= MAX_AGE; }
inline bool validSeverity(long long v) { return v >= MIN_SEVERITY && v <= MAX_SEVERITY; }

} // namespace validate

#endif
