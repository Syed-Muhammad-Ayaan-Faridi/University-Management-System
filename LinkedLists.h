// LinkedLists.h - Singly, doubly and circular linked lists (Rule of Three applied).
#pragma once
#include <stdexcept>
#include <utility>

using namespace std;

namespace ums
{

    // Singly linked list : used for each student's course enrollments.
    template <typename T>
    class SinglyLinkedList
    {
    public:
        struct Node
        {
            T value;
            Node *next;
            explicit Node(const T &v) : value(v), next(nullptr) {}
        };

        SinglyLinkedList() : head_(nullptr), tail_(nullptr), size_(0) {}
        SinglyLinkedList(const SinglyLinkedList &o) : head_(nullptr), tail_(nullptr), size_(0)
        {
            for (const Node *p = o.head_; p; p = p->next)
                pushBack(p->value);
        }
        SinglyLinkedList &operator=(const SinglyLinkedList &o)
        {
            if (this != &o)
            {
                SinglyLinkedList tmp(o);
                swap(tmp);
            }
            return *this;
        }
        ~SinglyLinkedList() { clear(); }

        // "::swap" is required: this class has a member named swap, so an
        // unqualified swap(a, b) would find the member first and fail to compile.
        void swap(SinglyLinkedList &o) noexcept
        {
            ::swap(head_, o.head_);
            ::swap(tail_, o.tail_);
            ::swap(size_, o.size_);
        }

        void pushFront(const T &v)
        {
            Node *n = new Node(v);
            n->next = head_;
            head_ = n;
            if (!tail_)
                tail_ = n;
            ++size_;
        }
        void pushBack(const T &v)
        {
            Node *n = new Node(v);
            if (tail_)
                tail_->next = n;
            else
                head_ = n;
            tail_ = n;
            ++size_;
        }
        bool popFront(T &out)
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
        // Removes the first element for which pred(value) is true.
        template <typename P>
        bool removeIf(P pred)
        {
            Node *prev = nullptr;
            for (Node *p = head_; p; prev = p, p = p->next)
            {
                if (pred(p->value))
                {
                    if (prev)
                        prev->next = p->next;
                    else
                        head_ = p->next;
                    if (p == tail_)
                        tail_ = prev;
                    delete p;
                    --size_;
                    return true;
                }
            }
            return false;
        }
        template <typename P>
        T *find(P pred)
        {
            for (Node *p = head_; p; p = p->next)
                if (pred(p->value))
                    return &p->value;
            return nullptr;
        }
        template <typename P>
        const T *find(P pred) const
        {
            for (const Node *p = head_; p; p = p->next)
                if (pred(p->value))
                    return &p->value;
            return nullptr;
        }
        void reverse()
        {
            Node *prev = nullptr;
            Node *cur = head_;
            tail_ = head_;
            while (cur)
            {
                Node *nx = cur->next;
                cur->next = prev;
                prev = cur;
                cur = nx;
            }
            head_ = prev;
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

        const Node *head() const { return head_; }
        int size() const { return size_; }
        bool empty() const { return size_ == 0; }

    private:
        Node *head_;
        Node *tail_;
        int size_;
    };

    // Doubly linked list with a movable cursor : forward / backward record browsing.
    template <typename T>
    class DoublyLinkedList
    {
    public:
        struct Node
        {
            T value;
            Node *prev;
            Node *next;
            explicit Node(const T &v) : value(v), prev(nullptr), next(nullptr) {}
        };

        DoublyLinkedList() : head_(nullptr), tail_(nullptr), cursor_(nullptr), size_(0), pos_(-1) {}
        DoublyLinkedList(const DoublyLinkedList &o) : head_(nullptr), tail_(nullptr), cursor_(nullptr), size_(0), pos_(-1)
        {
            for (const Node *p = o.head_; p; p = p->next)
                pushBack(p->value);
            resetCursor();
        }
        DoublyLinkedList &operator=(const DoublyLinkedList &o)
        {
            if (this != &o)
            {
                DoublyLinkedList tmp(o);
                swap(head_, tmp.head_);
                swap(tail_, tmp.tail_);
                swap(cursor_, tmp.cursor_);
                swap(size_, tmp.size_);
                swap(pos_, tmp.pos_);
            }
            return *this;
        }
        ~DoublyLinkedList() { clear(); }

        void pushBack(const T &v)
        {
            Node *n = new Node(v);
            n->prev = tail_;
            if (tail_)
                tail_->next = n;
            else
                head_ = n;
            tail_ = n;
            ++size_;
            if (!cursor_)
                resetCursor();
        }
        void pushFront(const T &v)
        {
            Node *n = new Node(v);
            n->next = head_;
            if (head_)
                head_->prev = n;
            else
                tail_ = n;
            head_ = n;
            ++size_;
            if (cursor_)
                ++pos_;
            else
                resetCursor();
        }
        void clear()
        {
            while (head_)
            {
                Node *n = head_;
                head_ = n->next;
                delete n;
            }
            tail_ = cursor_ = nullptr;
            size_ = 0;
            pos_ = -1;
        }

        void resetCursor()
        {
            cursor_ = head_;
            pos_ = head_ ? 0 : -1;
        }
        bool moveNext()
        {
            if (!cursor_ || !cursor_->next)
                return false;
            cursor_ = cursor_->next;
            ++pos_;
            return true;
        }
        bool movePrev()
        {
            if (!cursor_ || !cursor_->prev)
                return false;
            cursor_ = cursor_->prev;
            --pos_;
            return true;
        }
        T &current()
        {
            if (!cursor_)
                throw runtime_error("DoublyLinkedList: empty list");
            return cursor_->value;
        }
        int position() const { return pos_; }
        int size() const { return size_; }
        bool empty() const { return size_ == 0; }

    private:
        Node *head_;
        Node *tail_;
        Node *cursor_;
        int size_;
        int pos_;
    };

    // Circular singly linked list : round-robin rotation (exam invigilator duty).
    template <typename T>
    class CircularLinkedList
    {
    public:
        struct Node
        {
            T value;
            Node *next;
            explicit Node(const T &v) : value(v), next(nullptr) {}
        };

        CircularLinkedList() : tail_(nullptr), cur_(nullptr), size_(0) {}
        CircularLinkedList(const CircularLinkedList &o) : tail_(nullptr), cur_(nullptr), size_(0)
        {
            if (o.tail_)
            {
                const Node *p = o.tail_->next; // head
                for (int i = 0; i < o.size_; ++i, p = p->next)
                    add(p->value);
                // restore the rotation point of the source list
                const Node *src = o.tail_->next;
                Node *mine = tail_->next;
                for (int i = 0; i < o.size_; ++i, src = src->next, mine = mine->next)
                    if (src == o.cur_)
                    {
                        cur_ = mine;
                        break;
                    }
            }
        }
        CircularLinkedList &operator=(const CircularLinkedList &o)
        {
            if (this != &o)
            {
                CircularLinkedList tmp(o);
                swap(tail_, tmp.tail_);
                swap(cur_, tmp.cur_);
                swap(size_, tmp.size_);
            }
            return *this;
        }
        ~CircularLinkedList() { clear(); }

        // Appends a node at the end of the ring.
        void add(const T &v)
        {
            Node *n = new Node(v);
            if (!tail_)
            {
                n->next = n;
                cur_ = n;
            }
            else
            {
                n->next = tail_->next;
                tail_->next = n;
            }
            tail_ = n;
            ++size_;
        }
        // Returns the value at the rotation pointer and advances the pointer.
        T &next()
        {
            if (!cur_)
                throw runtime_error("CircularLinkedList: empty ring");
            T &v = cur_->value;
            cur_ = cur_->next;
            return v;
        }
        T &peek()
        {
            if (!cur_)
                throw runtime_error("CircularLinkedList: empty ring");
            return cur_->value;
        }
        bool remove(const T &v)
        {
            if (!tail_)
                return false;
            Node *prev = tail_;
            Node *p = tail_->next;
            for (int i = 0; i < size_; ++i, prev = p, p = p->next)
            {
                if (p->value == v)
                {
                    if (size_ == 1)
                    {
                        delete p;
                        tail_ = cur_ = nullptr;
                    }
                    else
                    {
                        prev->next = p->next;
                        if (p == tail_)
                            tail_ = prev;
                        if (p == cur_)
                            cur_ = p->next;
                        delete p;
                    }
                    --size_;
                    return true;
                }
            }
            return false;
        }
        void clear()
        {
            if (!tail_)
                return;
            Node *p = tail_->next;
            tail_->next = nullptr; // break the ring
            while (p)
            {
                Node *n = p->next;
                delete p;
                p = n;
            }
            tail_ = cur_ = nullptr;
            size_ = 0;
        }
        int size() const { return size_; }
        bool empty() const { return size_ == 0; }

    private:
        Node *tail_; // tail_->next is the head
        Node *cur_;  // rotation pointer
        int size_;
    };

} // namespace ums