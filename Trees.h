// Trees.h - Binary Search Tree and height-balanced AVL tree (int key -> int value).
// Both support insert, find, erase, range queries, and report how many nodes a
// lookup visited so the effect of balancing can be demonstrated.
#pragma once
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>
#include "ADT.h"

using namespace std;

namespace ums
{

    // Plain (unbalanced) Binary Search Tree
    class BST
    {
        struct Node
        {
            int key, val;
            Node *l;
            Node *r;
            Node(int k, int v) : key(k), val(v), l(nullptr), r(nullptr) {}
        };

    public:
        BST() : root_(nullptr), size_(0), visited_(0) {}
        BST(const BST &o) : root_(clone(o.root_)), size_(o.size_), visited_(0) {}
        BST &operator=(const BST &o)
        {
            if (this != &o)
            {
                BST tmp(o);
                swap(root_, tmp.root_);
                swap(size_, tmp.size_);
            }
            return *this;
        }
        ~BST() { destroy(root_); }

        bool insert(int key, int val)
        {
            bool added = false;
            root_ = insertRec(root_, key, val, added);
            if (added)
                ++size_;
            return added;
        }
        bool find(int key, int &val) const
        {
            visited_ = 0;
            const Node *n = root_;
            while (n)
            {
                ++visited_;
                if (key == n->key)
                {
                    val = n->val;
                    return true;
                }
                n = key < n->key ? n->l : n->r;
            }
            return false;
        }
        bool erase(int key)
        {
            bool removed = false;
            root_ = eraseRec(root_, key, removed);
            if (removed)
                --size_;
            return removed;
        }
        void clear()
        {
            destroy(root_);
            root_ = nullptr;
            size_ = 0;
        }
        // Collects every (key, value) with lo <= key <= hi in ascending key order.
        void rangeQuery(int lo, int hi, DynArray<int> &keys, DynArray<int> &vals) const
        {
            visited_ = 0;
            rangeRec(root_, lo, hi, keys, vals);
        }
        void inorder(DynArray<int> &keys) const { inorderRec(root_, keys); }

        int size() const { return size_; }
        int height() const { return heightRec(root_); }
        int lastVisited() const { return visited_; }

        // Prints the top `maxDepth` levels sideways (right subtree on top).
        void printTop(ostream &os, int maxDepth = 4) const { printRec(os, root_, 0, maxDepth); }

    private:
        static void destroy(Node *n)
        {
            if (!n)
                return;
            destroy(n->l);
            destroy(n->r);
            delete n;
        }
        static Node *clone(const Node *n)
        {
            if (!n)
                return nullptr;
            Node *c = new Node(n->key, n->val);
            c->l = clone(n->l);
            c->r = clone(n->r);
            return c;
        }
        static Node *insertRec(Node *n, int k, int v, bool &added)
        {
            if (!n)
            {
                added = true;
                return new Node(k, v);
            }
            if (k < n->key)
                n->l = insertRec(n->l, k, v, added);
            else if (k > n->key)
                n->r = insertRec(n->r, k, v, added);
            else
                n->val = v;
            return n;
        }
        static Node *eraseRec(Node *n, int k, bool &removed)
        {
            if (!n)
                return nullptr;
            if (k < n->key)
                n->l = eraseRec(n->l, k, removed);
            else if (k > n->key)
                n->r = eraseRec(n->r, k, removed);
            else
            {
                removed = true;
                if (!n->l)
                {
                    Node *r = n->r;
                    delete n;
                    return r;
                }
                if (!n->r)
                {
                    Node *l = n->l;
                    delete n;
                    return l;
                }
                Node *m = n->r;
                while (m->l)
                    m = m->l; // in-order successor
                n->key = m->key;
                n->val = m->val;
                bool dummy = false;
                n->r = eraseRec(n->r, m->key, dummy);
            }
            return n;
        }
        void rangeRec(const Node *n, int lo, int hi, DynArray<int> &keys, DynArray<int> &vals) const
        {
            if (!n)
                return;
            ++visited_;
            if (lo < n->key)
                rangeRec(n->l, lo, hi, keys, vals);
            if (n->key >= lo && n->key <= hi)
            {
                keys.push_back(n->key);
                vals.push_back(n->val);
            }
            if (hi > n->key)
                rangeRec(n->r, lo, hi, keys, vals);
        }
        static void inorderRec(const Node *n, DynArray<int> &keys)
        {
            if (!n)
                return;
            inorderRec(n->l, keys);
            keys.push_back(n->key);
            inorderRec(n->r, keys);
        }
        static int heightRec(const Node *n) { return n ? 1 + max(heightRec(n->l), heightRec(n->r)) : 0; }
        static void printRec(ostream &os, const Node *n, int depth, int maxDepth)
        {
            if (!n)
                return;
            if (depth >= maxDepth)
            {
                os << string(depth * 4, ' ') << "...\n";
                return;
            }
            printRec(os, n->r, depth + 1, maxDepth);
            os << string(depth * 4, ' ') << n->key << "\n";
            printRec(os, n->l, depth + 1, maxDepth);
        }

        Node *root_;
        int size_;
        mutable int visited_;
    };

    // AVL tree : self-balancing BST (balance factor kept in {-1, 0, +1})
    class AVLTree
    {
        struct Node
        {
            int key, val, h;
            Node *l;
            Node *r;
            Node(int k, int v) : key(k), val(v), h(1), l(nullptr), r(nullptr) {}
        };

    public:
        AVLTree() : root_(nullptr), size_(0), visited_(0), rotations_(0) {}
        AVLTree(const AVLTree &o) : root_(clone(o.root_)), size_(o.size_), visited_(0), rotations_(o.rotations_) {}
        AVLTree &operator=(const AVLTree &o)
        {
            if (this != &o)
            {
                AVLTree tmp(o);
                swap(root_, tmp.root_);
                swap(size_, tmp.size_);
                swap(rotations_, tmp.rotations_);
            }
            return *this;
        }
        ~AVLTree() { destroy(root_); }

        bool insert(int key, int val)
        {
            bool added = false;
            root_ = insertRec(root_, key, val, added);
            if (added)
                ++size_;
            return added;
        }
        bool find(int key, int &val) const
        {
            visited_ = 0;
            const Node *n = root_;
            while (n)
            {
                ++visited_;
                if (key == n->key)
                {
                    val = n->val;
                    return true;
                }
                n = key < n->key ? n->l : n->r;
            }
            return false;
        }
        bool erase(int key)
        {
            bool removed = false;
            root_ = eraseRec(root_, key, removed);
            if (removed)
                --size_;
            return removed;
        }
        void clear()
        {
            destroy(root_);
            root_ = nullptr;
            size_ = 0;
            rotations_ = 0;
        }
        void rangeQuery(int lo, int hi, DynArray<int> &keys, DynArray<int> &vals) const
        {
            visited_ = 0;
            rangeRec(root_, lo, hi, keys, vals);
        }
        void inorder(DynArray<int> &keys) const { inorderRec(root_, keys); }
        // True if every node satisfies |balance factor| <= 1 (used by the self-tests).
        bool isBalanced() const { return checkBalance(root_); }

        int size() const { return size_; }
        int height() const { return hgt(root_); }
        int lastVisited() const { return visited_; }
        int rotations() const { return rotations_; }
        void printTop(ostream &os, int maxDepth = 4) const { printRec(os, root_, 0, maxDepth); }

    private:
        static int hgt(const Node *n) { return n ? n->h : 0; }
        static void upd(Node *n) { n->h = 1 + max(hgt(n->l), hgt(n->r)); }
        static int bal(const Node *n) { return hgt(n->l) - hgt(n->r); }
        static void destroy(Node *n)
        {
            if (!n)
                return;
            destroy(n->l);
            destroy(n->r);
            delete n;
        }
        static Node *clone(const Node *n)
        {
            if (!n)
                return nullptr;
            Node *c = new Node(n->key, n->val);
            c->h = n->h;
            c->l = clone(n->l);
            c->r = clone(n->r);
            return c;
        }
        Node *rotateRight(Node *y)
        {
            Node *x = y->l;
            y->l = x->r;
            x->r = y;
            upd(y);
            upd(x);
            ++rotations_;
            return x;
        }
        Node *rotateLeft(Node *x)
        {
            Node *y = x->r;
            x->r = y->l;
            y->l = x;
            upd(x);
            upd(y);
            ++rotations_;
            return y;
        }
        Node *rebalance(Node *n)
        {
            upd(n);
            int b = bal(n);
            if (b > 1)
            { // left heavy
                if (bal(n->l) < 0)
                    n->l = rotateLeft(n->l); // left-right case
                return rotateRight(n);
            }
            if (b < -1)
            { // right heavy
                if (bal(n->r) > 0)
                    n->r = rotateRight(n->r); // right-left case
                return rotateLeft(n);
            }
            return n;
        }
        Node *insertRec(Node *n, int k, int v, bool &added)
        {
            if (!n)
            {
                added = true;
                return new Node(k, v);
            }
            if (k < n->key)
                n->l = insertRec(n->l, k, v, added);
            else if (k > n->key)
                n->r = insertRec(n->r, k, v, added);
            else
            {
                n->val = v;
                return n;
            }
            return rebalance(n);
        }
        Node *eraseRec(Node *n, int k, bool &removed)
        {
            if (!n)
                return nullptr;
            if (k < n->key)
                n->l = eraseRec(n->l, k, removed);
            else if (k > n->key)
                n->r = eraseRec(n->r, k, removed);
            else
            {
                removed = true;
                if (!n->l || !n->r)
                {
                    Node *child = n->l ? n->l : n->r;
                    delete n;
                    return child; // may be nullptr; child subtree is already balanced
                }
                Node *m = n->r;
                while (m->l)
                    m = m->l;
                n->key = m->key;
                n->val = m->val;
                bool dummy = false;
                n->r = eraseRec(n->r, m->key, dummy);
            }
            return rebalance(n);
        }
        void rangeRec(const Node *n, int lo, int hi, DynArray<int> &keys, DynArray<int> &vals) const
        {
            if (!n)
                return;
            ++visited_;
            if (lo < n->key)
                rangeRec(n->l, lo, hi, keys, vals);
            if (n->key >= lo && n->key <= hi)
            {
                keys.push_back(n->key);
                vals.push_back(n->val);
            }
            if (hi > n->key)
                rangeRec(n->r, lo, hi, keys, vals);
        }
        static void inorderRec(const Node *n, DynArray<int> &keys)
        {
            if (!n)
                return;
            inorderRec(n->l, keys);
            keys.push_back(n->key);
            inorderRec(n->r, keys);
        }
        static bool checkBalance(const Node *n)
        {
            if (!n)
                return true;
            int b = bal(n);
            if (b < -1 || b > 1)
                return false;
            return checkBalance(n->l) && checkBalance(n->r);
        }
        static void printRec(ostream &os, const Node *n, int depth, int maxDepth)
        {
            if (!n)
                return;
            if (depth >= maxDepth)
            {
                os << string(depth * 4, ' ') << "...\n";
                return;
            }
            printRec(os, n->r, depth + 1, maxDepth);
            os << string(depth * 4, ' ') << n->key << " (h=" << n->h << ")\n";
            printRec(os, n->l, depth + 1, maxDepth);
        }

        Node *root_;
        int size_;
        mutable int visited_;
        int rotations_;
    };

} // namespace ums