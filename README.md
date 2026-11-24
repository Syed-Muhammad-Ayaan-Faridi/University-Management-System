# University Management System (UMS)

FAST-NUCES Karachi · Data Structures project · C++17 · menu-driven console application
Group: 25K-3081 Sarim Sadiq, 25K-3103 Ayaan Faridi

Every data structure and algorithm in the proposal's mapping matrix is implemented from scratch
(no `std::vector`, `std::map`, `std::sort`, `std::list`, …). The standard library is used only for
I/O, `std::string`, file streams and timing.

## Build and run

| Platform | Command |
|---|---|
| Linux / macOS / MinGW with `make` | `make` then `./ums` |
| Windows (MinGW-w64 GCC, g++ on PATH) | double-click or run `build.bat`, then `ums.exe` |
| Dev-C++ | open `src/main.cpp` (or add it to a project) and set the compiler option **`-std=c++17`** (*Tools → Compiler Options → Settings → Code Generation → Language standard*). No include-path setup is needed |
| Any shell, from inside `src` | `g++ -std=c++17 main.cpp -o main` works with no extra flags |
| VS Code | build with `g++ -std=c++17 -O2 -Iinclude src/main.cpp -o ums` |

Run the self-tests with `make test` (or `tests_run.exe` on Windows). `make asan` runs them under
AddressSanitizer + UBSan to check memory safety.

On first launch the app loads built-in sample data (24 students incl. the two group members,
11 courses with prerequisites, 8 faculty, an 11-building campus). Records are saved to
`ums_students.txt`, `ums_courses.txt`, `ums_faculty.txt`, `ums_campus.txt` in the working
directory on exit and reloaded on the next start (menu 14 can also save, reload or reset).

## Project layout

```
include/
  ADT.h            DynArray, SafePtr, JaggedArray            (Rule of Three, bounds checks)
  LinkedLists.h    Singly, Doubly (cursor), Circular lists
  StackQueue.h     Stack (LIFO), Queue (FIFO)
  Heap.h           Binary-heap priority queue
  Searching.h      Linear, binary, interpolation search
  Sorting.h        Bubble, selection, insertion, comb, merge, quick, radix
  StringAlgo.h     KMP, Rabin-Karp
  HashTable.h      Chaining + open addressing, collisions, rehashing
  Trees.h          BST, AVL
  MultiwayTrees.h  2-3 tree, B-tree
  Graph.h          BFS, DFS, Dijkstra, Kruskal MST, topological sort
  Timetable.h      Recursive backtracking scheduler
  Models.h         Student, Course, Faculty
  University.h     Application core: wires every structure to its workflow
  Storage.h        Text-file persistence
  Utils.h          Validated console input, string helpers
src/main.cpp       Menus
tests/tests.cpp    ~6,250 assertions across all modules
```

## Syllabus matrix → implementation

| Syllabus topic | Where it lives | Used for | Menu |
|---|---|---|---|
| ADT, Rule of Three, safe pointers | `ADT.h` (`DynArray`, `SafePtr`); every owning class has destructor + copy ctor + copy assignment | Record storage, undo snapshots; bounds-checked access | 13 |
| Jagged array | `JaggedArray` in `ADT.h`; `University::buildGradebook/buildStudentSlots` | Per-student grade and timetable rows of different lengths | 4 |
| Linear / binary / interpolation search | `Searching.h` | Roll-number lookup with probe counts side by side | 5 |
| Bubble, selection, insertion, comb | `Sorting.h` | Sorting the student list, comparison counts | 6 |
| Merge, quick, radix | `Sorting.h` | Sorting by CGPA / name / roll; radix on integer keys; benchmark | 6 |
| Singly / doubly / circular lists | `LinkedLists.h` | Enrollments; next/previous browsing; rotating invigilators | 4, 1, 10 |
| Stack | `Stack` + `University::undoLast` | Undo of add / edit / delete for students, courses, faculty, enrollments, grades | 10 |
| Queue | `Queue` + `joinQueue` | Fee / registration counter, served in arrival order | 10 |
| Recursion and backtracking | `Timetable.h`, `University::tracePrereqs` | Clash-free timetable; recursive prerequisite chain | 11, 2 |
| BST and AVL | `Trees.h` | Roll-number index, range queries, height/rotation statistics | 7 |
| 2-3 tree and B-tree | `MultiwayTrees.h` | Indexes; B-tree nodes model disk pages (page reads reported) | 7 |
| KMP and Rabin-Karp | `StringAlgo.h` | Keyword search across student, course and faculty records | 8 |
| Heaps and priority queue | `Heap.h` + `University::meritList` | Top-k merit list / scholarship ranking | 9 |
| Hashing, collisions, rehashing | `HashTable.h` | Primary roll-number lookup; chaining vs open addressing demo | 7 |
| Graph representation and traversal | `Graph.h` (adjacency lists) | Campus map; BFS and DFS; connectivity | 12 |
| Shortest path | `Graph::shortestPath` (Dijkstra with the heap) | Building-to-building route in metres | 12 |
| Minimum spanning tree | `Graph::minimumSpanningTree` (Kruskal + union-find) | Minimum-cost campus cabling layout | 12 |
| Topological sort | `Graph::topologicalSort` (Kahn) | Valid course order; cycle detection on new prerequisites | 11, 2 |

## Behaviour worth knowing

* **Roll numbers** are numeric; type `3081` or `25K-3081`.
* **Roll-number indexes**: all five (hash, BST, AVL, 2-3, B-tree) map roll → position in the master
  array. The hash table serves normal lookups; the others are exposed in menu 7 so their behaviour
  can be compared. Ascending roll numbers make the plain BST a chain (height = n) while AVL, 2-3 and
  B-tree stay logarithmic. This is intentional and visible in menu 7 → option 3.
* **Enrollment** is checked against prerequisites: a student must have passed (grade ≥ 1.0) every
  prerequisite. Graded courses cannot be dropped. CGPA is the credit-weighted average of graded
  courses and is recalculated whenever a grade is recorded.
* **Prerequisites** are a directed graph; adding one that would create a cycle is rejected.
* **Timetable**: two courses may not share a slot if a student is currently taking both
  (completed courses don't count), if they have the same instructor, or if one is the other's direct
  prerequisite. Courses are tried most-constrained first, with symmetry breaking so interchangeable
  empty slots are not retried. A search limit stops runaway searches. Rooms and capacities are not
  modelled (consistent with the proposal's scope).
* **Undo** covers add / edit / delete of students, courses and faculty, plus enrollment, drop and
  grade changes, and added prerequisites. An undone delete restores the record at the end of the
  list. Courses and faculty with dependants (enrolled students, dependent courses, taught courses)
  cannot be deleted, so undo never has to repair dangling references.
* **Persistence** uses `|`-delimited text; `|` typed into a field is replaced by `/`. Bad or
  duplicate lines are skipped and reported on load.

## Testing


## Limitations

* Developed and tested with GCC 13 on Linux. The code uses only standard C++17 (no `<filesystem>`, no
  platform APIs), so it should build with MinGW-w64 GCC, but it was not compiled on Windows or in
  Dev-C++ here; if Dev-C++ complains, check that C++17 is enabled.
* The 2-3 tree and B-tree support insert, search and range queries but not deletion. After a deletion
  `University` rebuilds all indexes (O(n log n)), which is fine at this scale. BST and AVL do implement
  deletion.
* Sample data is generated deterministically; grades are pseudo-random, not real.
