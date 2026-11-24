// main.cpp - University Management System (menu-driven console application)
// FAST-NUCES Karachi | Data Structures project | C++17
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

#include "../include/Storage.h"
#include "../include/StringAlgo.h"
#include "../include/Sorting.h"
#include "../include/University.h"

using namespace std;
using namespace ums;

//  Printing helpers
static void rule(int n = 78) { cout << string(n, '-') << "\n"; }

static void printStudentHeader()
{
    cout << left << setw(10) << "Roll No" << setw(20) << "Name" << setw(5) << "Sec" << setw(6) << "Dept" << setw(7) << "CGPA" << "Courses\n";
    rule(56);
}
static void printStudentRow(const Student &s)
{
    cout << left << setw(10) << formatRoll(s.id) << setw(20) << clip(s.name, 19) << setw(5) << s.section << setw(6) << s.dept << fixed << setprecision(2) << setw(7) << s.cgpa << s.enrollments.size() << "\n";
}
static void printStudentDetail(const University &u, const Student &s)
{
    cout << "\n  Roll No : " << formatRoll(s.id) << "\n  Name    : " << s.name << "\n  Section : " << s.section << "\n  Dept    : " << s.dept << "\n  CGPA    : " << fixed << setprecision(2) << s.cgpa << "\n";
    cout << "  Enrollments (" << s.enrollments.size() << "):\n";
    for (const SinglyLinkedList<Enrollment>::Node *p = s.enrollments.head(); p; p = p->next)
    {
        int ci = u.findCourseIndex(p->value.courseCode);
        cout << "    " << left << setw(7) << p->value.courseCode << setw(36) << (ci >= 0 ? clip(u.courses[ci].title, 35) : "(unknown course)");
        if (p->value.graded())
            cout << "grade " << setprecision(1) << p->value.grade << "\n";
        else
            cout << "in progress\n";
    }
}
static void printCourseHeader()
{
    cout << left << setw(8) << "Code" << setw(36) << "Title" << setw(5) << "Cr" << setw(8) << "Fac.ID" << setw(18) << "Prerequisites" << "Slot\n";
    rule(88);
}
static void printCourseRow(const Course &c)
{
    string pre;
    for (int i = 0; i < c.prereqs.size(); ++i)
        pre += (i ? "," : "") + c.prereqs[i];
    if (pre.empty())
        pre = "-";
    cout << left << setw(8) << c.code << setw(36) << clip(c.title, 35) << setw(5) << c.credits << setw(8) << c.facultyId << setw(18) << clip(pre, 17) << slotLabel(c.slot) << "\n";
}
static void printSortedStudents(const DynArray<Student> &v, int limit = 15)
{
    printStudentHeader();
    for (int i = 0; i < v.size() && i < limit; ++i)
        printStudentRow(v[i]);
    if (v.size() > limit)
        cout << "  ... " << v.size() - limit << " more\n";
}
static string facultyName(const University &u, int id)
{
    int i = u.findFacultyIndex(id);
    return i >= 0 ? u.faculty[i].name : "(unknown)";
}

// Reads a roll number and verifies it exists; returns -1 if not found.
static int pickStudent(const University &u, const string &prompt = "Roll number: ")
{
    int id = readRoll(prompt);
    if (u.findStudentIndex(id) < 0)
    {
        cout << "  No student with roll number " << formatRoll(id) << ".\n";
        return -1;
    }
    return id;
}
static void report(bool ok, const string &okMsg, const string &err) { cout << (ok ? "  OK: " + okMsg : "  Error: " + err) << "\n"; }

//  1. Students
static void studentMenu(University &u)
{
    while (true)
    {
        cout << "\n=== STUDENT RECORDS ===\n 1. Add student\n 2. View all students\n 3. View one student\n"
                " 4. Update student\n 5. Delete student\n 6. Browse students (doubly linked list: next/previous)\n 0. Back\n";
        int ch = readInt("Choice: ", 0, 6);
        string err;
        if (ch == 0)
            return;
        if (ch == 1)
        {
            Student s;
            s.id = readRoll("Roll number (e.g. 3081): ");
            s.name = readNonEmpty("Name: ");
            s.section = upperCopy(readNonEmpty("Section: "));
            s.dept = upperCopy(readNonEmpty("Department: "));
            s.cgpa = readDouble("Starting CGPA (0-4): ", 0.0, 4.0);
            report(u.addStudent(s, err), "student added.", err);
        }
        else if (ch == 2)
        {
            printStudentHeader();
            for (int i = 0; i < u.students.size(); ++i)
                printStudentRow(u.students[i]);
            cout << u.students.size() << " student(s).\n";
        }
        else if (ch == 3)
        {
            int id = pickStudent(u);
            if (id > 0)
                printStudentDetail(u, u.students[u.findStudentIndex(id)]);
        }
        else if (ch == 4)
        {
            int id = pickStudent(u);
            if (id < 0)
                continue;
            const Student &cur = u.students[u.findStudentIndex(id)];
            string n = readLine("Name [" + cur.name + "]: ");
            string sec = readLine("Section [" + cur.section + "]: ");
            string d = readLine("Department [" + cur.dept + "]: ");
            string g = readLine("CGPA [" + fmtGrade(cur.cgpa) + "]: ");
            double cg = cur.cgpa;
            if (!g.empty() && !parseDoubleStrict(g, cg))
            {
                cout << "  Error: invalid CGPA.\n";
                continue;
            }
            report(u.updateStudent(id, n.empty() ? cur.name : sanitize(n), sec.empty() ? cur.section : upperCopy(sanitize(sec)), d.empty() ? cur.dept : upperCopy(sanitize(d)), cg, err), "student updated (undo available).", err);
        }
        else if (ch == 5)
        {
            int id = readRoll("Roll number to delete: ");
            if (u.findStudentIndex(id) >= 0 && !askYesNo("Delete " + formatRoll(id) + "?"))
                continue;
            report(u.deleteStudent(id, err), "student deleted (undo available).", err);
        }
        else if (ch == 6)
        {
            if (u.students.empty())
            {
                cout << "  No students.\n";
                continue;
            }
            DoublyLinkedList<int> list;
            for (int i = 0; i < u.students.size(); ++i)
                list.pushBack(i);
            while (true)
            {
                const Student &s = u.students[list.current()];
                cout << "\n  [" << list.position() + 1 << "/" << list.size() << "]  " << formatRoll(s.id) << "  " << s.name << "  " << s.dept << "-" << s.section << "  CGPA " << fixed << setprecision(2) << s.cgpa << "\n";
                string c = upperCopy(readLine("  (N)ext, (P)revious, (Q)uit: "));
                if (c == "N")
                {
                    if (!list.moveNext())
                        cout << "  Already at the last record.\n";
                }
                else if (c == "P")
                {
                    if (!list.movePrev())
                        cout << "  Already at the first record.\n";
                }
                else if (c == "Q")
                    break;
            }
        }
    }
}

//  2. Courses and prerequisites
static void courseMenu(University &u)
{
    while (true)
    {
        cout << "\n=== COURSE RECORDS ===\n 1. Add course\n 2. List courses\n 3. Add prerequisite\n 4. Delete course\n"
                " 5. Trace prerequisite chain (recursion)\n 0. Back\n";
        int ch = readInt("Choice: ", 0, 5);
        string err;
        if (ch == 0)
            return;
        if (ch == 1)
        {
            Course c;
            c.code = readNonEmpty("Course code (e.g. CS205): ");
            c.title = readNonEmpty("Title: ");
            c.credits = readInt("Credit hours (1-6): ", 1, 6);
            c.facultyId = readInt("Instructor faculty id: ", 1, 999999);
            report(u.addCourse(c, err), "course added.", err);
        }
        else if (ch == 2)
        {
            printCourseHeader();
            for (int i = 0; i < u.courses.size(); ++i)
                printCourseRow(u.courses[i]);
        }
        else if (ch == 3)
        {
            string c = readLine("Course code: "), p = readLine("Prerequisite course code: ");
            report(u.addPrerequisite(c, p, err), "prerequisite added.", err);
        }
        else if (ch == 4)
        {
            report(u.deleteCourse(readLine("Course code to delete: "), err), "course deleted (undo available).", err);
        }
        else if (ch == 5)
        {
            string c = readLine("Course code: ");
            if (u.findCourseIndex(c) < 0)
            {
                cout << "  No such course.\n";
                continue;
            }
            cout << "\nPrerequisite chain:\n";
            u.tracePrereqs(c, 0, cout);
        }
    }
}

//  3. Faculty
static void facultyMenu(University &u)
{
    while (true)
    {
        cout << "\n=== FACULTY RECORDS ===\n 1. Add faculty\n 2. List faculty\n 3. Update faculty\n 4. Delete faculty\n 0. Back\n";
        int ch = readInt("Choice: ", 0, 4);
        string err;
        if (ch == 0)
            return;
        if (ch == 1)
        {
            int id = readInt("Faculty id: ", 1, 999999);
            string n = readNonEmpty("Name: "), d = upperCopy(readNonEmpty("Department: "));
            report(u.addFaculty(Faculty(id, n, d), err), "faculty added.", err);
        }
        else if (ch == 2)
        {
            cout << left << setw(8) << "ID" << setw(26) << "Name" << "Dept\n";
            rule(44);
            for (int i = 0; i < u.faculty.size(); ++i)
                cout << left << setw(8) << u.faculty[i].id << setw(26) << u.faculty[i].name << u.faculty[i].dept << "\n";
        }
        else if (ch == 3)
        {
            int id = readInt("Faculty id: ", 1, 999999);
            int i = u.findFacultyIndex(id);
            if (i < 0)
            {
                cout << "  No such faculty member.\n";
                continue;
            }
            string n = readLine("Name [" + u.faculty[i].name + "]: "), d = readLine("Dept [" + u.faculty[i].dept + "]: ");
            report(u.updateFaculty(id, n.empty() ? u.faculty[i].name : sanitize(n), d.empty() ? u.faculty[i].dept : upperCopy(sanitize(d)), err), "faculty updated (undo available).", err);
        }
        else if (ch == 4)
        {
            report(u.deleteFaculty(readInt("Faculty id: ", 1, 999999), err), "faculty deleted (undo available).", err);
        }
    }
}

//  4. Enrollment, grades, gradebook (jagged array)
static void enrollmentMenu(University &u)
{
    while (true)
    {
        cout << "\n--- ENROLLMENT & GRADES ---\n 1. Enroll student in course\n 2. Drop course\n 3. Record grade\n 4. Gradebook (jagged array)\n 5. Student timetable (jagged array)\n 0. Back\n";
        int ch = readInt("Choice: ", 0, 5);
        string err;
        if (ch == 0)
            return;
        if (ch == 1)
        {
            int id = readRoll("Roll number: ");
            report(u.enroll(id, readLine("Course code: "), err), "enrolled (prerequisites verified).", err);
        }
        else if (ch == 2)
        {
            int id = readRoll("Roll number: ");
            report(u.dropCourse(id, readLine("Course code: "), err), "course dropped.", err);
        }
        else if (ch == 3)
        {
            int id = readRoll("Roll number: ");
            string code = readLine("Course code: ");
            double g = readDouble("Grade points (0.0 - 4.0): ", 0.0, 4.0);
            report(u.setGrade(id, code, g, err), "grade recorded, CGPA recalculated.", err);
        }
        else if (ch == 4)
        {
            JaggedArray<GradeCell> gb = u.buildGradebook();
            cout << "\nEach row has a different length (students take different numbers of courses).\n";
            for (int r = 0; r < gb.rowCount(); ++r)
            {
                cout << left << setw(10) << formatRoll(u.students[r].id) << setw(16) << clip(u.students[r].name, 15) << "(" << gb.rowLength(r) << ") ";
                for (int c = 0; c < gb.rowLength(r); ++c)
                {
                    cout << gb.at(r, c).code << "=";
                    if (gb.at(r, c).grade < 0)
                        cout << "--";
                    else
                        cout << fixed << setprecision(1) << gb.at(r, c).grade;
                    cout << " ";
                }
                cout << "\n";
            }
            cout << "Total cells: " << gb.totalCells() << " (a rectangular table would need " << gb.rowCount() * 10 << ").\n";
        }
        else if (ch == 5)
        {
            int id = pickStudent(u);
            if (id < 0)
                continue;
            int si = u.findStudentIndex(id);
            JaggedArray<int> slots = u.buildStudentSlots();
            cout << "\nTimetable for " << u.students[si].name << " (" << slots.rowLength(si) << " courses):\n";
            int col = 0;
            for (const SinglyLinkedList<Enrollment>::Node *p = u.students[si].enrollments.head(); p; p = p->next, ++col)
                if (!p->value.graded())
                    cout << "  " << left << setw(8) << p->value.courseCode << slotLabel(slots.at(si, col)) << "\n";
            cout << "  (generate the timetable first if slots show as Unscheduled)\n";
        }
    }
}

//  5. Searching
static void searchMenu(University &u)
{
    while (true)
    {
        cout << "\n=== SEARCHING ===\n 1. Find by roll number: linear vs binary vs interpolation\n"
                " 2. Find by exact name (linear search, unsorted)\n 0. Back\n";
        int ch = readInt("Choice: ", 0, 2);
        if (ch == 0)
            return;
        if (ch == 1)
        {
            int id = readRoll("Roll number: ");
            int n = u.students.size();
            auto keyOf = [](const Student &s)
            { return s.id; };
            DynArray<Student> sorted = u.students;
            radixSort(sorted.data(), n, keyOf); // sorted copy for binary / interpolation
            SearchResult a = linearSearch(u.students.data(), n, id, keyOf);
            SearchResult b = binarySearch(sorted.data(), n, id, keyOf);
            SearchResult c = interpolationSearch(sorted.data(), n, id, keyOf);
            cout << "\n  " << left << setw(22) << "Algorithm" << setw(12) << "Found?" << "Probes\n  " << string(44, '-') << "\n";
            cout << "  " << left << setw(22) << "Linear (unsorted)" << setw(12) << (a.index >= 0 ? "yes" : "no") << a.probes << "\n";
            cout << "  " << left << setw(22) << "Binary (sorted)" << setw(12) << (b.index >= 0 ? "yes" : "no") << b.probes << "\n";
            cout << "  " << left << setw(22) << "Interpolation" << setw(12) << (c.index >= 0 ? "yes" : "no") << c.probes << "\n";
            if (a.index >= 0)
            {
                cout << "\n";
                printStudentHeader();
                printStudentRow(u.students[a.index]);
            }
        }
        else
        {
            string name = toLowerCopy(readNonEmpty("Exact name (case-insensitive): "));
            int i = linearFindIf(u.students.data(), u.students.size(), [&](const Student &s)
                                 { return toLowerCopy(s.name) == name; });
            if (i < 0)
                cout << "  No student with that name.\n";
            else
            {
                printStudentHeader();
                printStudentRow(u.students[i]);
            }
        }
    }
}

//  6. Sorting
static const char *kAlgoNames[7] = {"Bubble", "Selection", "Insertion", "Comb", "Merge", "Quick", "Radix"};

template <typename Less>
static SortStats runStudentSort(int alg, Student *a, int n, Less less)
{
    switch (alg)
    {
    case 0:
        return bubbleSort(a, n, less);
    case 1:
        return selectionSort(a, n, less);
    case 2:
        return insertionSort(a, n, less);
    case 3:
        return combSort(a, n, less);
    case 4:
        return mergeSort(a, n, less);
    default:
        return quickSort(a, n, less);
    }
}

static void sortMenu(University &u)
{
    while (true)
    {
        cout << "\n=== SORTING ===\n 1. Sort the student list\n 2. Benchmark all algorithms on random data\n 0. Back\n";
        int ch = readInt("Choice: ", 0, 2);
        if (ch == 0)
            return;
        if (ch == 1)
        {
            cout << " Field: 1) CGPA (high to low)  2) Name (A-Z)  3) Roll number\n";
            int field = readInt("Field: ", 1, 3);
            cout << " Algorithm: 1) Bubble 2) Selection 3) Insertion 4) Comb 5) Merge 6) Quick 7) Radix\n";
            int alg = readInt("Algorithm: ", 1, 7) - 1;
            DynArray<Student> v = u.students;
            int n = v.size();
            SortStats st;
            auto t0 = chrono::steady_clock::now();
            if (alg == 6)
            {
                if (field == 2)
                {
                    cout << "  Radix sort needs a numeric key; names are strings. Use another algorithm.\n";
                    continue;
                }
                if (field == 1)
                    st = radixSort(v.data(), n, [](const Student &s)
                                   { return 400 - (int)lround(s.cgpa * 100); });
                else
                    st = radixSort(v.data(), n, [](const Student &s)
                                   { return s.id; });
            }
            else if (field == 1)
            {
                st = runStudentSort(alg, v.data(), n, [](const Student &a, const Student &b)
                                    { return a.cgpa > b.cgpa; });
            }
            else if (field == 2)
            {
                st = runStudentSort(alg, v.data(), n, [](const Student &a, const Student &b)
                                    { return toLowerCopy(a.name) < toLowerCopy(b.name); });
            }
            else
            {
                st = runStudentSort(alg, v.data(), n, [](const Student &a, const Student &b)
                                    { return a.id < b.id; });
            }
            double ms = chrono::duration<double, milli>(chrono::steady_clock::now() - t0).count();
            printSortedStudents(v);
            cout << kAlgoNames[alg] << " sort: " << st.comparisons << " comparisons, " << st.moves << " moves, " << fixed << setprecision(3) << ms << " ms\n";
        }
        else
        {
            int n = readInt("Number of random integers (10 - 10000): ", 10, 10000);
            cout << " Input pattern: 1) random  2) already sorted  3) reverse sorted\n";
            int pat = readInt("Pattern: ", 1, 3);
            DynArray<int> base;
            unsigned int x = 12345;
            for (int i = 0; i < n; ++i)
            {
                x = x * 1664525u + 1013904223u;
                base.push_back((int)((x >> 8) % 1000000));
            }
            if (pat != 1)
                quickSort(base.data(), n, [](int a, int b)
                          { return a < b; });
            if (pat == 3)
                for (int i = 0, j = n - 1; i < j; ++i, --j)
                    swap(base[i], base[j]);
            cout << "\n  " << left << setw(12) << "Algorithm" << right << setw(16) << "Comparisons" << setw(14) << "Moves" << setw(12) << "Time (ms)" << "  Sorted?\n  " << string(62, '-') << "\n";
            auto less = [](int a, int b)
            { return a < b; };
            for (int alg = 0; alg < 7; ++alg)
            {
                DynArray<int> v = base;
                auto t0 = chrono::steady_clock::now();
                SortStats st;
                switch (alg)
                {
                case 0:
                    st = bubbleSort(v.data(), n, less);
                    break;
                case 1:
                    st = selectionSort(v.data(), n, less);
                    break;
                case 2:
                    st = insertionSort(v.data(), n, less);
                    break;
                case 3:
                    st = combSort(v.data(), n, less);
                    break;
                case 4:
                    st = mergeSort(v.data(), n, less);
                    break;
                case 5:
                    st = quickSort(v.data(), n, less);
                    break;
                default:
                    st = radixSort(v.data(), n, [](int a)
                                   { return a; });
                    break;
                }
                double ms = chrono::duration<double, milli>(chrono::steady_clock::now() - t0).count();
                cout << "  " << left << setw(12) << kAlgoNames[alg] << right << setw(16) << st.comparisons << setw(14) << st.moves << setw(12) << fixed << setprecision(3) << ms << "  " << (isSorted(v.data(), n, less) ? "yes" : "NO") << "\n";
            }
            cout << "  (Radix sort performs no key comparisons; it distributes by digit.)\n";
        }
    }
}

//  7. Indexing: hash table, BST, AVL, 2-3 tree, B-tree
static void hashDemo()
{
    int n = readInt("How many keys to insert (5 - 60): ", 5, 60);
    ChainedHashTable chain(7);
    OpenAddressHashTable open(7);
    cout << "\nKeys are 3, 10, 17, 24 ... (all congruent mod 7) into tables that start with 7 slots,\n"
            "so they collide heavily until rehashing grows the table to a larger prime.\n";
    for (int i = 0; i < n; ++i)
    {
        chain.insert(3 + 7 * i, i);
        open.insert(3 + 7 * i, i);
    }
    cout << fixed << setprecision(2);
    cout << "\n  " << left << setw(26) << "" << setw(16) << "Chaining" << "Open addressing\n  " << string(58, '-') << "\n";
    cout << "  " << left << setw(26) << "Keys stored" << setw(16) << chain.size() << open.size() << "\n";
    cout << "  " << left << setw(26) << "Capacity (prime)" << setw(16) << chain.capacity() << open.capacity() << "\n";
    cout << "  " << left << setw(26) << "Load factor" << setw(16) << chain.loadFactor() << open.loadFactor() << "\n";
    cout << "  " << left << setw(26) << "Collisions" << setw(16) << chain.collisions() << open.collisions() << "\n";
    cout << "  " << left << setw(26) << "Rehashes" << setw(16) << chain.rehashes() << open.rehashes() << "\n";
    cout << "\nChained table buckets:\n";
    chain.dump(cout, 14);
    cout << "\nOpen-addressing slots:\n";
    open.dump(cout, 14);
}

static void indexMenu(University &u)
{
    while (true)
    {
        cout << "\n=== INDEXING & HASHING ===\n 1. Look up a roll number in every index\n 2. Range query (AVL vs BST vs B-tree vs 2-3 tree)\n"
                " 3. Index statistics (height, balance, pages)\n 4. Draw BST and AVL (top levels)\n 5. Draw B-tree and 2-3 tree (top levels)\n"
                " 6. Hash table demo: collisions & rehashing\n 0. Back\n";
        int ch = readInt("Choice: ", 0, 6);
        if (ch == 0)
            return;
        if (ch == 1)
        {
            int id = readRoll("Roll number: ");
            int v = -1;
            cout << "\n  " << left << setw(24) << "Index" << setw(10) << "Found?" << "Nodes visited / probes\n  " << string(52, '-') << "\n";
            bool f = u.idxHash.find(id, v);
            cout << "  " << left << setw(24) << "Hash table (chaining)" << setw(10) << (f ? "yes" : "no") << u.idxHash.lastProbes() << "\n";
            f = u.bst.find(id, v);
            cout << "  " << left << setw(24) << "BST" << setw(10) << (f ? "yes" : "no") << u.bst.lastVisited() << "\n";
            f = u.avl.find(id, v);
            cout << "  " << left << setw(24) << "AVL tree" << setw(10) << (f ? "yes" : "no") << u.avl.lastVisited() << "\n";
            f = u.t23.find(id, v);
            cout << "  " << left << setw(24) << "2-3 tree" << setw(10) << (f ? "yes" : "no") << u.t23.lastVisited() << "\n";
            f = u.btree.find(id, v);
            cout << "  " << left << setw(24) << "B-tree (disk pages)" << setw(10) << (f ? "yes" : "no") << u.btree.lastVisited() << "\n";
            if (f && v >= 0)
            {
                cout << "\n";
                printStudentHeader();
                printStudentRow(u.students[v]);
            }
        }
        else if (ch == 2)
        {
            int lo = readRoll("From roll number: "), hi = readRoll("To roll number: ");
            if (lo > hi)
                swap(lo, hi);
            DynArray<int> k1, v1, k2, v2, k3, v3, k4, v4;
            u.avl.rangeQuery(lo, hi, k1, v1);
            int avlV = u.avl.lastVisited();
            u.bst.rangeQuery(lo, hi, k2, v2);
            int bstV = u.bst.lastVisited();
            u.btree.rangeQuery(lo, hi, k3, v3);
            int btV = u.btree.lastVisited();
            u.t23.rangeQuery(lo, hi, k4, v4);
            int t23V = u.t23.lastVisited();
            cout << "\n  Nodes visited:  AVL " << avlV << " | BST " << bstV << " | B-tree " << btV << " | 2-3 tree " << t23V << "\n  " << k1.size() << " student(s) in range (ascending roll number):\n";
            printStudentHeader();
            for (int i = 0; i < v1.size(); ++i)
                printStudentRow(u.students[v1[i]]);
        }
        else if (ch == 3)
        {
            int n = u.students.size();
            cout << "\n  Records indexed: " << n << "   (minimum possible height: " << (n ? (int)ceil(log2(n + 1)) : 0) << ")\n";
            cout << "  BST       height " << u.bst.height() << "   <- unbalanced: ascending roll numbers form a chain\n";
            cout << "  AVL       height " << u.avl.height() << "   rotations " << u.avl.rotations() << "   balanced: " << (u.avl.isBalanced() ? "yes" : "NO") << "\n";
            cout << "  2-3 tree  height " << u.t23.height() << "   nodes " << u.t23.nodeCount() << "   leaves equal depth: " << (u.t23.leavesAtSameDepth() ? "yes" : "NO") << "\n";
            cout << "  B-tree    height " << u.btree.height() << "   pages " << u.btree.nodeCount() << " (max " << BTree::maxKeysPerNode() << " keys/page)   leaves equal depth: " << (u.btree.leavesAtSameDepth() ? "yes" : "NO") << "\n";
            cout << "  Hash      capacity " << u.idxHash.capacity() << "   load " << fixed << setprecision(2) << u.idxHash.loadFactor() << "   collisions " << u.idxHash.collisions() << "   rehashes " << u.idxHash.rehashes() << "\n";
        }
        else if (ch == 4)
        {
            cout << "\nBST (right subtree drawn above, root at left):\n";
            u.bst.printTop(cout, 5);
            cout << "\nAVL tree:\n";
            u.avl.printTop(cout, 5);
        }
        else if (ch == 5)
        {
            cout << "\n2-3 tree:\n";
            u.t23.print(cout, 4);
            cout << "\nB-tree (each [..] is one disk page):\n";
            u.btree.print(cout, 4);
        }
        else
        {
            hashDemo();
        }
    }
}

//  8. Text search (KMP / Rabin-Karp)
static void textSearchMenu(University &u)
{
    while (true)
    {
        string pat = readLine("\nKeyword to search for (blank = back): ");
        if (pat.empty())
            return;
        cout << " Algorithm: 1) KMP  2) Rabin-Karp\n";
        int alg = readInt("Algorithm: ", 1, 2);
        string p = toLowerCopy(pat);
        int cmp = 0, spurious = 0;
        int hits = 0;
        auto test = [&](const string &hay)
        {
            string h = toLowerCopy(hay);
            MatchResult r = alg == 1 ? kmpSearch(h, p) : rabinKarpSearch(h, p);
            cmp += r.comparisons;
            spurious += r.spurious;
            return r.positions.size() > 0;
        };
        cout << "\nStudents:\n";
        for (int i = 0; i < u.students.size(); ++i)
            if (test(u.students[i].name + " " + u.students[i].dept))
            {
                cout << "  " << formatRoll(u.students[i].id) << "  " << u.students[i].name << "  (" << u.students[i].dept << ")\n";
                ++hits;
            }
        cout << "Courses:\n";
        for (int i = 0; i < u.courses.size(); ++i)
            if (test(u.courses[i].code + " " + u.courses[i].title))
            {
                cout << "  " << u.courses[i].code << "  " << u.courses[i].title << "\n";
                ++hits;
            }
        cout << "Faculty:\n";
        for (int i = 0; i < u.faculty.size(); ++i)
            if (test(u.faculty[i].name + " " + u.faculty[i].dept))
            {
                cout << "  " << u.faculty[i].id << "  " << u.faculty[i].name << "\n";
                ++hits;
            }
        cout << "\n"
             << hits << " match(es) using " << (alg == 1 ? "KMP" : "Rabin-Karp") << "; " << cmp << " character comparisons";
        if (alg == 2)
            cout << ", " << spurious << " spurious hash hits";
        cout << ".\n";
    }
}

//  9. Merit list / scholarships
static void scholarshipMenu(University &u)
{
    int k = readInt("How many scholarships / merit positions (1-100): ", 1, 100);
    double minCg = readDouble("Minimum CGPA to qualify (0-4): ", 0.0, 4.0);
    DynArray<int> top = u.meritList(k, minCg);
    cout << "\nMERIT LIST (extracted from a max-heap, best first)\n";
    cout << left << setw(6) << "Rank" << setw(10) << "Roll No" << setw(20) << "Name" << setw(6) << "Dept" << "CGPA\n";
    rule(50);
    for (int i = 0; i < top.size(); ++i)
    {
        const Student &s = u.students[top[i]];
        cout << left << setw(6) << i + 1 << setw(10) << formatRoll(s.id) << setw(20) << clip(s.name, 19) << setw(6) << s.dept << fixed << setprecision(2) << s.cgpa << "\n";
    }
    if (top.empty())
        cout << "  No student meets the CGPA threshold.\n";
}

//  10. Queue, undo, invigilators
static void operationsMenu(University &u)
{
    while (true)
    {
        cout << "\n=== QUEUES, UNDO & INVIGILATION ===\n 1. Join registration / fee queue\n 2. Serve next in queue\n 3. View queue\n"
                " 4. Undo last edit/deletion (stack)\n 5. View undo history\n 6. Assign exam invigilators (circular list)\n 0. Back\n";
        int ch = readInt("Choice: ", 0, 6);
        string err;
        if (ch == 0)
            return;
        if (ch == 1)
        {
            int id = readRoll("Roll number: ");
            cout << " Purpose: 1) Fee payment  2) Course registration  3) Transcript request\n";
            const char *purposes[3] = {"Fee payment", "Course registration", "Transcript request"};
            Ticket t;
            if (u.joinQueue(id, purposes[readInt("Purpose: ", 1, 3) - 1], t, err))
                cout << "  Ticket #" << t.number << " issued. People ahead of you: " << u.feeQueue.size() - 1 << "\n";
            else
                cout << "  Error: " << err << "\n";
        }
        else if (ch == 2)
        {
            Ticket t;
            if (!u.feeQueue.dequeue(t))
            {
                cout << "  Queue is empty.\n";
                continue;
            }
            int si = u.findStudentIndex(t.studentId);
            cout << "  Now serving ticket #" << t.number << ": " << (si >= 0 ? u.students[si].name : "(record removed)") << " - " << t.purpose << "\n";
        }
        else if (ch == 3)
        {
            if (u.feeQueue.empty())
            {
                cout << "  Queue is empty.\n";
                continue;
            }
            cout << "  Front of queue first (" << u.feeQueue.size() << " waiting):\n";
            u.feeQueue.forEach([&](const Ticket &t)
                               {
                int si = u.findStudentIndex(t.studentId);
                cout << "   #" << t.number << "  " << formatRoll(t.studentId) << "  " << (si >= 0 ? u.students[si].name : "?") << "  - " << t.purpose << "\n"; });
        }
        else if (ch == 4)
        {
            string msg;
            u.undoLast(msg);
            cout << "  " << msg << "\n";
        }
        else if (ch == 5)
        {
            if (u.undoStack.empty())
            {
                cout << "  Undo history is empty.\n";
                continue;
            }
            cout << "  Most recent first:\n";
            int n = 0;
            u.undoStack.forEach([&](const Action &a)
                                { cout << "   " << ++n << ". " << a.description << "\n"; });
        }
        else if (ch == 6)
        {
            if (u.invigilators.size() < 2 || u.courses.empty())
            {
                cout << "  Need at least two faculty members and one course.\n";
                continue;
            }
            int shifts = readInt("Invigilators per paper (1-3): ", 1, 3);
            cout << "\nEach paper takes the next faculty member from the circular rotation (its own instructor is skipped).\n";
            for (int i = 0; i < u.courses.size(); ++i)
            {
                cout << "  " << left << setw(8) << u.courses[i].code;
                for (int s = 0; s < shifts; ++s)
                {
                    int fid = u.nextInvigilator(u.courses[i].facultyId);
                    cout << (s ? ", " : "") << facultyName(u, fid);
                }
                cout << "\n";
            }
        }
    }
}

//  11. Timetable and prerequisites
static void timetableMenu(University &u)
{
    while (true)
    {
        cout << "\n=== TIMETABLE & PREREQUISITES ===\n 1. Valid course order (topological sort)\n 2. Generate clash-free timetable (backtracking)\n"
                " 3. View timetable\n 4. Set number of time slots (currently "
             << u.slotCount << ")\n 5. Can a student take a course? (prerequisite check)\n 0. Back\n";
        int ch = readInt("Choice: ", 0, 5);
        if (ch == 0)
            return;
        if (ch == 1)
        {
            DynArray<int> order;
            Graph g = u.buildPrereqGraph();
            if (!g.topologicalSort(order))
            {
                cout << "  The prerequisite graph has a cycle; no valid order exists.\n";
                continue;
            }
            cout << "\nA valid order in which courses can be taken:\n  ";
            for (int i = 0; i < order.size(); ++i)
                cout << (i ? " -> " : "") << g.name(order[i]);
            cout << "\n";
        }
        else if (ch == 2)
        {
            TimetableResult res;
            DynArray<int> order;
            string err;
            if (!u.generateTimetable(res, order, err))
            {
                cout << "  " << err << "  (" << res.nodes << " placements tried, " << res.backtracks << " backtracks)\n";
                continue;
            }
            cout << "  Timetable generated using " << res.slotsUsed << " of " << u.slotCount << " slots.\n  Search effort: " << res.nodes << " placements examined, " << res.backtracks << " backtracks.\n";
            cout << "  Rules enforced: no shared students, no shared instructor, no course alongside its direct prerequisite.\n";
        }
        else if (ch == 3)
        {
            bool any = false;
            for (int s = 0; s < u.slotCount; ++s)
            {
                string line;
                for (int i = 0; i < u.courses.size(); ++i)
                    if (u.courses[i].slot == s)
                        line += (line.empty() ? "" : ", ") + u.courses[i].code;
                if (!line.empty())
                {
                    cout << "  " << left << setw(14) << slotLabel(s) << line << "\n";
                    any = true;
                }
            }
            if (!any)
                cout << "  No timetable yet. Choose option 2 first.\n";
        }
        else if (ch == 4)
        {
            u.slotCount = readInt("Number of slots (1-40): ", 1, 40);
            cout << "  Slots set to " << u.slotCount << ". Regenerate the timetable to apply.\n";
        }
        else if (ch == 5)
        {
            int id = pickStudent(u);
            if (id < 0)
                continue;
            string code = readLine("Course code: ");
            int ci = u.findCourseIndex(code);
            if (ci < 0)
            {
                cout << "  No such course.\n";
                continue;
            }
            const Student &s = u.students[u.findStudentIndex(id)];
            bool ok = true;
            for (int p = 0; p < u.courses[ci].prereqs.size(); ++p)
            {
                const Enrollment *e = s.findEnrollment(u.courses[ci].prereqs[p]);
                bool passed = e && e->graded() && e->grade >= 1.0;
                cout << "  Prerequisite " << u.courses[ci].prereqs[p] << ": " << (passed ? "passed" : "NOT passed") << "\n";
                ok = ok && passed;
            }
            if (u.courses[ci].prereqs.empty())
                cout << "  No prerequisites.\n";
            cout << (ok ? "  => Eligible to enroll.\n" : "  => Not eligible yet.\n");
        }
    }
}

//  12. Campus map (graph)
static int pickBuilding(const University &u, const string &prompt)
{
    for (int i = 0; i < u.campus.vertexCount(); ++i)
        cout << "  " << setw(2) << i + 1 << ". " << u.campus.name(i) << "\n";
    return readInt(prompt, 1, u.campus.vertexCount()) - 1;
}
static void printPath(const Graph &g, const DynArray<int> &path)
{
    for (int i = 0; i < path.size(); ++i)
        cout << (i ? " -> " : "") << g.name(path[i]);
    cout << "\n";
}
static void campusMenu(University &u)
{
    while (true)
    {
        cout << "\n=== CAMPUS MAP (GRAPH) ===\n 1. Show map\n 2. BFS traversal\n 3. DFS traversal\n 4. Connectivity\n"
                " 5. Shortest route in metres (Dijkstra)\n 6. Fewest-buildings route (BFS)\n 7. Minimum spanning tree (cabling layout)\n"
                " 8. Add building\n 9. Add walkway\n 0. Back\n";
        int ch = readInt("Choice: ", 0, 9);
        const Graph &g = u.campus;
        if (ch == 0)
            return;
        if (g.vertexCount() == 0 && ch != 8)
        {
            cout << "  The map is empty.\n";
            continue;
        }
        if (ch == 1)
        {
            cout << "\n"
                 << g.vertexCount() << " buildings, " << g.edgeCount() << " walkways:\n";
            for (int i = 0; i < g.vertexCount(); ++i)
            {
                cout << "  " << left << setw(16) << g.name(i) << ": ";
                for (int j = 0; j < g.neighbors(i).size(); ++j)
                    cout << (j ? ", " : "") << g.name(g.neighbors(i)[j].to) << " (" << g.neighbors(i)[j].weight << "m)";
                cout << "\n";
            }
        }
        else if (ch == 2 || ch == 3)
        {
            int s = pickBuilding(u, "Start building: ");
            DynArray<int> order = ch == 2 ? g.bfs(s) : g.dfs(s);
            cout << (ch == 2 ? "  BFS (level by level): " : "  DFS (as deep as possible first): ");
            printPath(g, order);
            if (order.size() < g.vertexCount())
                cout << "  Note: " << g.vertexCount() - order.size() << " building(s) are not reachable from here.\n";
        }
        else if (ch == 4)
        {
            int c = g.components();
            cout << "  " << (c == 1 ? "The campus is fully connected." : "The campus has " + to_string(c) + " disconnected parts.") << "\n";
        }
        else if (ch == 5 || ch == 6)
        {
            int a = pickBuilding(u, "From: "), b = pickBuilding(u, "To: ");
            DynArray<int> path;
            int dist = 0;
            bool ok = ch == 5 ? g.shortestPath(a, b, path, dist) : g.fewestHops(a, b, path);
            if (!ok)
            {
                cout << "  No route exists.\n";
                continue;
            }
            cout << "  Route: ";
            printPath(g, path);
            if (ch == 5)
                cout << "  Total distance: " << dist << " m\n";
            else
                cout << "  Buildings passed: " << path.size() << "\n";
        }
        else if (ch == 7)
        {
            DynArray<WeightedEdge> mst;
            int total = g.minimumSpanningTree(mst);
            cout << "  Kruskal MST - connects all buildings with minimum total cable length:\n";
            for (int i = 0; i < mst.size(); ++i)
                cout << "   " << left << setw(16) << g.name(mst[i].u) << "-- " << setw(16) << g.name(mst[i].v) << mst[i].w << " m\n";
            cout << "  Total cable: " << total << " m using " << mst.size() << " of " << g.edgeCount() << " walkways.\n";
            if (!g.isConnected())
                cout << "  (Map is disconnected, so this is a spanning forest.)\n";
        }
        else if (ch == 8)
        {
            string n = readNonEmpty("Building name: ");
            if (u.campus.indexOf(n) >= 0)
                cout << "  That building already exists.\n";
            else
            {
                u.campus.addVertex(n);
                cout << "  Added. Connect it with option 9.\n";
            }
        }
        else if (ch == 9)
        {
            if (g.vertexCount() < 2)
            {
                cout << "  Need at least two buildings.\n";
                continue;
            }
            int a = pickBuilding(u, "First building: "), b = pickBuilding(u, "Second building: ");
            int w = readInt("Distance in metres (1-5000): ", 1, 5000);
            cout << (u.campus.addEdge(a, b, w) ? "  Walkway added.\n" : "  Could not add (same building or already connected).\n");
        }
    }
}

//  13. ADT / memory-safety demonstration
static void adtDemo(University &u)
{
    cout << "\n=== ADT, RULE OF THREE & SAFE POINTERS ===\n";
    DynArray<int> a;
    for (int i = 1; i <= 5; ++i)
        a.push_back(i * 10);
    DynArray<int> b = a; // copy constructor => deep copy
    b[0] = 999;
    cout << "1. Deep copy: a[0]=" << a[0] << ", copy b[0]=" << b[0] << "  (changing the copy leaves the original intact)\n";
    DynArray<int> c;
    c = a; // copy assignment
    c.push_back(7);
    cout << "   Assignment: a has " << a.size() << " items, c has " << c.size() << " items\n";
    a = a; // self-assignment must be harmless
    cout << "   Self-assignment safe: a still has " << a.size() << " items\n";
    try
    {
        cout << "2. Bounds check: reading a[10] ... ";
        cout << a[10] << "\n";
    }
    catch (const out_of_range &e)
    {
        cout << "caught -> " << e.what() << "\n";
    }
    if (!u.students.empty())
    {
        Student s1 = u.students[0];
        Student s2 = s1;
        s2.enrollments.pushBack(Enrollment("DEMO1", 4.0));
        cout << "3. Student copy: original has " << s1.enrollments.size() << " enrollments, copy has " << s2.enrollments.size() << " (separate linked lists)\n";
    }
    SafePtr<Student> sp;
    try
    {
        cout << "4. SafePtr: dereferencing a null SafePtr ... ";
        cout << sp->name << "\n";
    }
    catch (const runtime_error &e)
    {
        cout << "caught -> " << e.what() << "\n";
    }
    if (!u.students.empty())
    {
        SafePtr<Student> snap = SafePtr<Student>::make(u.students[0]);
        SafePtr<Student> snap2 = snap; // deep copy of the pointee
        snap2->name = "Changed";
        cout << "   SafePtr copy: original '" << snap->name << "', copy '" << snap2->name << "' (independent objects)\n";
    }
    JaggedArray<int> j;
    for (int r = 0; r < 4; ++r)
    {
        int row = j.addRow(r + 1, r);
        (void)row;
    }
    cout << "5. Jagged array with row lengths:";
    for (int r = 0; r < j.rowCount(); ++r)
        cout << " " << j.rowLength(r);
    cout << "  (total " << j.totalCells() << " cells)\n";
    try
    {
        j.at(1, 5);
    }
    catch (const out_of_range &e)
    {
        cout << "   Jagged bounds check: " << e.what() << "\n";
    }
}

//  14. Save / load
static void fileMenu(University &u)
{
    while (true)
    {
        cout << "\n=== SAVE / LOAD ===\n 1. Save all records to text files\n 2. Load records from text files\n 3. Reset to sample data\n 0. Back\n";
        int ch = readInt("Choice: ", 0, 3);
        if (ch == 0)
            return;
        if (ch == 1)
        {
            cout << (saveAll(u) ? "  Saved to ums_students.txt, ums_courses.txt, ums_faculty.txt, ums_campus.txt\n" : "  Error: could not write files here.\n");
        }
        else if (ch == 2)
        {
            LoadReport r = loadAll(u);
            if (!r.found)
                cout << "  No saved files found in this folder.\n";
            else
                cout << "  Loaded " << r.students << " students, " << r.courses << " courses, " << r.faculty << " faculty, " << r.buildings << " buildings; " << r.rejected << " line(s) rejected.\n";
        }
        else
        {
            if (askYesNo("Replace all current records with the sample data?"))
            {
                u.seedSampleData();
                cout << "  Sample data restored.\n";
            }
        }
    }
}

//  Main menu
int main()
{
    University u;
    LoadReport r = loadAll(u);
    cout << "_______________________________________________________________\n"
            "   UNIVERSITY MANAGEMENT SYSTEM FAST-NUCES Karachi\n"
            "---------------------------------------------------------------\n";
    if (r.found)
        cout << "Loaded saved records: " << u.students.size() << " students, " << u.courses.size() << " courses, " << u.faculty.size() << " faculty.\n";
    else
    {
        u.seedSampleData();
        cout << "No saved data found loaded sample data (" << u.students.size() << " students, " << u.courses.size() << " courses).\n";
    }

    try
    {
        while (true)
        {
            cout << "\n================ MAIN MENU ================\n"
                    "  1. Student records\n  2. Course records & prerequisites\n  3. Faculty records\n  4. Enrollment, grades & gradebook\n"
                    "  5. Searching (linear / binary / interpolation)\n  6. Sorting (7 algorithms + benchmark)\n  7. Indexing & hashing (BST, AVL, 2-3, B-tree, hash)\n"
                    "  8. Text search (KMP / Rabin-Karp)\n  9. Merit list & scholarships (heap)\n 10. Queue, undo & exam invigilators\n"
                    " 11. Timetable & course order (backtracking, topological sort)\n 12. Campus map (BFS, DFS, shortest path, MST)\n"
                    " 13. ADT / Rule of Three demo\n 14. Save / load / reset data\n  0. Exit\n";
            int ch = readInt("Choice: ", 0, 14);
            switch (ch)
            {
            case 1:
                studentMenu(u);
                break;
            case 2:
                courseMenu(u);
                break;
            case 3:
                facultyMenu(u);
                break;
            case 4:
                enrollmentMenu(u);
                break;
            case 5:
                searchMenu(u);
                break;
            case 6:
                sortMenu(u);
                break;
            case 7:
                indexMenu(u);
                break;
            case 8:
                textSearchMenu(u);
                break;
            case 9:
                scholarshipMenu(u);
                break;
            case 10:
                operationsMenu(u);
                break;
            case 11:
                timetableMenu(u);
                break;
            case 12:
                campusMenu(u);
                break;
            case 13:
                adtDemo(u);
                break;
            case 14:
                fileMenu(u);
                break;
            case 0:
                saveAll(u);
                cout << "Records saved. Goodbye!\n";
                return 0;
            }
        }
    }
    catch (const InputEnded &)
    {
        saveAll(u);
        cout << "\nInput closed. Records saved. Goodbye!\n";
    }
    catch (const exception &e)
    {
        cout << "\nUnexpected error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}