# Emergency Room Triage System
**Data Structures – BCS-3F** | 25K-0560 Insiya Murad · 25K-0720 Hooria Ali · 25K-0873 Musfira Rehan

Console application in C++11. Patients are treated by urgency (severity 1 = critical … 5 = minor);
equal severity → first come, first served. All core data structures are written by hand
(no `std::vector`, `map`, `list`, `queue`, `priority_queue`, …).

## Build & run
Linux / macOS / MinGW with `make`:
```
make            # builds ./triage and ./run_tests
./triage        # the application
make test       # runs the test program
```
Windows (MinGW g++): double-click / run `build.bat`, then `triage.exe` and `run_tests.exe`.
Quick demo: `./triage < tests/demo_input.txt`  (a ready-made data file is in `sample_data.txt`; use menu 10 to load it).

## Design
| Module | File | Structure | Owner |
|---|---|---|---|
| 1 Priority queue | `include/MinHeap.h` | Binary min-heap of `Patient*`; each patient stores its `heapIndex`; auto-grows | Hooria Ali |
| 2 Patient lookup | `include/HashTable.h` | Hash table, separate chaining, rehash at load > 0.75, rejects duplicate IDs; owns the `Patient` objects | Musfira Rehan |
| 3 History / UI | `HistoryList.h`, `TriageSystem.h` | Doubly linked list, file I/O with checksum, controller| Insiya Murad|

Shared: `Patient.h` (record types), `Validation.h` (input checks), Storage.h, main.cpp

Each waiting patient is stored **once** (in the hash table); the heap holds pointers to the same object.

### Module interface (agreed function contract)
| Operation | Calls |
|---|---|
| Register | `table.insert(p)` → `heap.insert(p)` |
| Treat next | `heap.extractMin()` → `history.append(rec)` → `table.remove(id)` + delete |
| Update severity | `table.find(id)` → change severity → `heap.update(p)` (arrival number kept) |
| Cancel | `table.find(id)` → `heap.remove(p)` → `table.remove(id)` + delete |
| Display waiting | copy of heap pointers → heap-extract the copy (system untouched) |

### Complexity
| Operation | Cost |
|---|---|
| Register / treat / update / cancel | O(log n) |
| View next | O(1) |
| Search by ID | O(1) average |
| Append to history | O(1) |
| Display waiting list | O(n log n) |
| Save / load | O(n log n) / O(n) |

## Save-file format
Text file, fields separated by `|`, ends with an FNV-1a checksum:
```
ERTS 1
NEXT_ARRIVAL 5
NEXT_TREATED 3
WAITING 1
101|Ali Khan|45|3|1
HISTORY 2
103|Omar Farooq|70|1|3|1
102|Sara Ahmed|30|2|2|2
CHECKSUM 1229299386
```
Loading is all-or-nothing: checksum, header, counts, ranges, duplicate IDs and history order are all
verified before the live data is replaced; on any error the current data is left untouched.

## Edge cases handled
empty system · single patient · duplicate IDs · equal severities · invalid/out-of-range input (menu,
ID, name, age, severity) · patient not found · cancel confirmation · missing/corrupted/tampered file ·
end of input (Ctrl+Z / Ctrl+D) · unsaved-changes prompts on exit and load. Enter `q` at any data prompt to cancel.

Note: an ID only has to be unique among *waiting* patients; once a patient has been treated the ID may be registered again.

## Tests (`tests/test_main.cpp`)
852 checks: heap ordering/tie-break/remove/update/growth, hash insert/find/delete/resize/duplicates,
linked list traversal both ways, system-level edge cases, file round-trip and corruption cases,
plus a 20,000-step randomised comparison against a naive reference model. Also verified clean
under AddressSanitizer/UBSan (no leaks or memory errors).
