// MinHeap.h - MODULE 1 (Member 1): binary min-heap of Patient pointers.
//
// Priority rule: lower severity first; equal severity -> lower arrival number
// first (first come, first served). Every patient remembers its own heap
// position (Patient::heapIndex) so remove/update need no searching.
//
//   insert ........ O(log n)      peek ......... O(1)
//   extractMin .... O(log n)      remove ....... O(log n)
//   update ........ O(log n)      grow ......... amortised O(1)
#ifndef MINHEAP_H
#define MINHEAP_H

#include "Patient.h"

class MinHeap {
public:
    // track=false -> used for temporary copies (does not touch heapIndex).
    explicit MinHeap(bool track = true, int initialCapacity = 16)
        : size_(0), cap_(initialCapacity < 4 ? 4 : initialCapacity), track_(track) {
        data_ = new Patient*[cap_];
    }
    ~MinHeap() { delete[] data_; }

    int  size()  const { return size_; }
    int  capacity() const { return cap_; }
    bool empty() const { return size_ == 0; }

    // Patient in array slot i (0 <= i < size). Used for enumeration.
    Patient* at(int i) const { return (i >= 0 && i < size_) ? data_[i] : nullptr; }

    void insert(Patient* p) {
        if (size_ == cap_) grow();
        data_[size_] = p;
        if (track_) p->heapIndex = size_;
        ++size_;
        siftUp(size_ - 1);
    }

    Patient* peek() const { return size_ == 0 ? nullptr : data_[0]; }

    Patient* extractMin() {
        if (size_ == 0) return nullptr;
        Patient* top = data_[0];
        removeAt(0);
        return top;
    }

    // Remove a specific patient (cancel). Returns false if p is not in this heap.
    bool remove(Patient* p) {
        if (!track_ || p == nullptr) return false;
        int i = p->heapIndex;
        if (i < 0 || i >= size_ || data_[i] != p) return false;
        removeAt(i);
        return true;
    }

    // Call after changing p->severity: restores heap order. Returns false if p not in heap.
    bool update(Patient* p) {
        if (!track_ || p == nullptr) return false;
        int i = p->heapIndex;
        if (i < 0 || i >= size_ || data_[i] != p) return false;
        siftUp(i);
        siftDown(p->heapIndex);
        return true;
    }

    void clear() {
        if (track_) for (int i = 0; i < size_; ++i) data_[i]->heapIndex = -1;
        size_ = 0;
    }

    // Debug/test helper: verifies heap property and stored indexes.
    bool isValid() const {
        for (int i = 0; i < size_; ++i) {
            if (track_ && data_[i]->heapIndex != i) return false;
            if (i > 0 && before(data_[i], data_[(i - 1) / 2])) return false;
        }
        return true;
    }

private:
    Patient** data_;
    int  size_;
    int  cap_;
    bool track_;

    MinHeap(const MinHeap&) = delete;
    MinHeap& operator=(const MinHeap&) = delete;

    static bool before(const Patient* a, const Patient* b) {
        if (a->severity != b->severity) return a->severity < b->severity;
        return a->arrival < b->arrival;
    }

    void place(int i, Patient* p) {
        data_[i] = p;
        if (track_) p->heapIndex = i;
    }

    void siftUp(int i) {
        Patient* p = data_[i];
        while (i > 0) {
            int parent = (i - 1) / 2;
            if (!before(p, data_[parent])) break;
            place(i, data_[parent]);
            i = parent;
        }
        place(i, p);
    }

    void siftDown(int i) {
        Patient* p = data_[i];
        for (;;) {
            int l = 2 * i + 1;
            if (l >= size_) break;
            int r = l + 1;
            int c = (r < size_ && before(data_[r], data_[l])) ? r : l;
            if (!before(data_[c], p)) break;
            place(i, data_[c]);
            i = c;
        }
        place(i, p);
    }

    void removeAt(int i) {
        Patient* gone = data_[i];
        Patient* last = data_[size_ - 1];
        --size_;
        if (i != size_) {
            place(i, last);
            if (i > 0 && before(last, data_[(i - 1) / 2])) siftUp(i);
            else siftDown(i);
        }
        if (track_) gone->heapIndex = -1;
    }

    void grow() {
        int newCap = cap_ * 2;
        Patient** nd = new Patient*[newCap];
        for (int i = 0; i < size_; ++i) nd[i] = data_[i];
        delete[] data_;
        data_ = nd;
        cap_ = newCap;
    }
};

#endif
