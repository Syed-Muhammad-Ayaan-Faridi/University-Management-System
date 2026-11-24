// StringAlgo.h - Knuth-Morris-Pratt and Rabin-Karp pattern matching.
#pragma once
#include <cctype>
#include <string>
#include "ADT.h"

namespace ums {

struct MatchResult {
    DynArray<int> positions;   // start index of every occurrence
    long long comparisons = 0; // character comparisons performed
    long long hashHits = 0;    // Rabin-Karp: windows whose hash matched
    long long spurious = 0;    // Rabin-Karp: hash matched but text did not
};

inline std::string toLowerCopy(const std::string& s) {
    std::string r = s;
    for (size_t i = 0; i < r.size(); ++i) r[i] = (char)std::tolower((unsigned char)r[i]);
    return r;
}

// KMP: precompute the longest-proper-prefix-suffix table, then scan the text once. O(n + m).
inline MatchResult kmpSearch(const std::string& text, const std::string& pat) {
    MatchResult r;
    int n = (int)text.size(), m = (int)pat.size();
    if (m == 0 || m > n) return r;
    DynArray<int> lps(m, 0);
    for (int i = 1, k = 0; i < m; ++i) {
        while (k > 0 && pat[k] != pat[i]) k = lps[k - 1];
        if (pat[k] == pat[i]) ++k;
        lps[i] = k;
    }
    int q = 0;
    for (int i = 0; i < n; ++i) {
        while (q > 0 && (++r.comparisons, pat[q] != text[i])) q = lps[q - 1];
        ++r.comparisons;
        if (pat[q] == text[i]) ++q;
        if (q == m) {
            r.positions.push_back(i - m + 1);
            q = lps[q - 1];
        }
    }
    return r;
}

// Rabin-Karp: rolling polynomial hash over a window of length m. Average O(n + m).
inline MatchResult rabinKarpSearch(const std::string& text, const std::string& pat) {
    MatchResult r;
    int n = (int)text.size(), m = (int)pat.size();
    if (m == 0 || m > n) return r;
    const long long BASE = 256, MOD = 1000000007LL;
    long long high = 1;  // BASE^(m-1) mod MOD
    for (int i = 0; i < m - 1; ++i) high = (high * BASE) % MOD;
    long long hp = 0, ht = 0;
    for (int i = 0; i < m; ++i) {
        hp = (hp * BASE + (unsigned char)pat[i]) % MOD;
        ht = (ht * BASE + (unsigned char)text[i]) % MOD;
    }
    for (int i = 0; i + m <= n; ++i) {
        if (hp == ht) {
            ++r.hashHits;
            bool ok = true;
            for (int j = 0; j < m; ++j) {
                ++r.comparisons;
                if (text[i + j] != pat[j]) { ok = false; break; }
            }
            if (ok) r.positions.push_back(i); else ++r.spurious;
        }
        if (i + m < n) {
            ht = (ht + MOD - ((unsigned char)text[i] * high) % MOD) % MOD;  // drop leading char
            ht = (ht * BASE + (unsigned char)text[i + m]) % MOD;           // add trailing char
        }
    }
    return r;
}

}  // namespace ums
