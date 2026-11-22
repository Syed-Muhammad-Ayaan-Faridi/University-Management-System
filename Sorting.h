// Sorting.h - Seven sorting algorithms implemented from scratch.
// All comparison sorts take a `less(a, b)` functor and return SortStats so the
// algorithms can be compared (comparisons and element moves/swaps).
#pragma once
#include <stdexcept>
#include <utility>
#include "ADT.h"

using namespace std;

namespace ums
{

    struct SortStats
    {
        int comparisons = 0;
        int moves = 0; // element swaps or copies
    };

    // O(n^2) simple sorts

    template <typename T, typename Less>
    SortStats bubbleSort(T *a, int n, Less less)
    {
        SortStats st;
        for (int i = 0; i < n - 1; ++i)
        {
            bool swapped = false;
            for (int j = 0; j < n - 1 - i; ++j)
            {
                ++st.comparisons;
                if (less(a[j + 1], a[j]))
                {
                    swap(a[j], a[j + 1]);
                    ++st.moves;
                    swapped = true;
                }
            }
            if (!swapped)
                break; // already sorted
        }
        return st;
    }

    template <typename T, typename Less>
    SortStats selectionSort(T *a, int n, Less less)
    {
        SortStats st;
        for (int i = 0; i < n - 1; ++i)
        {
            int best = i;
            for (int j = i + 1; j < n; ++j)
            {
                ++st.comparisons;
                if (less(a[j], a[best]))
                    best = j;
            }
            if (best != i)
            {
                swap(a[i], a[best]);
                ++st.moves;
            }
        }
        return st;
    }

    template <typename T, typename Less>
    SortStats insertionSort(T *a, int n, Less less)
    {
        SortStats st;
        for (int i = 1; i < n; ++i)
        {
            T key = a[i];
            int j = i - 1;
            while (j >= 0)
            {
                ++st.comparisons;
                if (!less(key, a[j]))
                    break;
                a[j + 1] = a[j];
                ++st.moves;
                --j;
            }
            a[j + 1] = key;
            ++st.moves;
        }
        return st;
    }

    // Comb sort: bubble sort with a shrinking gap (shrink factor 1.3).
    template <typename T, typename Less>
    SortStats combSort(T *a, int n, Less less)
    {
        SortStats st;
        int gap = n;
        bool swapped = true;
        while (gap > 1 || swapped)
        {
            gap = (gap * 10) / 13;
            if (gap < 1)
                gap = 1;
            swapped = false;
            for (int i = 0; i + gap < n; ++i)
            {
                ++st.comparisons;
                if (less(a[i + gap], a[i]))
                {
                    swap(a[i], a[i + gap]);
                    ++st.moves;
                    swapped = true;
                }
            }
        }
        return st;
    }

    // O(n log n) sorts

    namespace detail
    {

        template <typename T, typename Less>
        void mergeRec(T *a, T *tmp, int lo, int hi, Less &less, SortStats &st)
        { // [lo, hi)
            if (hi - lo < 2)
                return;
            int mid = lo + (hi - lo) / 2;
            mergeRec(a, tmp, lo, mid, less, st);
            mergeRec(a, tmp, mid, hi, less, st);
            int i = lo, j = mid, k = lo;
            while (i < mid && j < hi)
            {
                ++st.comparisons;
                if (less(a[j], a[i]))
                    tmp[k++] = a[j++];
                else
                    tmp[k++] = a[i++]; // stable
                ++st.moves;
            }
            while (i < mid)
            {
                tmp[k++] = a[i++];
                ++st.moves;
            }
            while (j < hi)
            {
                tmp[k++] = a[j++];
                ++st.moves;
            }
            for (k = lo; k < hi; ++k)
                a[k] = tmp[k];
        }

        template <typename T, typename Less>
        void quickRec(T *a, int lo, int hi, Less &less, SortStats &st)
        { // inclusive bounds
            while (lo < hi)
            {
                // median-of-three pivot selection
                int mid = lo + (hi - lo) / 2;
                ++st.comparisons;
                if (less(a[mid], a[lo]))
                {
                    swap(a[mid], a[lo]);
                    ++st.moves;
                }
                ++st.comparisons;
                if (less(a[hi], a[lo]))
                {
                    swap(a[hi], a[lo]);
                    ++st.moves;
                }
                ++st.comparisons;
                if (less(a[hi], a[mid]))
                {
                    swap(a[hi], a[mid]);
                    ++st.moves;
                }
                T pivot = a[mid];
                // Hoare-style partition (copes well with many equal keys)
                int i = lo, j = hi;
                while (i <= j)
                {
                    while (++st.comparisons, less(a[i], pivot))
                        ++i;
                    while (++st.comparisons, less(pivot, a[j]))
                        --j;
                    if (i <= j)
                    {
                        swap(a[i], a[j]);
                        ++st.moves;
                        ++i;
                        --j;
                    }
                }
                // recurse into the smaller half, loop on the larger => O(log n) stack depth
                if (j - lo < hi - i)
                {
                    quickRec(a, lo, j, less, st);
                    lo = i;
                }
                else
                {
                    quickRec(a, i, hi, less, st);
                    hi = j;
                }
            }
        }

    } // namespace detail

    template <typename T, typename Less>
    SortStats mergeSort(T *a, int n, Less less)
    {
        SortStats st;
        if (n < 2)
            return st;
        DynArray<T> tmp(n);
        detail::mergeRec(a, tmp.data(), 0, n, less, st);
        return st;
    }

    template <typename T, typename Less>
    SortStats quickSort(T *a, int n, Less less)
    {
        SortStats st;
        if (n < 2)
            return st;
        detail::quickRec(a, 0, n - 1, less, st);
        return st;
    }

    // Non-comparison sort

    // LSD radix sort (base 10) on non-negative integer keys; stable, O(d * (n + 10)).
    template <typename T, typename KeyFn>
    SortStats radixSort(T *a, int n, KeyFn keyOf)
    {
        SortStats st;
        if (n < 2)
            return st;
        int maxKey = 0;
        for (int i = 0; i < n; ++i)
        {
            int k = keyOf(a[i]);
            if (k < 0)
                throw invalid_argument("radixSort: keys must be non-negative");
            if (k > maxKey)
                maxKey = k;
        }
        DynArray<T> out(n);
        int exp = 1;
        for (int rem = maxKey; rem > 0; rem /= 10)
        {
            int count[10] = {0};
            for (int i = 0; i < n; ++i)
                ++count[(keyOf(a[i]) / exp) % 10];
            for (int d = 1; d < 10; ++d)
                count[d] += count[d - 1];
            for (int i = n - 1; i >= 0; --i)
            {
                out[--count[(keyOf(a[i]) / exp) % 10]] = a[i];
                ++st.moves;
            }
            for (int i = 0; i < n; ++i)
                a[i] = out[i];
            if (rem >= 10)
                exp *= 10; // avoids int overflow after the last digit
        }
        return st;
    }

    template <typename T, typename Less>
    bool isSorted(const T *a, int n, Less less)
    {
        for (int i = 1; i < n; ++i)
            if (less(a[i], a[i - 1]))
                return false;
        return true;
    }

} // namespace ums