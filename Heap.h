// Heap.h - Binary heap priority queue. The comparator HigherPriority(a, b)
// must return true when `a` should be served BEFORE `b`.
//   * max-heap on CGPA  -> merit list / scholarship ranking
//   * min-heap on dist  -> Dijkstra's shortest path
#pragma once
#include <utility>
#include "ADT.h"

using namespace std;

namespace ums
{

    template <typename T, typename HigherPriority>
    class PriorityQueue
    {
    public:
        PriorityQueue() {}
        explicit PriorityQueue(HigherPriority hp) : hp_(hp) {}

        // Floyd's bottom-up heap construction, O(n).
        void build(const T *arr, int n)
        {
            a_.clear();
            for (int i = 0; i < n; ++i)
                a_.push_back(arr[i]);
            for (int i = n / 2 - 1; i >= 0; --i)
                down(i);
        }

        void push(const T &v)
        {
            a_.push_back(v);
            up(a_.size() - 1);
        }
        bool pop(T &out)
        {
            if (a_.empty())
                return false;
            out = a_[0];
            a_[0] = a_[a_.size() - 1];
            a_.pop_back();
            if (!a_.empty())
                down(0);
            return true;
        }
        bool top(T &out) const
        {
            if (a_.empty())
                return false;
            out = a_[0];
            return true;
        }
        int size() const { return a_.size(); }
        bool empty() const { return a_.empty(); }
        void clear() { a_.clear(); }

    private:
        void up(int i)
        {
            while (i > 0)
            {
                int p = (i - 1) / 2;
                if (hp_(a_[i], a_[p]))
                {
                    swap(a_[i], a_[p]);
                    i = p;
                }
                else
                {
                    break;
                }
            }
        }
        void down(int i)
        {
            int n = a_.size();
            while (true)
            {
                int l = 2 * i + 1, r = l + 1, best = i;
                if (l < n && hp_(a_[l], a_[best]))
                    best = l;
                if (r < n && hp_(a_[r], a_[best]))
                    best = r;
                if (best == i)
                    break;
                swap(a_[i], a_[best]);
                i = best;
            }
        }
        DynArray<T> a_;
        HigherPriority hp_;
    };

} // namespace ums