// test_main.cpp - separate test program: normal cases, edge cases, stress test.
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include "TriageSystem.h"
#include "Storage.h"

static int passed = 0, failed = 0;
#define CHECK(cond) do { if (cond) ++passed; else { ++failed; \
    std::cout << "  FAIL line " << __LINE__ << ": " #cond "\n"; } } while (0)

static Patient* mk(int id, int sev, long arr) {
    Patient* p = new Patient(); p->id = id; p->name = "P"; p->age = 30; p->severity = sev; p->arrival = arr; return p;
}

static void testHeap() {
    std::cout << "[MinHeap]\n";
    MinHeap h;
    CHECK(h.empty()); CHECK(h.peek() == nullptr); CHECK(h.extractMin() == nullptr);

    Patient* single = mk(1, 3, 1);
    h.insert(single);
    CHECK(h.size() == 1); CHECK(h.peek() == single);
    CHECK(h.extractMin() == single); CHECK(h.empty()); CHECK(single->heapIndex == -1);
    delete single;

    // priority + tie-break by arrival
    Patient* a = mk(1, 3, 1); Patient* b = mk(2, 1, 2); Patient* c = mk(3, 3, 3);
    Patient* d = mk(4, 1, 4); Patient* e = mk(5, 5, 5);
    h.insert(a); h.insert(b); h.insert(c); h.insert(d); h.insert(e);
    CHECK(h.isValid());
    CHECK(h.extractMin() == b); CHECK(h.extractMin() == d);
    CHECK(h.extractMin() == a); CHECK(h.extractMin() == c); CHECK(h.extractMin() == e);
    delete a; delete b; delete c; delete d; delete e;

    // remove arbitrary + update
    Patient* p[6];
    for (int i = 0; i < 6; ++i) { p[i] = mk(i + 1, 3, i + 1); h.insert(p[i]); }
    CHECK(h.remove(p[2])); CHECK(!h.remove(p[2])); CHECK(h.isValid());
    p[4]->severity = 1; CHECK(h.update(p[4])); CHECK(h.peek() == p[4]);
    p[4]->severity = 5; h.update(p[4]); CHECK(h.peek() == p[0]); CHECK(h.isValid());
    while (!h.empty()) h.extractMin();
    for (int i = 0; i < 6; ++i) delete p[i];

    // growth beyond initial capacity
    MinHeap g(true, 4);
    Patient* many[200];
    for (int i = 0; i < 200; ++i) { many[i] = mk(i + 1, (i * 7) % 5 + 1, i + 1); g.insert(many[i]); }
    CHECK(g.size() == 200); CHECK(g.capacity() >= 200); CHECK(g.isValid());
    int prevSev = 0; long prevArr = 0; bool ordered = true;
    while (!g.empty()) {
        Patient* x = g.extractMin();
        if (x->severity < prevSev || (x->severity == prevSev && x->arrival < prevArr)) ordered = false;
        prevSev = x->severity; prevArr = x->arrival;
    }
    CHECK(ordered);
    for (int i = 0; i < 200; ++i) delete many[i];
}

static void testHash() {
    std::cout << "[HashTable]\n";
    HashTable t(5);
    CHECK(t.find(1) == nullptr); CHECK(t.remove(1) == nullptr); CHECK(t.size() == 0);
    CHECK(t.insert(mk(10, 1, 1)));
    Patient* dup = mk(10, 2, 2);
    CHECK(!t.insert(dup)); delete dup;
    CHECK(t.find(10) != nullptr); CHECK(t.find(11) == nullptr);
    int startBuckets = t.bucketCount();
    for (int i = 100; i < 600; ++i) CHECK(t.insert(mk(i, 3, i)));
    CHECK(t.bucketCount() > startBuckets);
    CHECK(t.loadFactor() <= 0.75);
    bool allFound = true;
    for (int i = 100; i < 600; ++i) if (!t.find(i) || t.find(i)->id != i) allFound = false;
    CHECK(allFound);
    for (int i = 100; i < 600; i += 2) { Patient* r = t.remove(i); CHECK(r != nullptr); delete r; }
    CHECK(t.find(100) == nullptr); CHECK(t.find(101) != nullptr);
    CHECK(t.size() == 1 + 250);
    CHECK(t.remove(100) == nullptr);
}

static void testHistory() {
    std::cout << "[HistoryList]\n";
    HistoryList h;
    CHECK(h.empty()); CHECK(h.head() == nullptr); CHECK(h.tail() == nullptr);
    Record r; r.id = 1; r.treatedOrder = 1; h.append(r);
    CHECK(h.head() == h.tail());
    r.id = 2; r.treatedOrder = 2; h.append(r);
    r.id = 3; r.treatedOrder = 3; h.append(r);
    CHECK(h.size() == 3);
    CHECK(h.head()->rec.id == 1 && h.head()->next->rec.id == 2 && h.head()->next->next->rec.id == 3);
    CHECK(h.tail()->rec.id == 3 && h.tail()->prev->rec.id == 2 && h.tail()->prev->prev->rec.id == 1);
    h.clear(); CHECK(h.empty());
}

static void testSystem() {
    std::cout << "[TriageSystem]\n";
    TriageSystem s; std::string err; Record rec;
    CHECK(s.empty()); CHECK(!s.treatNext(rec)); CHECK(s.peekNext() == nullptr);
    CHECK(!s.cancelPatient(5, rec, err)); CHECK(!s.updateSeverity(5, 2, err));

    CHECK(s.registerPatient(1, "Ali Khan", 40, 3, err));
    CHECK(!s.registerPatient(1, "Dup Person", 20, 1, err));          // duplicate ID
    CHECK(!s.registerPatient(0, "Zero", 20, 1, err));                // bad ID
    CHECK(!s.registerPatient(2, "", 20, 1, err));                    // empty name
    CHECK(!s.registerPatient(2, "B0b", 20, 1, err));                 // bad chars
    CHECK(!s.registerPatient(2, "Bob|x", 20, 1, err));               // delimiter
    CHECK(!s.registerPatient(2, "Bob", 121, 1, err));                // bad age
    CHECK(!s.registerPatient(2, "Bob", -1, 1, err));
    CHECK(!s.registerPatient(2, "Bob", 20, 0, err));                 // bad severity
    CHECK(!s.registerPatient(2, "Bob", 20, 6, err));
    CHECK(s.waitingCount() == 1);

    CHECK(s.registerPatient(2, "Sara", 25, 3, err));                 // same severity, later arrival
    CHECK(s.registerPatient(3, "Omar", 70, 1, err));
    CHECK(s.registerPatient(4, "Zain", 10, 5, err));
    CHECK(s.peekNext()->id == 3);

    // update keeps original arrival position among equals
    CHECK(s.updateSeverity(2, 5, err));                              // Sara -> 5 (arrived before Zain)
    CHECK(s.updateSeverity(2, 3, err));                              // back to 3, still after Ali
    int n; Patient** snap = s.waitingSnapshot(n);
    CHECK(n == 4 && snap[0]->id == 3 && snap[1]->id == 1 && snap[2]->id == 2 && snap[3]->id == 4);
    delete[] snap;
    CHECK(s.waitingCount() == 4);                                    // snapshot does not alter system

    CHECK(s.cancelPatient(1, rec, err)); CHECK(s.find(1) == nullptr);
    CHECK(s.treatNext(rec) && rec.id == 3 && rec.treatedOrder == 1);
    CHECK(s.treatNext(rec) && rec.id == 2);
    CHECK(s.treatNext(rec) && rec.id == 4);
    CHECK(!s.treatNext(rec)); CHECK(s.treatedCount() == 3);
    CHECK(s.registerPatient(3, "Omar Again", 70, 2, err));           // ID free again after treatment
}

static void testStorage() {
    std::cout << "[Storage]\n";
    const char* f = "test_tmp_data.txt";
    std::string err; Record rec;

    TriageSystem a;
    // empty system round-trip
    CHECK(storage::save(a, f, err));
    TriageSystem b; b.registerPatient(9, "Temp", 1, 1, err);
    CHECK(storage::load(b, f, err)); CHECK(b.empty() && b.treatedCount() == 0);

    a.registerPatient(1, "Ali Khan", 40, 3, err);
    a.registerPatient(2, "Sara", 25, 3, err);
    a.registerPatient(3, "Omar", 70, 1, err);
    a.registerPatient(4, "Zain", 10, 5, err);
    a.treatNext(rec);
    CHECK(storage::save(a, f, err));

    TriageSystem c;
    CHECK(storage::load(c, f, err));
    CHECK(c.waitingCount() == 3 && c.treatedCount() == 1);
    CHECK(c.history().head()->rec.id == 3);
    int n; Patient** s = c.waitingSnapshot(n);
    CHECK(s[0]->id == 1 && s[1]->id == 2 && s[2]->id == 4);
    delete[] s;
    CHECK(c.registerPatient(5, "New", 5, 3, err));                   // counters restored
    CHECK(c.find(5)->arrival == 5);

    // missing file
    TriageSystem d; d.registerPatient(7, "Keep", 9, 2, err);
    CHECK(!storage::load(d, "no_such_file.txt", err));
    CHECK(d.waitingCount() == 1);                                    // unchanged

    // corruption tests
    std::string text;
    { std::ifstream in(f); std::ostringstream ss; ss << in.rdbuf(); text = ss.str(); }
    struct Case { std::string from, to; } cases[] = {
        {"Ali Khan", "Ali Khen"},      // checksum mismatch
        {"WAITING 3", "WAITING 9"},
        {"ERTS 1", "XXXX 1"},
    };
    for (int i = 0; i < 3; ++i) {
        std::string bad = text; size_t p = bad.find(cases[i].from);
        bad.replace(p, cases[i].from.size(), cases[i].to);
        { std::ofstream o(f); o << bad; }
        TriageSystem e; e.registerPatient(7, "Keep", 9, 2, err);
        CHECK(!storage::load(e, f, err)); CHECK(e.waitingCount() == 1);
    }
    { std::ofstream o(f); o << "garbage\n"; }
    CHECK(!storage::load(d, f, err));
    { std::ofstream o(f); }                                          // empty file
    CHECK(!storage::load(d, f, err));

    // valid checksum but invalid content (duplicate IDs, bad severity)
    std::string body = "ERTS 1\nNEXT_ARRIVAL 3\nNEXT_TREATED 1\nWAITING 2\n1|A|20|2|1\n1|B|20|2|2\nHISTORY 0\n";
    { std::ofstream o(f); o << body << "CHECKSUM " << storage::checksum(body) << "\n"; }
    CHECK(!storage::load(d, f, err));
    body = "ERTS 1\nNEXT_ARRIVAL 3\nNEXT_TREATED 1\nWAITING 1\n1|A|20|9|1\nHISTORY 0\n";
    { std::ofstream o(f); o << body << "CHECKSUM " << storage::checksum(body) << "\n"; }
    CHECK(!storage::load(d, f, err));
    std::remove(f);
}

// Randomised comparison against a naive O(n) reference model.
static void testStress() {
    std::cout << "[Stress test vs naive model]\n";
    srand(12345);
    TriageSystem s; std::string err; Record rec;
    const int MAXID = 300;
    bool  in[MAXID + 1]; int sev[MAXID + 1]; long arr[MAXID + 1];
    for (int i = 0; i <= MAXID; ++i) in[i] = false;
    long nextArr = 1; bool allOk = true;

    for (int step = 0; step < 20000; ++step) {
        int op = rand() % 5, id = rand() % MAXID + 1;
        if (op == 0) {
            int sv = rand() % 5 + 1;
            bool ok = s.registerPatient(id, "Name", 30, sv, err);
            if (ok == in[id]) allOk = false;
            if (ok) { in[id] = true; sev[id] = sv; arr[id] = nextArr++; }
        } else if (op == 1) {
            int best = -1;
            for (int i = 1; i <= MAXID; ++i)
                if (in[i] && (best < 0 || sev[i] < sev[best] || (sev[i] == sev[best] && arr[i] < arr[best]))) best = i;
            bool ok = s.treatNext(rec);
            if (ok != (best >= 0) || (ok && rec.id != best)) allOk = false;
            if (ok) in[best] = false;
        } else if (op == 2) {
            int sv = rand() % 5 + 1;
            bool ok = s.updateSeverity(id, sv, err);
            if (ok != in[id]) allOk = false;
            if (ok) sev[id] = sv;
        } else if (op == 3) {
            bool ok = s.cancelPatient(id, rec, err);
            if (ok != in[id]) allOk = false;
            if (ok) in[id] = false;
        } else {
            if ((s.find(id) != nullptr) != in[id]) allOk = false;
        }
        if (!s.heap().isValid()) allOk = false;
        if (!allOk) { std::cout << "  mismatch at step " << step << "\n"; break; }
    }
    CHECK(allOk);
}

int main() {
    testHeap(); testHash(); testHistory(); testSystem(); testStorage(); testStress();
    std::cout << "\nPassed: " << passed << "   Failed: " << failed << "\n";
    return failed == 0 ? 0 : 1;
}
