// main.cpp - menu-driven console interface (MODULE 3, Member 3)
// Emergency Room Triage System - Data Structures project, BCS-3F
#include <iostream>
#include <iomanip>
#include <string>
#include "TriageSystem.h"
#include "Storage.h"

using std::cin;
using std::cout;
using std::string;

static const char* DEFAULT_FILE = "triage_data.txt";

// ---------- input helpers (all input is read as whole lines and validated) ----------
enum InputResult { INPUT_OK = 1, INPUT_CANCEL = 0, INPUT_EOF = -1 };

static bool readLine(const string& prompt, string& out) {
    cout << prompt;
    if (!std::getline(cin, out)) return false;
    out = validate::trim(out);
    return true;
}

// Re-asks until valid. Typing 'q' cancels the operation.
static InputResult askInt(const string& prompt, long long lo, long long hi, int& out) {
    string s;
    for (;;) {
        if (!readLine(prompt, s)) return INPUT_EOF;
        if (s == "q" || s == "Q") return INPUT_CANCEL;
        long long v;
        if (validate::parseInteger(s, lo, hi, v)) { out = (int)v; return INPUT_OK; }
        cout << "  Invalid input. Enter a whole number from " << lo << " to " << hi
             << " (or 'q' to cancel).\n";
    }
}

static InputResult askName(const string& prompt, string& out) {
    string s;
    for (;;) {
        if (!readLine(prompt, s)) return INPUT_EOF;
        if (s == "q" || s == "Q") return INPUT_CANCEL;
        if (validate::validName(s)) { out = s; return INPUT_OK; }
        cout << "  Invalid name. Use letters, spaces, . ' - only (max "
             << validate::MAX_NAME_LEN << " chars), or 'q' to cancel.\n";
    }
}

static InputResult askYesNo(const string& prompt, bool& yes) {
    string s;
    for (;;) {
        if (!readLine(prompt, s)) return INPUT_EOF;
        if (s == "y" || s == "Y" || s == "yes") { yes = true; return INPUT_OK; }
        if (s == "n" || s == "N" || s == "no")  { yes = false; return INPUT_OK; }
        cout << "  Please answer y or n.\n";
    }
}

// ---------- display helpers ----------
static void printHeader() {
    cout << "  " << std::left << std::setw(11) << "ID" << std::setw(24) << "Name"
         << std::setw(5) << "Age" << std::setw(14) << "Severity" << "Arrival#\n";
    cout << "  " << string(62, '-') << "\n";
}

static void printRow(int id, const string& name, int age, int sev, long arrival) {
    cout << "  " << std::left << std::setw(11) << id << std::setw(24) << name
         << std::setw(5) << age
         << std::setw(14) << (validate::toStr(sev) + " " + severityLabel(sev))
         << arrival << "\n";
}

static void printPatient(const Patient& p) { printRow(p.id, p.name, p.age, p.severity, p.arrival); }

static void showMenu() {
    cout << "\n==============================================\n"
         << "      EMERGENCY ROOM TRIAGE SYSTEM\n"
         << "==============================================\n"
         << "  1. Register a patient\n"
         << "  2. Treat next patient\n"
         << "  3. View next patient (without treating)\n"
         << "  4. Search waiting patient by ID\n"
         << "  5. Update a patient's severity\n"
         << "  6. Cancel a waiting patient\n"
         << "  7. Display waiting list (treatment order)\n"
         << "  8. Display treatment history\n"
         << "  9. Save data to file\n"
         << " 10. Load data from file\n"
         << " 11. System statistics\n"
         << "  0. Exit\n"
         << "----------------------------------------------\n";
}

// ---------- menu actions (each returns false only on end-of-input) ----------
static bool doRegister(TriageSystem& sys, bool& dirty) {
    cout << "\n-- Register patient (enter 'q' at any prompt to cancel) --\n";
    int id, age, sev; string name, err;
    InputResult r;

    for (;;) {
        r = askInt("Patient ID (1-999999999): ", validate::MIN_ID, validate::MAX_ID, id);
        if (r == INPUT_EOF) return false;
        if (r == INPUT_CANCEL) { cout << "Cancelled.\n"; return true; }
        if (sys.find(id) != nullptr) { cout << "  A waiting patient with ID " << id << " already exists.\n"; continue; }
        break;
    }
    r = askName("Name: ", name);
    if (r == INPUT_EOF) return false;
    if (r == INPUT_CANCEL) { cout << "Cancelled.\n"; return true; }
    r = askInt("Age (0-120): ", validate::MIN_AGE, validate::MAX_AGE, age);
    if (r == INPUT_EOF) return false;
    if (r == INPUT_CANCEL) { cout << "Cancelled.\n"; return true; }
    cout << "  Severity: 1=Critical 2=Emergent 3=Urgent 4=Less urgent 5=Minor\n";
    r = askInt("Severity (1-5): ", validate::MIN_SEVERITY, validate::MAX_SEVERITY, sev);
    if (r == INPUT_EOF) return false;
    if (r == INPUT_CANCEL) { cout << "Cancelled.\n"; return true; }

    if (sys.registerPatient(id, name, age, sev, err)) {
        dirty = true;
        cout << "Patient registered. Waiting: " << sys.waitingCount() << "\n";
    } else cout << "Error: " << err << "\n";
    return true;
}

static void doTreat(TriageSystem& sys, bool& dirty) {
    Record rec;
    if (!sys.treatNext(rec)) { cout << "\nNo patients are waiting.\n"; return; }
    dirty = true;
    cout << "\nNow treating:\n";
    printHeader();
    printRow(rec.id, rec.name, rec.age, rec.severity, rec.arrival);
    cout << "Patients still waiting: " << sys.waitingCount() << "\n";
}

static void doPeek(const TriageSystem& sys) {
    const Patient* p = sys.peekNext();
    if (!p) { cout << "\nNo patients are waiting.\n"; return; }
    cout << "\nNext patient to be treated:\n";
    printHeader(); printPatient(*p);
}

static bool doSearch(const TriageSystem& sys) {
    if (sys.empty()) { cout << "\nNo patients are waiting.\n"; return true; }
    int id; InputResult r = askInt("\nPatient ID to search: ", validate::MIN_ID, validate::MAX_ID, id);
    if (r == INPUT_EOF) return false;
    if (r == INPUT_CANCEL) { cout << "Cancelled.\n"; return true; }
    const Patient* p = sys.find(id);
    if (!p) { cout << "No waiting patient with ID " << id << ".\n"; return true; }
    printHeader(); printPatient(*p);
    return true;
}

static bool doUpdate(TriageSystem& sys, bool& dirty) {
    if (sys.empty()) { cout << "\nNo patients are waiting.\n"; return true; }
    int id, sev; string err;
    InputResult r = askInt("\nPatient ID to update: ", validate::MIN_ID, validate::MAX_ID, id);
    if (r == INPUT_EOF) return false;
    if (r == INPUT_CANCEL) { cout << "Cancelled.\n"; return true; }
    const Patient* p = sys.find(id);
    if (!p) { cout << "No waiting patient with ID " << id << ".\n"; return true; }
    cout << "Current: " << p->name << ", severity " << p->severity << " (" << severityLabel(p->severity) << ")\n";
    r = askInt("New severity (1-5): ", validate::MIN_SEVERITY, validate::MAX_SEVERITY, sev);
    if (r == INPUT_EOF) return false;
    if (r == INPUT_CANCEL) { cout << "Cancelled.\n"; return true; }
    if (sys.updateSeverity(id, sev, err)) { dirty = true; cout << "Severity updated. Original arrival position kept.\n"; }
    else cout << "Error: " << err << "\n";
    return true;
}

static bool doCancel(TriageSystem& sys, bool& dirty) {
    if (sys.empty()) { cout << "\nNo patients are waiting.\n"; return true; }
    int id; string err; Record rec;
    InputResult r = askInt("\nPatient ID to cancel: ", validate::MIN_ID, validate::MAX_ID, id);
    if (r == INPUT_EOF) return false;
    if (r == INPUT_CANCEL) { cout << "Cancelled.\n"; return true; }
    bool yes;
    const Patient* p = sys.find(id);
    if (!p) { cout << "No waiting patient with ID " << id << ".\n"; return true; }
    cout << "Cancel " << p->name << " (ID " << p->id << ")?\n";
    if (askYesNo("Confirm (y/n): ", yes) == INPUT_EOF) return false;
    if (!yes) { cout << "Not cancelled.\n"; return true; }
    if (sys.cancelPatient(id, rec, err)) { dirty = true; cout << "Patient " << rec.name << " removed from the waiting list.\n"; }
    else cout << "Error: " << err << "\n";
    return true;
}

static void doShowWaiting(const TriageSystem& sys) {
    if (sys.empty()) { cout << "\nNo patients are waiting.\n"; return; }
    int n; Patient** snap = sys.waitingSnapshot(n);
    cout << "\nWaiting list in treatment order (" << n << "):\n";
    printHeader();
    for (int i = 0; i < n; ++i) printPatient(*snap[i]);
    delete[] snap;
}

static bool doShowHistory(const TriageSystem& sys) {
    if (sys.history().empty()) { cout << "\nNo patients have been treated yet.\n"; return true; }
    string s;
    for (;;) {
        if (!readLine("\nShow (1) oldest first or (2) newest first: ", s)) return false;
        if (s == "1" || s == "2") break;
        cout << "  Enter 1 or 2.\n";
    }
    cout << "\nTreatment history (" << sys.treatedCount() << "):\n";
    cout << "  " << std::left << std::setw(7) << "Order" << std::setw(11) << "ID" << std::setw(24) << "Name"
         << std::setw(5) << "Age" << "Severity\n  " << string(62, '-') << "\n";
    const HistoryList& h = sys.history();
    if (s == "1") {
        for (const HistoryList::Node* n = h.head(); n; n = n->next)
            cout << "  " << std::left << std::setw(7) << n->rec.treatedOrder << std::setw(11) << n->rec.id
                 << std::setw(24) << n->rec.name << std::setw(5) << n->rec.age
                 << n->rec.severity << " " << severityLabel(n->rec.severity) << "\n";
    } else {
        for (const HistoryList::Node* n = h.tail(); n; n = n->prev)
            cout << "  " << std::left << std::setw(7) << n->rec.treatedOrder << std::setw(11) << n->rec.id
                 << std::setw(24) << n->rec.name << std::setw(5) << n->rec.age
                 << n->rec.severity << " " << severityLabel(n->rec.severity) << "\n";
    }
    return true;
}

static bool askFileName(const char* verb, string& file) {
    string s;
    if (!readLine(string("\nFile to ") + verb + " [" + DEFAULT_FILE + "]: ", s)) return false;
    file = s.empty() ? string(DEFAULT_FILE) : s;
    return true;
}

static bool doSave(const TriageSystem& sys, bool& dirty) {
    string file, err;
    if (!askFileName("save to", file)) return false;
    if (storage::save(sys, file, err)) { dirty = false; cout << "Saved to '" << file << "'.\n"; }
    else cout << "Save failed: " << err << "\n";
    return true;
}

static bool doLoad(TriageSystem& sys, bool& dirty) {
    string file, err; bool yes = true;
    if (dirty) {
        cout << "\nWarning: loading replaces all current data, which has unsaved changes.\n";
        if (askYesNo("Continue (y/n): ", yes) == INPUT_EOF) return false;
        if (!yes) { cout << "Load cancelled.\n"; return true; }
    }
    if (!askFileName("load from", file)) return false;
    if (storage::load(sys, file, err)) {
        dirty = false;
        cout << "Loaded: " << sys.waitingCount() << " waiting, " << sys.treatedCount() << " treated.\n";
    } else cout << "Load failed: " << err << "\n(Current data was left unchanged.)\n";
    return true;
}

static void doStats(const TriageSystem& sys) {
    cout << "\nSystem statistics\n"
         << "  Waiting patients ........ " << sys.waitingCount() << "\n"
         << "  Treated patients ........ " << sys.treatedCount() << "\n"
         << "  Heap size / capacity .... " << sys.heap().size() << " / " << sys.heap().capacity() << "\n"
         << "  Hash table buckets ...... " << sys.table().bucketCount() << "\n"
         << "  Hash table load factor .. " << std::fixed << std::setprecision(2) << sys.table().loadFactor() << "\n"
         << "  Longest hash chain ...... " << sys.table().longestChain() << "\n";
}

int main() {
    TriageSystem sys;
    bool dirty = false;
    string choice;

    cout << "Welcome. Enter 'q' at any data prompt to cancel an operation.\n";
    for (;;) {
        showMenu();
        if (!readLine("Choose an option: ", choice)) break;   // end of input
        bool ok = true;
        if      (choice == "1")  ok = doRegister(sys, dirty);
        else if (choice == "2")  doTreat(sys, dirty);
        else if (choice == "3")  doPeek(sys);
        else if (choice == "4")  ok = doSearch(sys);
        else if (choice == "5")  ok = doUpdate(sys, dirty);
        else if (choice == "6")  ok = doCancel(sys, dirty);
        else if (choice == "7")  doShowWaiting(sys);
        else if (choice == "8")  ok = doShowHistory(sys);
        else if (choice == "9")  ok = doSave(sys, dirty);
        else if (choice == "10") ok = doLoad(sys, dirty);
        else if (choice == "11") doStats(sys);
        else if (choice == "0") {
            if (dirty) {
                bool yes;
                if (askYesNo("You have unsaved changes. Save before exit (y/n): ", yes) == INPUT_OK && yes) {
                    string err;
                    if (storage::save(sys, DEFAULT_FILE, err)) cout << "Saved to '" << DEFAULT_FILE << "'.\n";
                    else cout << "Save failed: " << err << "\n";
                }
            }
            cout << "Goodbye.\n";
            return 0;
        }
        else cout << "Invalid choice. Enter a number from 0 to 11.\n";
        if (!ok) break;
    }
    cout << "\nInput ended. Goodbye.\n";
    return 0;
}
