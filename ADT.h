// ADT.h - Custom Abstract Data Types: DynArray, SafePtr, JaggedArray.
// Every class that owns raw dynamic memory follows the Rule of Three
// (destructor, copy constructor, copy-assignment) and performs deep copies.
#pragma once
#include <stdexcept>
#include <string>
#include <utility>

using namespace std;

namespace ums
{

    // DynArray<T> : bounds-checked growable array (our own vector replacement)
    template <typename T>
    class DynArray
    {
    public:
        DynArray() : data_(nullptr), size_(0), cap_(0) {}

        explicit DynArray(int n, const T &fill = T()) : data_(nullptr), size_(0), cap_(0)
        {
            if (n < 0)
                throw invalid_argument("DynArray: negative size");
            if (n > 0)
            {
                data_ = new T[n];
                cap_ = n;
                for (int i = 0; i < n; ++i)
                    data_[i] = fill;
                size_ = n;
            }
        }

        // Rule of Three (1/3): copy constructor performs a DEEP copy
        DynArray(const DynArray &o) : data_(nullptr), size_(0), cap_(0)
        {
            if (o.size_ > 0)
            {
                data_ = new T[o.size_];
                try
                {
                    for (int i = 0; i < o.size_; ++i)
                        data_[i] = o.data_[i];
                }
                catch (...)
                {
                    delete[] data_;
                    throw;
                }
                size_ = cap_ = o.size_;
            }
        }

        // Rule of Three (2/3): copy assignment (copy-and-swap => self-assignment & exception safe)
        DynArray &operator=(const DynArray &o)
        {
            if (this != &o)
            {
                DynArray tmp(o);
                swap(tmp);
            }
            return *this;
        }

        // Rule of Three (3/3): destructor releases the buffer
        ~DynArray() { 
            delete[] data_; 
        }

        // Move operations (an optimisation on top of the Rule of Three)
        DynArray(DynArray &&o) noexcept : data_(o.data_), size_(o.size_), cap_(o.cap_)
        {
            o.data_ = nullptr;
            o.size_ = o.cap_ = 0;
        }
        DynArray &operator=(DynArray &&o) noexcept
        {
            if (this != &o)
            {
                delete[] data_;
                data_ = o.data_;
                size_ = o.size_;
                cap_ = o.cap_;
                o.data_ = nullptr;
                o.size_ = o.cap_ = 0;
            }
            return *this;
        }

        void swap(DynArray &o) noexcept
        {
            ::swap(data_, o.data_);
            ::swap(size_, o.size_);
            ::swap(cap_, o.cap_);
        }

        int size() const { return size_; }
        int capacity() const { return cap_; }
        bool empty() const { return size_ == 0; }

        T &operator[](int i)
        {
            check(i);
            return data_[i];
        }
        const T &operator[](int i) const
        {
            check(i);
            return data_[i];
        }
        T &back()
        {
            check(size_ - 1);
            return data_[size_ - 1];
        }
        const T &back() const
        {
            check(size_ - 1);
            return data_[size_ - 1];
        }

        void push_back(const T &v)
        {
            T copy = v; // v might alias an element of this array
            if (size_ == cap_)
                grow(cap_ ? cap_ * 2 : 4);
            data_[size_++] = move(copy);
        }
        void pop_back()
        {
            check(size_ - 1);
            --size_;
        }
        void insertAt(int pos, const T &v)
        {
            if (pos < 0 || pos > size_)
                throw out_of_range("DynArray::insertAt position out of range");
            T copy = v;
            if (size_ == cap_)
                grow(cap_ ? cap_ * 2 : 4);
            for (int i = size_; i > pos; --i)
                data_[i] = move(data_[i - 1]);
            data_[pos] = move(copy);
            ++size_;
        }
        void removeAt(int pos)
        {
            check(pos);
            for (int i = pos; i < size_ - 1; ++i)
                data_[i] = move(data_[i + 1]);
            --size_;
        }
        void clear() { 
            size_ = 0; 
        }

        T *data() {
            return data_; 
        }
        const T *data() const {
            return data_; 
        }
        T *begin() {
            return data_; 
        }
        T *end() { 
            return data_ + size_; 
        }
        const T *begin() const { 
            return data_; 
        }
        const T *end() const { 
            return data_ + size_; 
        }

    private:
        void check(int i) const
        {
            if (i < 0 || i >= size_)
                throw out_of_range("DynArray index " + to_string(i) + " out of range [0," + to_string(size_) + ")");
        }
        void grow(int newCap)
        {
            T *nd = new T[newCap];
            for (int i = 0; i < size_; ++i)
                nd[i] = move(data_[i]);
            delete[] data_;
            data_ = nd;
            cap_ = newCap;
        }
        T *data_;
        int size_;
        int cap_;
    };

    // SafePtr<T> : single-owner pointer with null checks and deep-copy semantics.
    // Used for undo snapshots so that a saved record can never dangle or be shared.
    template <typename T>
    class SafePtr
    {
    public:
        SafePtr() : p_(nullptr) {}
        explicit SafePtr(T *p) : p_(p) {} // takes ownership
        SafePtr(const SafePtr &o) : p_(o.p_ ? new T(*o.p_) : nullptr) {}
        SafePtr &operator=(const SafePtr &o)
        {
            if (this != &o)
            {
                T *fresh = o.p_ ? new T(*o.p_) : nullptr;
                delete p_;
                p_ = fresh;
            }
            return *this;
        }
        ~SafePtr() { delete p_; }
        SafePtr(SafePtr &&o) noexcept : p_(o.p_) { o.p_ = nullptr; }
        SafePtr &operator=(SafePtr &&o) noexcept
        {
            if (this != &o)
            {
                delete p_;
                p_ = o.p_;
                o.p_ = nullptr;
            }
            return *this;
        }

        static SafePtr make(const T &value) { return SafePtr(new T(value)); }

        bool isNull() const { return p_ == nullptr; }
        void reset()
        {
            delete p_;
            p_ = nullptr;
        }

        T &operator*()
        {
            require();
            return *p_;
        }
        const T &operator*() const
        {
            require();
            return *p_;
        }
        T *operator->()
        {
            require();
            return p_;
        }
        const T *operator->() const
        {
            require();
            return p_;
        }

    private:
        void require() const
        {
            if (!p_)
                throw runtime_error("SafePtr: attempt to dereference a null pointer");
        }
        T *p_;
    };

    // JaggedArray<T> : array of rows where every row may have a different length.
    // Used for per-student grade rows and timetable rows (different course loads).
    template <typename T>
    class JaggedArray
    {
    public:
        JaggedArray() : rows_(nullptr), lens_(nullptr), count_(0), cap_(0) {}

        JaggedArray(const JaggedArray &o) : rows_(nullptr), lens_(nullptr), count_(0), cap_(0)
        {
            if (o.count_ > 0)
            {
                rows_ = new T *[o.count_]();
                lens_ = new int[o.count_]();
                cap_ = o.count_;
                for (int r = 0; r < o.count_; ++r)
                {
                    lens_[r] = o.lens_[r];
                    rows_[r] = lens_[r] > 0 ? new T[lens_[r]] : nullptr;
                    for (int c = 0; c < lens_[r]; ++c)
                        rows_[r][c] = o.rows_[r][c];
                    count_ = r + 1;
                }
            }
        }
        JaggedArray &operator=(const JaggedArray &o)
        {
            if (this != &o)
            {
                JaggedArray tmp(o);
                swap(tmp);
            }
            return *this;
        }
        ~JaggedArray()
        {
            for (int r = 0; r < count_; ++r)
                delete[] rows_[r];
            delete[] rows_;
            delete[] lens_;
        }

        void swap(JaggedArray &o) noexcept
        {
            ::swap(rows_, o.rows_);
            ::swap(lens_, o.lens_);
            ::swap(count_, o.count_);
            ::swap(cap_, o.cap_);
        }

        // Adds a new row of the requested length and returns its row index.
        int addRow(int length, const T &fill = T())
        {
            if (length < 0)
                throw invalid_argument("JaggedArray: negative row length");
            if (count_ == cap_)
            {
                int newCap = cap_ ? cap_ * 2 : 4;
                T **nr = new T *[newCap]();
                int *nl = new int[newCap]();
                for (int i = 0; i < count_; ++i)
                {
                    nr[i] = rows_[i];
                    nl[i] = lens_[i];
                }
                delete[] rows_;
                delete[] lens_;
                rows_ = nr;
                lens_ = nl;
                cap_ = newCap;
            }
            rows_[count_] = length > 0 ? new T[length] : nullptr;
            for (int c = 0; c < length; ++c)
                rows_[count_][c] = fill;
            lens_[count_] = length;
            return count_++;
        }

        int rowCount() const { return count_; }
        int rowLength(int r) const
        {
            checkRow(r);
            return lens_[r];
        }
        T &at(int r, int c)
        {
            checkCell(r, c);
            return rows_[r][c];
        }
        const T &at(int r, int c) const
        {
            checkCell(r, c);
            return rows_[r][c];
        }
        long long totalCells() const
        {
            long long t = 0;
            for (int r = 0; r < count_; ++r)
                t += lens_[r];
            return t;
        }

    private:
        void checkRow(int r) const
        {
            if (r < 0 || r >= count_)
                throw out_of_range("JaggedArray: row " + to_string(r) + " out of range");
        }
        void checkCell(int r, int c) const
        {
            checkRow(r);
            if (c < 0 || c >= lens_[r])
                throw out_of_range("JaggedArray: column " + to_string(c) + " out of range for row " + to_string(r));
        }
        T **rows_;
        int *lens_;
        int count_;
        int cap_;
    };

} // namespace ums