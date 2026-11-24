// Storage.h - MODULE 3 (Member 3): save / load to a text file with validation.
//
// File layout (fields separated by '|'; last line is an FNV-1a checksum):
//   ERTS 1
//   NEXT_ARRIVAL <n>
//   NEXT_TREATED <n>
//   WAITING <count>
//   id|name|age|severity|arrival              (count lines)
//   HISTORY <count>
//   id|name|age|severity|arrival|treatedOrder (count lines)
//   CHECKSUM <number>
//
// Loading is all-or-nothing: the file is fully parsed and validated first;
// the live system is only replaced if every check passes.
#ifndef STORAGE_H
#define STORAGE_H

#include <string>
#include <fstream>
#include <sstream>
#include <cstdio>
#include "TriageSystem.h"

namespace storage {

const int MAX_RECORDS = 100000;

inline unsigned long checksum(const std::string& s) {
    unsigned long h = 2166136261UL;
    for (size_t i = 0; i < s.size(); ++i) {
        h ^= (unsigned char)s[i];
        h = (h * 16777619UL) & 0xFFFFFFFFUL;
    }
    return h;
}

inline std::string recordLine(const Record& r, bool withTreated) {
    std::string s = validate::toStr(r.id) + "|" + r.name + "|" + validate::toStr(r.age) + "|" +
                    validate::toStr(r.severity) + "|" + validate::toStr(r.arrival);
    if (withTreated) s += "|" + validate::toStr(r.treatedOrder);
    return s + "\n";
}

inline bool save(const TriageSystem& sys, const std::string& path, std::string& err) {
    if (path.empty()) { err = "File name is empty."; return false; }

    std::string body = "ERTS 1\n";
    body += "NEXT_ARRIVAL " + validate::toStr(sys.nextArrival()) + "\n";
    body += "NEXT_TREATED " + validate::toStr(sys.nextTreatedOrder()) + "\n";

    int n = 0;
    Patient** snap = sys.waitingSnapshot(n);
    body += "WAITING " + validate::toStr(n) + "\n";
    for (int i = 0; i < n; ++i) {
        Record r;
        r.id = snap[i]->id; r.name = snap[i]->name; r.age = snap[i]->age;
        r.severity = snap[i]->severity; r.arrival = snap[i]->arrival;
        body += recordLine(r, false);
    }
    delete[] snap;

    body += "HISTORY " + validate::toStr(sys.treatedCount()) + "\n";
    for (const HistoryList::Node* h = sys.history().head(); h; h = h->next)
        body += recordLine(h->rec, true);

    std::string full = body + "CHECKSUM " + validate::toStr((long long)checksum(body)) + "\n";

    // Write to a temporary file first so a failed save cannot destroy old data.
    std::string tmp = path + ".tmp";
    {
        std::ofstream out(tmp.c_str());
        if (!out) { err = "Cannot open '" + tmp + "' for writing."; return false; }
        out << full;
        out.flush();
        if (!out) { err = "Write error while saving."; return false; }
    }
    std::remove(path.c_str());
    if (std::rename(tmp.c_str(), path.c_str()) != 0) {
        err = "Could not replace '" + path + "'.";
        std::remove(tmp.c_str());
        return false;
    }
    return true;
}

// ---- parsing helpers ----
inline bool nextLine(const std::string& text, size_t& pos, std::string& line) {
    if (pos >= text.size()) return false;
    size_t e = text.find('\n', pos);
    if (e == std::string::npos) { line = text.substr(pos); pos = text.size(); }
    else { line = text.substr(pos, e - pos); pos = e + 1; }
    return true;
}

inline int splitBar(const std::string& s, std::string* f, int maxFields) {
    int n = 0; size_t start = 0;
    for (;;) {
        size_t b = s.find('|', start);
        if (n >= maxFields) return maxFields + 1;
        if (b == std::string::npos) { f[n++] = s.substr(start); break; }
        f[n++] = s.substr(start, b - start);
        start = b + 1;
    }
    return n;
}

inline bool headerValue(const std::string& line, const std::string& key, long long hi, long long& out) {
    std::string prefix = key + " ";
    if (line.compare(0, prefix.size(), prefix) != 0) return false;
    return validate::parseInteger(line.substr(prefix.size()), 0, hi, out);
}

inline bool parseRecord(const std::string& line, bool withTreated, long long nextArrival, Record& r) {
    std::string f[7];
    int want = withTreated ? 6 : 5;
    if (splitBar(line, f, 6) != want) return false;
    long long id, age, sev, arr, tr = 0;
    if (!validate::parseInteger(f[0], validate::MIN_ID, validate::MAX_ID, id)) return false;
    if (!validate::validName(f[1])) return false;
    if (!validate::parseInteger(f[2], validate::MIN_AGE, validate::MAX_AGE, age)) return false;
    if (!validate::parseInteger(f[3], validate::MIN_SEVERITY, validate::MAX_SEVERITY, sev)) return false;
    if (!validate::parseInteger(f[4], 1, nextArrival - 1, arr)) return false;
    if (withTreated && !validate::parseInteger(f[5], 1, 999999999999LL, tr)) return false;
    r.id = (int)id; r.name = f[1]; r.age = (int)age; r.severity = (int)sev;
    r.arrival = (long)arr; r.treatedOrder = (long)tr;
    return true;
}

inline bool load(TriageSystem& sys, const std::string& path, std::string& err) {
    std::ifstream in(path.c_str());
    if (!in) { err = "Cannot open file '" + path + "'."; return false; }
    std::ostringstream ss; ss << in.rdbuf();
    std::string content = ss.str();

    std::string clean;                      // drop '\r' so CRLF files still verify
    for (size_t i = 0; i < content.size(); ++i) if (content[i] != '\r') clean += content[i];

    // ---- checksum ----
    size_t cpos = clean.rfind("CHECKSUM ");
    if (cpos == std::string::npos || (cpos != 0 && clean[cpos - 1] != '\n')) {
        err = "Corrupted file: checksum line missing."; return false;
    }
    std::string body = clean.substr(0, cpos);
    std::string cline = validate::trim(clean.substr(cpos + 9));
    long long stored;
    if (!validate::parseInteger(cline, 0, 4294967295LL, stored) ||
        (unsigned long)stored != checksum(body)) {
        err = "Corrupted file: checksum does not match (file was modified or damaged).";
        return false;
    }

    // ---- structure ----
    size_t pos = 0; std::string line;
    if (!nextLine(body, pos, line) || line != "ERTS 1") { err = "Not a valid triage data file."; return false; }

    long long nextArr, nextTr, wn, hn;
    if (!nextLine(body, pos, line) || !headerValue(line, "NEXT_ARRIVAL", 999999999999LL, nextArr) || nextArr < 1)
        { err = "Invalid NEXT_ARRIVAL line."; return false; }
    if (!nextLine(body, pos, line) || !headerValue(line, "NEXT_TREATED", 999999999999LL, nextTr) || nextTr < 1)
        { err = "Invalid NEXT_TREATED line."; return false; }
    if (!nextLine(body, pos, line) || !headerValue(line, "WAITING", MAX_RECORDS, wn))
        { err = "Invalid WAITING line."; return false; }

    Record* waiting = new Record[wn > 0 ? wn : 1];
    Record* hist    = nullptr;
    HashTable seen;                         // duplicate-ID detector (owns dummy patients)
    bool ok = true;

    for (long long i = 0; ok && i < wn; ++i) {
        if (!nextLine(body, pos, line) || !parseRecord(line, false, nextArr, waiting[i])) {
            err = "Invalid waiting-patient record at entry " + validate::toStr(i + 1) + "."; ok = false; break;
        }
        Patient* d = new Patient(); d->id = waiting[i].id;
        if (!seen.insert(d)) { delete d; err = "Duplicate patient ID " + validate::toStr(waiting[i].id) + " in file."; ok = false; }
    }

    if (ok && (!nextLine(body, pos, line) || !headerValue(line, "HISTORY", MAX_RECORDS, hn)))
        { err = "Invalid HISTORY line."; ok = false; }

    if (ok) {
        hist = new Record[hn > 0 ? hn : 1];
        long long prevOrder = 0;
        for (long long i = 0; i < hn; ++i) {
            if (!nextLine(body, pos, line) || !parseRecord(line, true, nextArr, hist[i])) {
                err = "Invalid history record at entry " + validate::toStr(i + 1) + "."; ok = false; break;
            }
            if (hist[i].treatedOrder <= prevOrder || hist[i].treatedOrder >= nextTr) {
                err = "History order numbers are inconsistent."; ok = false; break;
            }
            prevOrder = hist[i].treatedOrder;
        }
    }
    if (ok && nextLine(body, pos, line)) { err = "Unexpected extra data after history."; ok = false; }

    if (ok) {
        if (!sys.restore(waiting, (int)wn, hist, (int)hn, (long)nextArr, (long)nextTr)) {
            err = "Could not rebuild system from file."; ok = false;
        }
    }
    delete[] waiting;
    delete[] hist;
    return ok;
}

} // namespace storage

#endif
