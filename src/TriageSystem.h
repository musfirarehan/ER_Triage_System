// TriageSystem.h - MODULE 3 (Member 3): coordinates heap + hash table + history.
//
// Interface contract between modules (see README):
//   registerPatient -> table.insert + heap.insert
//   treatNext       -> heap.extractMin + history.append + table.remove
//   updateSeverity  -> table.find + heap.update
//   cancelPatient   -> table.find + heap.remove + table.remove
#ifndef TRIAGESYSTEM_H
#define TRIAGESYSTEM_H

#include <string>
#include "Patient.h"
#include "Validation.h"
#include "MinHeap.h"
#include "HashTable.h"
#include "HistoryList.h"

class TriageSystem {
public:
    TriageSystem() : nextArrival_(1), nextTreated_(1) {}

    int waitingCount() const { return table_.size(); }
    int treatedCount() const { return history_.size(); }
    bool empty()       const { return table_.size() == 0; }

    const HistoryList& history() const { return history_; }
    const MinHeap&     heap()    const { return heap_; }
    const HashTable&   table()   const { return table_; }
    long nextArrival()      const { return nextArrival_; }
    long nextTreatedOrder() const { return nextTreated_; }

    bool registerPatient(int id, const std::string& rawName, int age, int severity, std::string& err) {
        std::string name = validate::trim(rawName);
        if (!validate::validId(id))             { err = "ID must be between 1 and 999999999."; return false; }
        if (!validate::validName(name))         { err = "Invalid name (letters, spaces . ' - only, max 50)."; return false; }
        if (!validate::validAge(age))           { err = "Age must be between 0 and 120."; return false; }
        if (!validate::validSeverity(severity)) { err = "Severity must be between 1 and 5."; return false; }
        if (table_.find(id) != nullptr)         { err = "A waiting patient with this ID already exists."; return false; }

        Patient* p = new Patient();
        p->id = id; p->name = name; p->age = age; p->severity = severity;
        p->arrival = nextArrival_++;
        table_.insert(p);   // cannot fail: duplicate checked above
        heap_.insert(p);
        return true;
    }

    const Patient* peekNext() const { return heap_.peek(); }

    // Treats the most urgent patient. Returns false if nobody is waiting.
    bool treatNext(Record& out) {
        Patient* p = heap_.extractMin();
        if (p == nullptr) return false;
        out.id = p->id; out.name = p->name; out.age = p->age;
        out.severity = p->severity; out.arrival = p->arrival;
        out.treatedOrder = nextTreated_++;
        history_.append(out);
        Patient* owned = table_.remove(p->id);
        delete owned;       // owned == p
        return true;
    }

    const Patient* find(int id) const { return table_.find(id); }

    bool updateSeverity(int id, int newSeverity, std::string& err) {
        if (!validate::validSeverity(newSeverity)) { err = "Severity must be between 1 and 5."; return false; }
        Patient* p = table_.find(id);
        if (p == nullptr) { err = "No waiting patient with that ID."; return false; }
        p->severity = newSeverity;      // arrival number is kept -> keeps place among equals
        heap_.update(p);
        return true;
    }

    bool cancelPatient(int id, Record& out, std::string& err) {
        Patient* p = table_.find(id);
        if (p == nullptr) { err = "No waiting patient with that ID."; return false; }
        out.id = p->id; out.name = p->name; out.age = p->age;
        out.severity = p->severity; out.arrival = p->arrival; out.treatedOrder = 0;
        heap_.remove(p);
        delete table_.remove(id);
        return true;
    }

    // Waiting patients in treatment order, without changing the system.
    // Caller must delete[] the returned array (the patients stay owned by the table).
    Patient** waitingSnapshot(int& n) const {
        n = heap_.size();
        Patient** out = new Patient*[n > 0 ? n : 1];
        MinHeap tmp(false, n + 1);
        for (int i = 0; i < n; ++i) tmp.insert(heap_.at(i));
        for (int i = 0; i < n; ++i) out[i] = tmp.extractMin();
        return out;
    }

    void clearAll() {
        heap_.clear();
        table_.clear();
        history_.clear();
        nextArrival_ = 1;
        nextTreated_ = 1;
    }

    // Rebuilds the whole system from already-validated records (used by file load).
    bool restore(const Record* waiting, int wn, const Record* hist, int hn,
                 long nextArrival, long nextTreated) {
        clearAll();
        for (int i = 0; i < wn; ++i) {
            Patient* p = new Patient();
            p->id = waiting[i].id; p->name = waiting[i].name; p->age = waiting[i].age;
            p->severity = waiting[i].severity; p->arrival = waiting[i].arrival;
            if (!table_.insert(p)) { delete p; clearAll(); return false; }
            heap_.insert(p);
        }
        for (int i = 0; i < hn; ++i) history_.append(hist[i]);
        nextArrival_ = nextArrival;
        nextTreated_ = nextTreated;
        return true;
    }

private:
    MinHeap     heap_;
    HashTable   table_;
    HistoryList history_;
    long        nextArrival_;
    long        nextTreated_;
};

#endif
