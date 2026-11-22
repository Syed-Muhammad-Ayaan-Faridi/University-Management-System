// Searching.h - Linear, binary and interpolation search over arrays of records.
// Each routine takes a key-extractor so it works on any record type, and reports
// the number of probes so the algorithms can be compared.
#pragma once

using namespace std;

namespace ums
{

    struct SearchResult
    {
        int index;        // position in the array, or -1 if not found
        long long probes; // number of elements examined
    };

    // Works on UNSORTED data. O(n).
    template <typename T, typename KeyFn>
    SearchResult linearSearch(const T *a, int n, int key, KeyFn keyOf)
    {
        SearchResult r{-1, 0};
        for (int i = 0; i < n; ++i)
        {
            ++r.probes;
            if (keyOf(a[i]) == key)
            {
                r.index = i;
                break;
            }
        }
        return r;
    }

    // Predicate version (e.g. match by name). Returns the first matching index.
    template <typename T, typename Pred>
    int linearFindIf(const T *a, int n, Pred pred)
    {
        for (int i = 0; i < n; ++i)
            if (pred(a[i]))
                return i;
        return -1;
    }

    // Requires data sorted ascending by key. O(log n).
    template <typename T, typename KeyFn>
    SearchResult binarySearch(const T *a, int n, int key, KeyFn keyOf)
    {
        SearchResult r{-1, 0};
        int lo = 0, hi = n - 1;
        while (lo <= hi)
        {
            int mid = lo + (hi - lo) / 2;
            ++r.probes;
            int k = keyOf(a[mid]);
            if (k == key)
            {
                r.index = mid;
                return r;
            }
            if (k < key)
                lo = mid + 1;
            else
                hi = mid - 1;
        }
        return r;
    }

    // Requires data sorted ascending and roughly uniformly distributed keys
    // (e.g. consecutive roll numbers). Average O(log log n).
    template <typename T, typename KeyFn>
    SearchResult interpolationSearch(const T *a, int n, int key, KeyFn keyOf)
    {
        SearchResult r{-1, 0};
        int lo = 0, hi = n - 1;
        while (lo <= hi && n > 0 && key >= keyOf(a[lo]) && key <= keyOf(a[hi]))
        {
            long long klo = keyOf(a[lo]), khi = keyOf(a[hi]);
            int pos;
            if (khi == klo)
            {
                pos = lo;
            }
            else
            {
                pos = lo + (int)(((long long)(key - klo) * (hi - lo)) / (khi - klo));
            }
            ++r.probes;
            int k = keyOf(a[pos]);
            if (k == key)
            {
                r.index = pos;
                return r;
            }
            if (k < key)
                lo = pos + 1;
            else
                hi = pos - 1;
        }
        return r;
    }

} // namespace ums