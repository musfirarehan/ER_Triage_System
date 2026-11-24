// HistoryList.h - MODULE 3 (Member 3): doubly linked list of treated patients.
// Records are only appended (O(1) via tail pointer) and traversed either way.
#ifndef HISTORYLIST_H
#define HISTORYLIST_H

#include "Patient.h"

class HistoryList {
public:
    struct Node {
        Record rec;
        Node*  prev;
        Node*  next;
        explicit Node(const Record& r) : rec(r), prev(nullptr), next(nullptr) {}
    };

    HistoryList() : head_(nullptr), tail_(nullptr), size_(0) {}
    ~HistoryList() { clear(); }

    int  size()  const { return size_; }
    bool empty() const { return size_ == 0; }

    const Node* head() const { return head_; }   // oldest
    const Node* tail() const { return tail_; }   // newest

    void append(const Record& r) {
        Node* n = new Node(r);
        if (tail_) { tail_->next = n; n->prev = tail_; tail_ = n; }
        else       { head_ = tail_ = n; }
        ++size_;
    }

    void clear() {
        Node* n = head_;
        while (n) { Node* nx = n->next; delete n; n = nx; }
        head_ = tail_ = nullptr;
        size_ = 0;
    }

private:
    Node* head_;
    Node* tail_;
    int   size_;

    HistoryList(const HistoryList&) = delete;
    HistoryList& operator=(const HistoryList&) = delete;
};

#endif
