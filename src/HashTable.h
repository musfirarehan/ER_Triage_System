// HashTable.h - MODULE 2 (Member 2): hash table with separate chaining.
//
// Keyed by patient ID. The table OWNS the Patient objects stored in it.
// Resizes (rehashes) automatically when load factor would exceed 0.75.
//
//   insert ... O(1) average, rejects duplicate IDs
//   find ..... O(1) average
//   remove ... O(1) average (detaches the patient; caller deletes it)
#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "Patient.h"

class HashTable {
public:
    explicit HashTable(int initialBuckets = 11) : count_(0) {
        cap_ = initialBuckets < 5 ? 5 : initialBuckets;
        buckets_ = new Node*[cap_];
        for (int i = 0; i < cap_; ++i) buckets_[i] = nullptr;
    }
    ~HashTable() {
        clear();
        delete[] buckets_;
    }

    int size()        const { return count_; }
    int bucketCount() const { return cap_; }
    double loadFactor() const { return (double)count_ / cap_; }

    // Returns false (and stores nothing) if a patient with this ID already exists.
    bool insert(Patient* p) {
        if (p == nullptr || find(p->id) != nullptr) return false;
        if ((count_ + 1) * 4 > cap_ * 3) rehash(cap_ * 2 + 1);
        int b = indexFor(p->id, cap_);
        buckets_[b] = new Node(p, buckets_[b]);
        ++count_;
        return true;
    }

    Patient* find(int id) const {
        for (Node* n = buckets_[indexFor(id, cap_)]; n != nullptr; n = n->next)
            if (n->patient->id == id) return n->patient;
        return nullptr;
    }

    // Detaches and returns the patient (nullptr if not found). Caller must delete it.
    Patient* remove(int id) {
        int b = indexFor(id, cap_);
        Node* prev = nullptr;
        for (Node* n = buckets_[b]; n != nullptr; prev = n, n = n->next) {
            if (n->patient->id == id) {
                if (prev) prev->next = n->next; else buckets_[b] = n->next;
                Patient* p = n->patient;
                delete n;
                --count_;
                return p;
            }
        }
        return nullptr;
    }

    // Deletes every stored patient and empties the table.
    void clear() {
        for (int i = 0; i < cap_; ++i) {
            Node* n = buckets_[i];
            while (n) {
                Node* next = n->next;
                delete n->patient;
                delete n;
                n = next;
            }
            buckets_[i] = nullptr;
        }
        count_ = 0;
    }

    int longestChain() const {
        int best = 0;
        for (int i = 0; i < cap_; ++i) {
            int len = 0;
            for (Node* n = buckets_[i]; n; n = n->next) ++len;
            if (len > best) best = len;
        }
        return best;
    }

private:
    struct Node {
        Patient* patient;
        Node*    next;
        Node(Patient* p, Node* nx) : patient(p), next(nx) {}
    };

    Node** buckets_;
    int    cap_;
    int    count_;

    HashTable(const HashTable&) = delete;
    HashTable& operator=(const HashTable&) = delete;

    static int indexFor(int id, int cap) {
        unsigned long long h = (unsigned long long)(unsigned int)id * 2654435761ULL;
        h ^= (h >> 15);
        return (int)(h % (unsigned long long)cap);
    }

    void rehash(int newCap) {
        Node** nb = new Node*[newCap];
        for (int i = 0; i < newCap; ++i) nb[i] = nullptr;
        for (int i = 0; i < cap_; ++i) {
            Node* n = buckets_[i];
            while (n) {
                Node* next = n->next;
                int b = indexFor(n->patient->id, newCap);
                n->next = nb[b];
                nb[b] = n;
                n = next;
            }
        }
        delete[] buckets_;
        buckets_ = nb;
        cap_ = newCap;
    }
};

#endif
