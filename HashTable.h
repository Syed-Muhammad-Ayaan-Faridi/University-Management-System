// HashTable.h - Two int->int hash tables sharing the same division hash:
//   ChainedHashTable      : separate chaining (linked buckets)
//   OpenAddressHashTable  : open addressing with linear probing + tombstones
// Both rehash (to the next prime >= 2 * capacity) when the load factor is exceeded.
#pragma once
#include <iomanip>
#include <iostream>
#include <utility>

using namespace std;

namespace ums
{

    inline bool isPrimeNumber(int n)
    {
        if (n < 2)
            return false;
        for (int i = 2; i <= n / i; ++i) // same as i*i <= n, without overflow
            if (n % i == 0)
                return false;
        return true;
    }
    inline int nextPrime(int n)
    {
        while (!isPrimeNumber(n))
            ++n;
        return n;
    }
    inline int hashIndex(int key, int cap)
    {
        int m = key % cap;
        return m < 0 ? m + cap : m;
    }

    class ChainedHashTable
    {
        struct Node
        {
            int key, val;
            Node *next;
            Node(int k, int v, Node *n) : key(k), val(v), next(n) {}
        };

    public:
        explicit ChainedHashTable(int initialCap = 11)
            : table_(nullptr), cap_(nextPrime(initialCap < 3 ? 3 : initialCap)), size_(0),
              collisions_(0), rehashes_(0), lastProbes_(0)
        {
            table_ = new Node *[cap_]();
        }
        ChainedHashTable(const ChainedHashTable &o)
            : table_(new Node *[o.cap_]()), cap_(o.cap_), size_(o.size_), collisions_(o.collisions_),
              rehashes_(o.rehashes_), lastProbes_(0)
        {
            for (int i = 0; i < cap_; ++i)
            {
                Node *tail = nullptr;
                for (const Node *p = o.table_[i]; p; p = p->next)
                {
                    Node *n = new Node(p->key, p->val, nullptr);
                    if (tail)
                        tail->next = n;
                    else
                        table_[i] = n;
                    tail = n;
                }
            }
        }
        ChainedHashTable &operator=(const ChainedHashTable &o)
        {
            if (this != &o)
            {
                ChainedHashTable tmp(o);
                swap(table_, tmp.table_);
                swap(cap_, tmp.cap_);
                swap(size_, tmp.size_);
                swap(collisions_, tmp.collisions_);
                swap(rehashes_, tmp.rehashes_);
            }
            return *this;
        }
        ~ChainedHashTable() { release(); }

        // Returns true if a new key was added, false if an existing key was updated.
        bool insert(int key, int val)
        {
            for (Node *p = table_[hashIndex(key, cap_)]; p; p = p->next)
                if (p->key == key)
                {
                    p->val = val;
                    return false;
                }
            if ((size_ + 1) > 0.75 * cap_)
                rehash();
            int i = hashIndex(key, cap_);
            if (table_[i])
                ++collisions_; // bucket already occupied
            table_[i] = new Node(key, val, table_[i]);
            ++size_;
            return true;
        }
        bool find(int key, int &val) const
        {
            lastProbes_ = 0;
            for (const Node *p = table_[hashIndex(key, cap_)]; p; p = p->next)
            {
                ++lastProbes_;
                if (p->key == key)
                {
                    val = p->val;
                    return true;
                }
            }
            if (lastProbes_ == 0)
                lastProbes_ = 1; // looked at an empty bucket
            return false;
        }
        bool erase(int key)
        {
            int i = hashIndex(key, cap_);
            Node *prev = nullptr;
            for (Node *p = table_[i]; p; prev = p, p = p->next)
            {
                if (p->key == key)
                {
                    if (prev)
                        prev->next = p->next;
                    else
                        table_[i] = p->next;
                    delete p;
                    --size_;
                    return true;
                }
            }
            return false;
        }
        void clear()
        {
            release();
            cap_ = 11;
            table_ = new Node *[cap_]();
            size_ = 0;
            collisions_ = 0;
            rehashes_ = 0;
        }

        int size() const { return size_; }
        int capacity() const { return cap_; }
        double loadFactor() const { return (double)size_ / cap_; }
        int collisions() const { return collisions_; }
        int rehashes() const { return rehashes_; }
        int lastProbes() const { return lastProbes_; }

        void dump(ostream &os, int maxBuckets = 20) const
        {
            for (int i = 0; i < cap_ && i < maxBuckets; ++i)
            {
                os << "  [" << setw(2) << i << "] ";
                for (const Node *p = table_[i]; p; p = p->next)
                    os << p->key << " -> ";
                os << "null\n";
            }
            if (cap_ > maxBuckets)
                os << "  ... (" << cap_ - maxBuckets << " more buckets)\n";
        }

    private:
        void release()
        {
            for (int i = 0; i < cap_; ++i)
            {
                Node *p = table_[i];
                while (p)
                {
                    Node *n = p->next;
                    delete p;
                    p = n;
                }
            }
            delete[] table_;
            table_ = nullptr;
        }
        void rehash()
        {
            int newCap = nextPrime(cap_ * 2 + 1);
            Node **nt = new Node *[newCap]();
            for (int i = 0; i < cap_; ++i)
            {
                Node *p = table_[i];
                while (p)
                {
                    Node *nx = p->next;
                    int j = hashIndex(p->key, newCap);
                    p->next = nt[j];
                    nt[j] = p;
                    p = nx;
                }
            }
            delete[] table_;
            table_ = nt;
            cap_ = newCap;
            ++rehashes_;
        }

        Node **table_;
        int cap_;
        int size_;
        int collisions_;
        int rehashes_;
        mutable int lastProbes_;
    };

    class OpenAddressHashTable
    {
        enum State : unsigned char
        {
            EMPTY = 0,
            FULL = 1,
            DELETED = 2
        };

    public:
        explicit OpenAddressHashTable(int initialCap = 11)
            : keys_(nullptr), vals_(nullptr), state_(nullptr), cap_(nextPrime(initialCap < 3 ? 3 : initialCap)),
              size_(0), used_(0), collisions_(0), rehashes_(0), lastProbes_(0)
        {
            allocate(cap_);
        }
        OpenAddressHashTable(const OpenAddressHashTable &o)
            : keys_(nullptr), vals_(nullptr), state_(nullptr), cap_(o.cap_), size_(o.size_), used_(o.used_),
              collisions_(o.collisions_), rehashes_(o.rehashes_), lastProbes_(0)
        {
            allocate(cap_);
            for (int i = 0; i < cap_; ++i)
            {
                keys_[i] = o.keys_[i];
                vals_[i] = o.vals_[i];
                state_[i] = o.state_[i];
            }
        }
        OpenAddressHashTable &operator=(const OpenAddressHashTable &o)
        {
            if (this != &o)
            {
                OpenAddressHashTable tmp(o);
                swap(keys_, tmp.keys_);
                swap(vals_, tmp.vals_);
                swap(state_, tmp.state_);
                swap(cap_, tmp.cap_);
                swap(size_, tmp.size_);
                swap(used_, tmp.used_);
                swap(collisions_, tmp.collisions_);
                swap(rehashes_, tmp.rehashes_);
            }
            return *this;
        }
        ~OpenAddressHashTable() { release(); }

        bool insert(int key, int val)
        {
            int existing = locate(key);
            if (existing >= 0)
            {
                vals_[existing] = val;
                return false;
            }
            if ((used_ + 1) > 0.5 * cap_)
                rehash();
            int i = hashIndex(key, cap_);
            int firstDeleted = -1;
            while (state_[i] != EMPTY)
            {
                if (state_[i] == DELETED && firstDeleted < 0)
                    firstDeleted = i;
                ++collisions_; // had to step past an occupied slot
                i = (i + 1) % cap_;
            }
            int slot = firstDeleted >= 0 ? firstDeleted : i;
            if (state_[slot] == EMPTY)
                ++used_;
            keys_[slot] = key;
            vals_[slot] = val;
            state_[slot] = FULL;
            ++size_;
            return true;
        }
        bool find(int key, int &val) const
        {
            int i = locate(key);
            if (i < 0)
                return false;
            val = vals_[i];
            return true;
        }
        bool erase(int key)
        {
            int i = locate(key);
            if (i < 0)
                return false;
            state_[i] = DELETED; // tombstone keeps probe sequences intact
            --size_;
            return true;
        }
        void clear()
        {
            release();
            cap_ = 11;
            allocate(cap_);
            size_ = used_ = 0;
            collisions_ = 0;
            rehashes_ = 0;
        }

        int size() const { return size_; }
        int capacity() const { return cap_; }
        double loadFactor() const { return (double)size_ / cap_; }
        int collisions() const { return collisions_; }
        int rehashes() const { return rehashes_; }
        int lastProbes() const { return lastProbes_; }

        void dump(ostream &os, int maxSlots = 20) const
        {
            for (int i = 0; i < cap_ && i < maxSlots; ++i)
            {
                os << "  [" << setw(2) << i << "] ";
                if (state_[i] == FULL)
                    os << keys_[i];
                else if (state_[i] == DELETED)
                    os << "(deleted)";
                else
                    os << "-";
                os << "\n";
            }
            if (cap_ > maxSlots)
                os << "  ... (" << cap_ - maxSlots << " more slots)\n";
        }

    private:
        int locate(int key) const
        {
            lastProbes_ = 0;
            int i = hashIndex(key, cap_);
            for (int step = 0; step < cap_; ++step)
            {
                ++lastProbes_;
                if (state_[i] == EMPTY)
                    return -1;
                if (state_[i] == FULL && keys_[i] == key)
                    return i;
                i = (i + 1) % cap_;
            }
            return -1;
        }
        void allocate(int cap)
        {
            keys_ = new int[cap]();
            vals_ = new int[cap]();
            state_ = new unsigned char[cap]();
        }
        void release()
        {
            delete[] keys_;
            delete[] vals_;
            delete[] state_;
            keys_ = vals_ = nullptr;
            state_ = nullptr;
        }
        void rehash()
        {
            int oldCap = cap_;
            int *ok = keys_;
            int *ov = vals_;
            unsigned char *os = state_;
            cap_ = nextPrime(oldCap * 2 + 1);
            allocate(cap_);
            size_ = used_ = 0;
            int keepCollisions = collisions_;
            for (int i = 0; i < oldCap; ++i)
                if (os[i] == FULL)
                    insert(ok[i], ov[i]);
            collisions_ = keepCollisions; // do not count re-insertion steps as new collisions
            ++rehashes_;
            delete[] ok;
            delete[] ov;
            delete[] os;
        }

        int *keys_;
        int *vals_;
        unsigned char *state_;
        int cap_;
        int size_;
        int used_; // FULL + DELETED slots (what actually lengthens probe chains)
        int collisions_;
        int rehashes_;
        mutable int lastProbes_;
    };

} // namespace ums