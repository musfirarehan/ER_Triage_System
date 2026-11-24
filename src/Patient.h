// Patient.h - shared record types used by all three modules.
#ifndef PATIENT_H
#define PATIENT_H

#include <string>

// A waiting patient. Stored ONCE (owned by the HashTable); the heap only
// keeps pointers to it. heapIndex lets the heap find the patient in O(1).
struct Patient {
    int         id;
    std::string name;
    int         age;
    int         severity;   // 1 (critical) .. 5 (minor)
    long        arrival;    // arrival sequence number (tie-breaker)
    int         heapIndex;  // current position inside the heap, -1 if none

    Patient() : id(0), age(0), severity(0), arrival(0), heapIndex(-1) {}
};

// A plain data record: used for the treatment history and for file I/O.
struct Record {
    int         id;
    std::string name;
    int         age;
    int         severity;
    long        arrival;       // original arrival number
    long        treatedOrder;  // 1 = first patient ever treated, 2 = second ...

    Record() : id(0), age(0), severity(0), arrival(0), treatedOrder(0) {}
};

inline const char* severityLabel(int s) {
    switch (s) {
        case 1: return "Critical";
        case 2: return "Emergent";
        case 3: return "Urgent";
        case 4: return "Less urgent";
        case 5: return "Minor";
        default: return "Unknown";
    }
}

#endif
