// StackQueue.h - Linked Stack (LIFO) and Queue (FIFO).
#pragma once
#include <utility>

using namespace std;

namespace ums
{

    // Stack<T> : LIFO, backs the undo history for edits and deletions.
    template <typename T>
    class Stack
    {
        struct Node
        {
            T value;
            Node *next;
            Node(const T &v, Node *n) : value(v), next(n) {}
        };

    public:
        Stack() : top_(nullptr), size_(0) {}
        Stack(const Stack &o) : top_(nullptr), size_(0)
        {
            Node *tail = nullptr; // copy top-to-bottom, preserving order
            for (const Node *p = o.top_; p; p = p->next)
            {
                Node *n = new Node(p->value, nullptr);
                if (tail)
                    tail->next = n;
                else
                    top_ = n;
                tail = n;
                ++size_;
            }
        }
        Stack &operator=(const Stack &o)
        {
            if (this != &o)
            {
                Stack tmp(o);
                swap(top_, tmp.top_);
                swap(size_, tmp.size_);
            }
            return *this;
        }
        ~Stack() { clear(); }

        void push(const T &v)
        {
            top_ = new Node(v, top_);
            ++size_;
        }
        bool pop(T &out)
        {
            if (!top_)
                return false;
            Node *n = top_;
            out = n->value;
            top_ = n->next;
            delete n;
            --size_;
            return true;
        }
        bool peek(T &out) const
        {
            if (!top_)
                return false;
            out = top_->value;
            return true;
        }
        void clear()
        {
            while (top_)
            {
                Node *n = top_;
                top_ = n->next;
                delete n;
            }
            size_ = 0;
        }
        int size() const { return size_; }
        bool empty() const { return size_ == 0; }

        // Visits elements from top (most recent) to bottom.
        template <typename F>
        void forEach(F f) const
        {
            for (const Node *p = top_; p; p = p->next)
                f(p->value);
        }

    private:
        Node *top_;
        int size_;
    };

    // Queue<T> : FIFO, backs the registration / fee-counter line.
    template <typename T>
    class Queue
    {
        struct Node
        {
            T value;
            Node *next;
            explicit Node(const T &v) : value(v), next(nullptr) {}
        };

    public:
        Queue() : head_(nullptr), tail_(nullptr), size_(0) {}
        Queue(const Queue &o) : head_(nullptr), tail_(nullptr), size_(0)
        {
            for (const Node *p = o.head_; p; p = p->next)
                enqueue(p->value);
        }
        Queue &operator=(const Queue &o)
        {
            if (this != &o)
            {
                Queue tmp(o);
                swap(head_, tmp.head_);
                swap(tail_, tmp.tail_);
                swap(size_, tmp.size_);
            }
            return *this;
        }
        ~Queue() { clear(); }

        void enqueue(const T &v)
        {
            Node *n = new Node(v);
            if (tail_)
                tail_->next = n;
            else
                head_ = n;
            tail_ = n;
            ++size_;
        }
        bool dequeue(T &out)
        {
            if (!head_)
                return false;
            Node *n = head_;
            out = n->value;
            head_ = n->next;
            if (!head_)
                tail_ = nullptr;
            delete n;
            --size_;
            return true;
        }
        bool front(T &out) const
        {
            if (!head_)
                return false;
            out = head_->value;
            return true;
        }
        void clear()
        {
            while (head_)
            {
                Node *n = head_;
                head_ = n->next;
                delete n;
            }
            tail_ = nullptr;
            size_ = 0;
        }
        int size() const { return size_; }
        bool empty() const { return size_ == 0; }

        // Visits elements from front (next to be served) to back.
        template <typename F>
        void forEach(F f) const
        {
            for (const Node *p = head_; p; p = p->next)
                f(p->value);
        }

    private:
        Node *head_;
        Node *tail_;
        int size_;
    };

} // namespace ums