// MultiwayTrees.h - 2-3 tree and B-tree (int key -> int value).
// Each node models a disk page, so "nodes visited" == "page reads" for a lookup.
#pragma once
#include <algorithm>
#include <iostream>
#include <string>
#include <utility>
#include "ADT.h"

using namespace std;

namespace ums
{

    // 2-3 tree : every internal node has 2 or 3 children, all leaves at one depth.
    class TwoThreeTree
    {
        struct Node
        {
            int nk; // number of keys (1 or 2)
            int k[2];
            int v[2];
            Node *c[3];
            Node() : nk(0)
            {
                k[0] = k[1] = v[0] = v[1] = 0;
                c[0] = c[1] = c[2] = nullptr;
            }
            bool leaf() const { return c[0] == nullptr; }
        };
        struct Split
        {
            bool happened;
            int k, v;
            Node *right;
        };

    public:
        TwoThreeTree() : root_(nullptr), size_(0), visited_(0) {}
        TwoThreeTree(const TwoThreeTree &o) : root_(clone(o.root_)), size_(o.size_), visited_(0) {}
        TwoThreeTree &operator=(const TwoThreeTree &o)
        {
            if (this != &o)
            {
                TwoThreeTree tmp(o);
                swap(root_, tmp.root_);
                swap(size_, tmp.size_);
            }
            return *this;
        }
        ~TwoThreeTree() { destroy(root_); }

        bool insert(int key, int val)
        {
            Node *hit = nullptr;
            int slot = 0;
            if (locate(key, hit, slot))
            {
                hit->v[slot] = val;
                return false;
            }
            if (!root_)
            {
                root_ = new Node();
                root_->nk = 1;
                root_->k[0] = key;
                root_->v[0] = val;
                ++size_;
                return true;
            }
            Split s = insertRec(root_, key, val);
            if (s.happened)
            { // root split => tree grows one level
                Node *nr = new Node();
                nr->nk = 1;
                nr->k[0] = s.k;
                nr->v[0] = s.v;
                nr->c[0] = root_;
                nr->c[1] = s.right;
                root_ = nr;
            }
            ++size_;
            return true;
        }
        bool find(int key, int &val) const
        {
            Node *n = nullptr;
            int slot = 0;
            if (!locate(key, n, slot))
                return false;
            val = n->v[slot];
            return true;
        }
        void clear()
        {
            destroy(root_);
            root_ = nullptr;
            size_ = 0;
        }
        void rangeQuery(int lo, int hi, DynArray<int> &keys, DynArray<int> &vals) const
        {
            visited_ = 0;
            rangeRec(root_, lo, hi, keys, vals);
        }
        void inorder(DynArray<int> &keys) const { inorderRec(root_, keys); }
        // True when all leaves sit at the same depth (the defining 2-3 property).
        bool leavesAtSameDepth() const
        {
            int depth = -1;
            return depthCheck(root_, 0, depth);
        }

        int size() const { return size_; }
        int height() const
        {
            int h = 0;
            for (const Node *n = root_; n; n = n->c[0])
                ++h;
            return h;
        }
        int nodeCount() const { return countRec(root_); }
        int lastVisited() const { return visited_; }
        void print(ostream &os, int maxDepth = 4) const { printRec(os, root_, 0, maxDepth); }

    private:
        bool locate(int key, Node *&node, int &slot) const
        {
            visited_ = 0;
            Node *x = root_;
            while (x)
            {
                ++visited_;
                if (key == x->k[0])
                {
                    node = x;
                    slot = 0;
                    return true;
                }
                if (x->nk == 2 && key == x->k[1])
                {
                    node = x;
                    slot = 1;
                    return true;
                }
                if (key < x->k[0])
                    x = x->c[0];
                else if (x->nk == 1 || key < x->k[1])
                    x = x->c[1];
                else
                    x = x->c[2];
            }
            return false;
        }

        // Inserts into the subtree rooted at n; reports whether n had to split.
        static Split insertRec(Node *n, int key, int val)
        {
            Split none{false, 0, 0, nullptr};
            const bool isLeaf = n->leaf();
            const int nk = n->nk;
            int tk[3], tv[3];
            Node *tc[4] = {nullptr, nullptr, nullptr, nullptr};

            int pos = 0;
            if (isLeaf)
            {
                while (pos < nk && n->k[pos] < key)
                    ++pos;
                for (int i = 0, j = 0; i < nk + 1; ++i)
                {
                    if (i == pos)
                    {
                        tk[i] = key;
                        tv[i] = val;
                    }
                    else
                    {
                        tk[i] = n->k[j];
                        tv[i] = n->v[j];
                        ++j;
                    }
                }
            }
            else
            {
                while (pos < nk && key > n->k[pos])
                    ++pos; // child to descend into
                Split s = insertRec(n->c[pos], key, val);
                if (!s.happened)
                    return none;
                // the child split: absorb the promoted key and new right child
                for (int i = 0, j = 0; i < nk + 1; ++i)
                {
                    if (i == pos)
                    {
                        tk[i] = s.k;
                        tv[i] = s.v;
                    }
                    else
                    {
                        tk[i] = n->k[j];
                        tv[i] = n->v[j];
                        ++j;
                    }
                }
                for (int i = 0, j = 0; i < nk + 2; ++i)
                {
                    if (i == pos + 1)
                        tc[i] = s.right;
                    else
                        tc[i] = n->c[j++];
                }
            }

            const int m = nk + 1; // keys now held temporarily
            if (m <= 2)
            { // fits, no split
                n->nk = m;
                for (int i = 0; i < m; ++i)
                {
                    n->k[i] = tk[i];
                    n->v[i] = tv[i];
                }
                if (!isLeaf)
                    for (int i = 0; i <= m; ++i)
                        n->c[i] = tc[i];
                return none;
            }
            // 3 keys: split into two 1-key nodes and promote the middle key
            Node *right = new Node();
            n->nk = 1;
            n->k[0] = tk[0];
            n->v[0] = tv[0];
            right->nk = 1;
            right->k[0] = tk[2];
            right->v[0] = tv[2];
            if (!isLeaf)
            {
                n->c[0] = tc[0];
                n->c[1] = tc[1];
                n->c[2] = nullptr;
                right->c[0] = tc[2];
                right->c[1] = tc[3];
            }
            return Split{true, tk[1], tv[1], right};
        }

        static void destroy(Node *n)
        {
            if (!n)
                return;
            for (int i = 0; i < 3; ++i)
                destroy(n->c[i]);
            delete n;
        }
        static Node *clone(const Node *n)
        {
            if (!n)
                return nullptr;
            Node *c = new Node();
            c->nk = n->nk;
            for (int i = 0; i < 2; ++i)
            {
                c->k[i] = n->k[i];
                c->v[i] = n->v[i];
            }
            for (int i = 0; i < 3; ++i)
                c->c[i] = clone(n->c[i]);
            return c;
        }
        static int countRec(const Node *n)
        {
            if (!n)
                return 0;
            return 1 + countRec(n->c[0]) + countRec(n->c[1]) + countRec(n->c[2]);
        }
        static bool depthCheck(const Node *n, int d, int &leafDepth)
        {
            if (!n)
                return true;
            if (n->leaf())
            {
                if (leafDepth < 0)
                    leafDepth = d;
                return leafDepth == d;
            }
            for (int i = 0; i <= n->nk; ++i)
                if (!n->c[i] || !depthCheck(n->c[i], d + 1, leafDepth))
                    return false;
            return true;
        }
        void rangeRec(const Node *n, int lo, int hi, DynArray<int> &keys, DynArray<int> &vals) const
        {
            if (!n)
                return;
            ++visited_;
            if (lo < n->k[0])
                rangeRec(n->c[0], lo, hi, keys, vals);
            if (n->k[0] >= lo && n->k[0] <= hi)
            {
                keys.push_back(n->k[0]);
                vals.push_back(n->v[0]);
            }
            if (n->nk == 2)
            {
                if (lo < n->k[1] && hi > n->k[0])
                    rangeRec(n->c[1], lo, hi, keys, vals);
                if (n->k[1] >= lo && n->k[1] <= hi)
                {
                    keys.push_back(n->k[1]);
                    vals.push_back(n->v[1]);
                }
                if (hi > n->k[1])
                    rangeRec(n->c[2], lo, hi, keys, vals);
            }
            else if (hi > n->k[0])
            {
                rangeRec(n->c[1], lo, hi, keys, vals);
            }
        }
        static void inorderRec(const Node *n, DynArray<int> &keys)
        {
            if (!n)
                return;
            inorderRec(n->c[0], keys);
            keys.push_back(n->k[0]);
            inorderRec(n->c[1], keys);
            if (n->nk == 2)
            {
                keys.push_back(n->k[1]);
                inorderRec(n->c[2], keys);
            }
        }
        static void printRec(ostream &os, const Node *n, int depth, int maxDepth)
        {
            if (!n)
                return;
            os << string(depth * 4, ' ') << "[" << n->k[0];
            if (n->nk == 2)
                os << " | " << n->k[1];
            os << "]\n";
            if (depth + 1 >= maxDepth)
            {
                if (!n->leaf())
                    os << string((depth + 1) * 4, ' ') << "...\n";
                return;
            }
            for (int i = 0; i <= n->nk; ++i)
                printRec(os, n->c[i], depth + 1, maxDepth);
        }

        Node *root_;
        int size_;
        mutable int visited_;
    };

    // B-tree of minimum degree T (each node holds T-1 .. 2T-1 keys).
    // With T = 3 a node holds up to 5 keys: a tiny stand-in for a disk page.
    class BTree
    {
        static constexpr int T = 3;
        struct Node
        {
            int n;
            bool leaf;
            int keys[2 * T - 1];
            int vals[2 * T - 1];
            Node *ch[2 * T];
            explicit Node(bool lf) : n(0), leaf(lf)
            {
                for (int i = 0; i < 2 * T - 1; ++i)
                    keys[i] = vals[i] = 0;
                for (int i = 0; i < 2 * T; ++i)
                    ch[i] = nullptr;
            }
        };

    public:
        BTree() : root_(nullptr), size_(0), visited_(0) {}
        BTree(const BTree &o) : root_(clone(o.root_)), size_(o.size_), visited_(0) {}
        BTree &operator=(const BTree &o)
        {
            if (this != &o)
            {
                BTree tmp(o);
                swap(root_, tmp.root_);
                swap(size_, tmp.size_);
            }
            return *this;
        }
        ~BTree() { destroy(root_); }

        bool insert(int key, int val)
        {
            Node *hit = nullptr;
            int slot = 0;
            if (locate(key, hit, slot))
            {
                hit->vals[slot] = val;
                return false;
            }
            if (!root_)
            {
                root_ = new Node(true);
                root_->keys[0] = key;
                root_->vals[0] = val;
                root_->n = 1;
                ++size_;
                return true;
            }
            if (root_->n == 2 * T - 1)
            { // full root: split first, tree grows upward
                Node *s = new Node(false);
                s->ch[0] = root_;
                splitChild(s, 0);
                root_ = s;
            }
            insertNonFull(root_, key, val);
            ++size_;
            return true;
        }
        bool find(int key, int &val) const
        {
            Node *n = nullptr;
            int slot = 0;
            if (!locate(key, n, slot))
                return false;
            val = n->vals[slot];
            return true;
        }
        void clear()
        {
            destroy(root_);
            root_ = nullptr;
            size_ = 0;
        }
        void rangeQuery(int lo, int hi, DynArray<int> &keys, DynArray<int> &vals) const
        {
            visited_ = 0;
            rangeRec(root_, lo, hi, keys, vals);
        }
        void inorder(DynArray<int> &keys) const { inorderRec(root_, keys); }
        bool leavesAtSameDepth() const
        {
            int depth = -1;
            return depthCheck(root_, 0, depth);
        }

        int size() const { return size_; }
        int height() const
        {
            int h = 0;
            for (const Node *n = root_; n; n = n->leaf ? nullptr : n->ch[0])
                ++h;
            return h;
        }
        int nodeCount() const { return countRec(root_); }
        int lastVisited() const { return visited_; }
        static int maxKeysPerNode() { return 2 * T - 1; }
        void print(ostream &os, int maxDepth = 4) const { printRec(os, root_, 0, maxDepth); }

    private:
        bool locate(int key, Node *&node, int &slot) const
        {
            visited_ = 0;
            Node *x = root_;
            while (x)
            {
                ++visited_; // one "page read"
                int i = 0;
                while (i < x->n && key > x->keys[i])
                    ++i;
                if (i < x->n && key == x->keys[i])
                {
                    node = x;
                    slot = i;
                    return true;
                }
                if (x->leaf)
                    return false;
                x = x->ch[i];
            }
            return false;
        }
        static void splitChild(Node *x, int i)
        {
            Node *y = x->ch[i];
            Node *z = new Node(y->leaf);
            z->n = T - 1;
            for (int j = 0; j < T - 1; ++j)
            {
                z->keys[j] = y->keys[j + T];
                z->vals[j] = y->vals[j + T];
            }
            if (!y->leaf)
                for (int j = 0; j < T; ++j)
                    z->ch[j] = y->ch[j + T];
            y->n = T - 1;
            for (int j = x->n; j >= i + 1; --j)
                x->ch[j + 1] = x->ch[j];
            x->ch[i + 1] = z;
            for (int j = x->n - 1; j >= i; --j)
            {
                x->keys[j + 1] = x->keys[j];
                x->vals[j + 1] = x->vals[j];
            }
            x->keys[i] = y->keys[T - 1];
            x->vals[i] = y->vals[T - 1];
            ++x->n;
        }
        static void insertNonFull(Node *x, int k, int v)
        {
            int i = x->n - 1;
            if (x->leaf)
            {
                while (i >= 0 && k < x->keys[i])
                {
                    x->keys[i + 1] = x->keys[i];
                    x->vals[i + 1] = x->vals[i];
                    --i;
                }
                x->keys[i + 1] = k;
                x->vals[i + 1] = v;
                ++x->n;
                return;
            }
            while (i >= 0 && k < x->keys[i])
                --i;
            ++i;
            if (x->ch[i]->n == 2 * T - 1)
            {
                splitChild(x, i);
                if (k > x->keys[i])
                    ++i;
            }
            insertNonFull(x->ch[i], k, v);
        }

        static void destroy(Node *n)
        {
            if (!n)
                return;
            if (!n->leaf)
                for (int i = 0; i <= n->n; ++i)
                    destroy(n->ch[i]);
            delete n;
        }
        static Node *clone(const Node *n)
        {
            if (!n)
                return nullptr;
            Node *c = new Node(n->leaf);
            c->n = n->n;
            for (int i = 0; i < n->n; ++i)
            {
                c->keys[i] = n->keys[i];
                c->vals[i] = n->vals[i];
            }
            if (!n->leaf)
                for (int i = 0; i <= n->n; ++i)
                    c->ch[i] = clone(n->ch[i]);
            return c;
        }
        static int countRec(const Node *n)
        {
            if (!n)
                return 0;
            int c = 1;
            if (!n->leaf)
                for (int i = 0; i <= n->n; ++i)
                    c += countRec(n->ch[i]);
            return c;
        }
        static bool depthCheck(const Node *n, int d, int &leafDepth)
        {
            if (!n)
                return true;
            if (n->leaf)
            {
                if (leafDepth < 0)
                    leafDepth = d;
                return leafDepth == d;
            }
            for (int i = 0; i <= n->n; ++i)
                if (!depthCheck(n->ch[i], d + 1, leafDepth))
                    return false;
            return true;
        }
        void rangeRec(const Node *n, int lo, int hi, DynArray<int> &keys, DynArray<int> &vals) const
        {
            if (!n)
                return;
            ++visited_;
            for (int i = 0; i < n->n; ++i)
            {
                if (!n->leaf && lo < n->keys[i])
                    rangeRec(n->ch[i], lo, hi, keys, vals);
                if (n->keys[i] >= lo && n->keys[i] <= hi)
                {
                    keys.push_back(n->keys[i]);
                    vals.push_back(n->vals[i]);
                }
            }
            if (!n->leaf && (n->n == 0 || hi > n->keys[n->n - 1]))
                rangeRec(n->ch[n->n], lo, hi, keys, vals);
        }
        static void inorderRec(const Node *n, DynArray<int> &keys)
        {
            if (!n)
                return;
            for (int i = 0; i < n->n; ++i)
            {
                if (!n->leaf)
                    inorderRec(n->ch[i], keys);
                keys.push_back(n->keys[i]);
            }
            if (!n->leaf)
                inorderRec(n->ch[n->n], keys);
        }
        static void printRec(ostream &os, const Node *n, int depth, int maxDepth)
        {
            if (!n)
                return;
            os << string(depth * 4, ' ') << "[";
            for (int i = 0; i < n->n; ++i)
                os << (i ? " " : "") << n->keys[i];
            os << "]\n";
            if (n->leaf)
                return;
            if (depth + 1 >= maxDepth)
            {
                os << string((depth + 1) * 4, ' ') << "...\n";
                return;
            }
            for (int i = 0; i <= n->n; ++i)
                printRec(os, n->ch[i], depth + 1, maxDepth);
        }

        Node *root_;
        int size_;
        mutable int visited_;
    };

} // namespace ums