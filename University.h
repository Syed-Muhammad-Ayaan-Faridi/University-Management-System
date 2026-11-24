// University.h - The application core. Owns all records and wires every data
// structure to the workflow it is best suited for:
//
//   DynArray .............. master record storage (students / courses / faculty)
//   Hash table, AVL, BST,
//   B-tree, 2-3 tree ...... roll-number indexes (roll -> position in `students`)
//   Stack ................. undo history for edits and deletions
//   Queue ................. registration / fee-counter line
//   Circular list ......... rotating exam-invigilator duty
//   Heap (priority queue).. merit list / scholarship ranking by CGPA
//   Singly linked list .... per-student course enrollments (inside Student)
//   Jagged array .......... gradebook / per-student timetable rows
//   Graph ................. course prerequisites (directed) + campus map (undirected)
//   Backtracking .......... clash-free timetable generation
#pragma once
#include <initializer_list>
#include <iostream>
#include <string>
#include "ADT.h"
#include "Graph.h"
#include "HashTable.h"
#include "Heap.h"
#include "LinkedLists.h"
#include "Models.h"
#include "MultiwayTrees.h"
#include "Searching.h"
#include "Sorting.h"
#include "StackQueue.h"
#include "Timetable.h"
#include "Trees.h"
#include "Utils.h"

using namespace std;

namespace ums
{

    enum class ActionType
    {
        AddStudent,
        DeleteStudent,
        UpdateStudent,
        AddCourse,
        DeleteCourse,
        UpdateCourse,
        AddFaculty,
        DeleteFaculty,
        UpdateFaculty
    };

    // One undoable operation. Snapshots are held in SafePtr (deep-copied, null-checked).
    struct Action
    {
        ActionType type;
        string description;
        SafePtr<Student> student;
        SafePtr<Course> course;
        SafePtr<Faculty> fac;
        Action() : type(ActionType::AddStudent) {}
    };

    struct Ticket
    {
        int number;
        int studentId;
        string purpose;
        Ticket() : number(0), studentId(0) {}
    };

    struct GradeCell
    {
        string code;
        double grade;
        GradeCell() : grade(-1.0) {}
    };

    struct Rank
    {
        int index; // position in `students`
        int id;
        double cgpa;
        Rank() : index(0), id(0), cgpa(0.0) {}
    };
    struct HigherCgpa
    { // max-heap order: higher CGPA first, ties broken by lower roll number
        bool operator()(const Rank &a, const Rank &b) const
        {
            if (a.cgpa != b.cgpa)
                return a.cgpa > b.cgpa;
            return a.id < b.id;
        }
    };

    class University
    {
    public:
        // ---- master data --------------------------------------------------------
        DynArray<Student> students;
        DynArray<Course> courses;
        DynArray<Faculty> faculty;

        // ---- roll-number indexes (all map roll -> index into `students`) -------
        AVLTree avl;
        BST bst;
        BTree btree;
        TwoThreeTree t23;
        ChainedHashTable idxHash;

        // ---- workflow structures -----------------------------------------------
        Stack<Action> undoStack;
        Queue<Ticket> feeQueue;
        int nextTicket;
        CircularLinkedList<int> invigilators; // faculty ids
        Graph campus;
        int slotCount;

        University() : nextTicket(1), campus(false), slotCount(20) {}

        // Indexing
        void rebuildIndices()
        {
            avl.clear();
            bst.clear();
            btree.clear();
            t23.clear();
            idxHash.clear();
            for (int i = 0; i < students.size(); ++i)
                indexStudent(students[i].id, i);
        }
        void rebuildInvigilators()
        {
            invigilators.clear();
            for (int i = 0; i < faculty.size(); ++i)
                invigilators.add(faculty[i].id);
        }
        void clearAll()
        {
            students = DynArray<Student>();
            courses = DynArray<Course>();
            faculty = DynArray<Faculty>();
            undoStack.clear();
            feeQueue.clear();
            nextTicket = 1;
            campus = Graph(false);
            rebuildIndices();
            rebuildInvigilators();
        }

        // Lookup helpers
        int findStudentIndex(int id) const
        { // O(1) average via the hash index
            int v;
            return idxHash.find(id, v) ? v : -1;
        }
        int findCourseIndex(const string &code) const
        {
            string c = upperCopy(trim(code));
            return linearFindIf(courses.data(), courses.size(), [&](const Course &x)
                                { return x.code == c; });
        }
        int findFacultyIndex(int id) const
        {
            return linearFindIf(faculty.data(), faculty.size(), [&](const Faculty &x)
                                { return x.id == id; });
        }

        // Students
        bool addStudent(const Student &s, string &err, bool record = true)
        {
            if (s.id <= 0)
            {
                err = "Roll number must be a positive number.";
                return false;
            }
            if (findStudentIndex(s.id) >= 0)
            {
                err = "Roll number " + formatRoll(s.id) + " already exists.";
                return false;
            }
            if (trim(s.name).empty())
            {
                err = "Name cannot be empty.";
                return false;
            }
            if (s.cgpa < 0.0 || s.cgpa > 4.0)
            {
                err = "CGPA must be between 0.00 and 4.00.";
                return false;
            }
            students.push_back(s);
            indexStudent(s.id, students.size() - 1);
            if (record)
                logStudent(ActionType::AddStudent, s, "Added student " + formatRoll(s.id) + " " + s.name);
            return true;
        }
        bool deleteStudent(int id, string &err, bool record = true)
        {
            int i = findStudentIndex(id);
            if (i < 0)
            {
                err = "No student with roll number " + formatRoll(id) + ".";
                return false;
            }
            Student snapshot = students[i];
            students.removeAt(i);
            rebuildIndices(); // positions after i shifted
            if (record)
                logStudent(ActionType::DeleteStudent, snapshot, "Deleted student " + formatRoll(id) + " " + snapshot.name);
            return true;
        }
        bool updateStudent(int id, const string &name, const string &section, const string &dept,
                           double cgpa, string &err)
        {
            int i = findStudentIndex(id);
            if (i < 0)
            {
                err = "No student with roll number " + formatRoll(id) + ".";
                return false;
            }
            if (trim(name).empty())
            {
                err = "Name cannot be empty.";
                return false;
            }
            if (cgpa < 0.0 || cgpa > 4.0)
            {
                err = "CGPA must be between 0.00 and 4.00.";
                return false;
            }
            Student before = students[i];
            students[i].name = name;
            students[i].section = section;
            students[i].dept = dept;
            students[i].cgpa = cgpa;
            logStudent(ActionType::UpdateStudent, before, "Edited student " + formatRoll(id));
            return true;
        }

        // Faculty
        bool addFaculty(const Faculty &f, string &err, bool record = true)
        {
            if (f.id <= 0)
            {
                err = "Faculty id must be positive.";
                return false;
            }
            if (findFacultyIndex(f.id) >= 0)
            {
                err = "Faculty id already exists.";
                return false;
            }
            if (trim(f.name).empty())
            {
                err = "Name cannot be empty.";
                return false;
            }
            faculty.push_back(f);
            invigilators.add(f.id);
            if (record)
            {
                Action a;
                a.type = ActionType::AddFaculty;
                a.description = "Added faculty " + to_string(f.id) + " " + f.name;
                a.fac = SafePtr<Faculty>::make(f);
                undoStack.push(a);
            }
            return true;
        }
        bool updateFaculty(int id, const string &name, const string &dept, string &err)
        {
            int i = findFacultyIndex(id);
            if (i < 0)
            {
                err = "No faculty member with that id.";
                return false;
            }
            if (trim(name).empty())
            {
                err = "Name cannot be empty.";
                return false;
            }
            Action a;
            a.type = ActionType::UpdateFaculty;
            a.description = "Edited faculty " + to_string(id);
            a.fac = SafePtr<Faculty>::make(faculty[i]);
            undoStack.push(a);
            faculty[i].name = name;
            faculty[i].dept = dept;
            return true;
        }
        bool deleteFaculty(int id, string &err, bool record = true)
        {
            int i = findFacultyIndex(id);
            if (i < 0)
            {
                err = "No faculty member with that id.";
                return false;
            }
            for (int c = 0; c < courses.size(); ++c)
                if (courses[c].facultyId == id)
                {
                    err = "Cannot delete: faculty teaches " + courses[c].code + ".";
                    return false;
                }
            Faculty snapshot = faculty[i];
            faculty.removeAt(i);
            invigilators.remove(id);
            if (record)
            {
                Action a;
                a.type = ActionType::DeleteFaculty;
                a.description = "Deleted faculty " + to_string(id) + " " + snapshot.name;
                a.fac = SafePtr<Faculty>::make(snapshot);
                undoStack.push(a);
            }
            return true;
        }

        // Courses and prerequisites
        bool addCourse(Course c, string &err, bool record = true)
        {
            c.code = upperCopy(trim(c.code));
            if (c.code.empty())
            {
                err = "Course code cannot be empty.";
                return false;
            }
            if (findCourseIndex(c.code) >= 0)
            {
                err = "Course " + c.code + " already exists.";
                return false;
            }
            if (trim(c.title).empty())
            {
                err = "Course title cannot be empty.";
                return false;
            }
            if (c.credits < 1 || c.credits > 6)
            {
                err = "Credit hours must be between 1 and 6.";
                return false;
            }
            if (findFacultyIndex(c.facultyId) < 0)
            {
                err = "Faculty id " + to_string(c.facultyId) + " does not exist.";
                return false;
            }
            courses.push_back(c);
            if (record)
                logCourse(ActionType::AddCourse, c, "Added course " + c.code);
            return true;
        }
        bool deleteCourse(const string &code, string &err, bool record = true)
        {
            int ci = findCourseIndex(code);
            if (ci < 0)
            {
                err = "No such course.";
                return false;
            }
            const string c = courses[ci].code;
            for (int i = 0; i < students.size(); ++i)
                if (students[i].findEnrollment(c))
                {
                    err = "Cannot delete: students are enrolled in " + c + ".";
                    return false;
                }
            for (int i = 0; i < courses.size(); ++i)
                if (courses[i].hasPrereq(c))
                {
                    err = "Cannot delete: " + courses[i].code + " lists it as a prerequisite.";
                    return false;
                }
            Course snapshot = courses[ci];
            courses.removeAt(ci);
            if (record)
                logCourse(ActionType::DeleteCourse, snapshot, "Deleted course " + c);
            return true;
        }
        // Adds "pre" as a prerequisite of "code"; rejects edges that would create a cycle.
        bool addPrerequisite(const string &code, const string &pre, string &err)
        {
            int ci = findCourseIndex(code), pi = findCourseIndex(pre);
            if (ci < 0 || pi < 0)
            {
                err = "Both courses must exist.";
                return false;
            }
            if (ci == pi)
            {
                err = "A course cannot be its own prerequisite.";
                return false;
            }
            if (courses[ci].hasPrereq(courses[pi].code))
            {
                err = "That prerequisite is already recorded.";
                return false;
            }
            Course before = courses[ci];
            courses[ci].prereqs.push_back(courses[pi].code);
            DynArray<int> order;
            if (!buildPrereqGraph().topologicalSort(order))
            {
                courses[ci] = before;
                err = "Rejected: this prerequisite would create a circular dependency.";
                return false;
            }
            logCourse(ActionType::UpdateCourse, before, "Added prerequisite " + courses[pi].code + " -> " + courses[ci].code);
            return true;
        }

        // Directed graph: edge prerequisite -> course. Vertex i == courses[i].
        Graph buildPrereqGraph() const
        {
            Graph g(true);
            for (int i = 0; i < courses.size(); ++i)
                g.addVertex(courses[i].code);
            for (int i = 0; i < courses.size(); ++i)
                for (int p = 0; p < courses[i].prereqs.size(); ++p)
                {
                    int pi = g.indexOf(courses[i].prereqs[p]);
                    if (pi >= 0)
                        g.addEdge(pi, i, 1);
                }
            return g;
        }
        // Recursive trace of the full prerequisite chain of a course (indented tree).
        void tracePrereqs(const string &code, int depth, ostream &os) const
        {
            int ci = findCourseIndex(code);
            if (ci < 0 || depth > courses.size())
                return; // depth guard
            os << string(depth * 3, ' ') << (depth ? "+- " : "") << courses[ci].code << "  " << courses[ci].title << "\n";
            for (int i = 0; i < courses[ci].prereqs.size(); ++i)
                tracePrereqs(courses[ci].prereqs[i], depth + 1, os);
        }

        // Enrollment and grades
        bool enroll(int sid, const string &code, string &err)
        {
            int si = findStudentIndex(sid), ci = findCourseIndex(code);
            if (si < 0)
            {
                err = "No student with that roll number.";
                return false;
            }
            if (ci < 0)
            {
                err = "No such course.";
                return false;
            }
            const Course &c = courses[ci];
            Student &s = students[si];
            if (s.findEnrollment(c.code))
            {
                err = "Already enrolled in " + c.code + ".";
                return false;
            }
            for (int p = 0; p < c.prereqs.size(); ++p)
            {
                const Enrollment *e = s.findEnrollment(c.prereqs[p]);
                if (!e || !e->graded() || e->grade < 1.0)
                {
                    err = "Prerequisite not passed: " + c.prereqs[p] + ".";
                    return false;
                }
            }
            Student before = s;
            s.enrollments.pushBack(Enrollment(c.code, -1.0));
            logStudent(ActionType::UpdateStudent, before, "Enrolled " + formatRoll(sid) + " in " + c.code);
            return true;
        }
        bool dropCourse(int sid, const string &code, string &err)
        {
            int si = findStudentIndex(sid);
            if (si < 0)
            {
                err = "No student with that roll number.";
                return false;
            }
            string c = upperCopy(trim(code));
            Enrollment *e = students[si].findEnrollment(c);
            if (!e)
            {
                err = "Student is not enrolled in " + c + ".";
                return false;
            }
            if (e->graded())
            {
                err = "Cannot drop a course that is already graded.";
                return false;
            }
            Student before = students[si];
            students[si].enrollments.removeIf([&](const Enrollment &x){ return x.courseCode == c; });
            logStudent(ActionType::UpdateStudent, before, "Dropped " + c + " for " + formatRoll(sid));
            return true;
        }
        bool setGrade(int sid, const string &code, double gradePoints, string &err)
        {
            int si = findStudentIndex(sid);
            if (si < 0)
            {
                err = "No student with that roll number.";
                return false;
            }
            if (gradePoints < 0.0 || gradePoints > 4.0)
            {
                err = "Grade points must be between 0.0 and 4.0.";
                return false;
            }
            string c = upperCopy(trim(code));
            Enrollment *e = students[si].findEnrollment(c);
            if (!e)
            {
                err = "Student is not enrolled in " + c + ".";
                return false;
            }
            Student before = students[si];
            e->grade = gradePoints;
            recalcCgpa(students[si]);
            logStudent(ActionType::UpdateStudent, before, "Graded " + formatRoll(sid) + " in " + c);
            return true;
        }
        // Credit-weighted GPA over graded courses. Keeps the stored value if nothing is graded yet.
        void recalcCgpa(Student &s) const
        {
            double pts = 0, cr = 0;
            for (const SinglyLinkedList<Enrollment>::Node *p = s.enrollments.head(); p; p = p->next)
            {
                if (!p->value.graded())
                    continue;
                int ci = findCourseIndex(p->value.courseCode);
                if (ci < 0)
                    continue;
                pts += courses[ci].credits * p->value.grade;
                cr += courses[ci].credits;
            }
            if (cr > 0)
                s.cgpa = (double)(int)(pts / cr * 100.0 + 0.5) / 100.0;
        }

        // Jagged gradebook: row r = student r, with one cell per enrolled course.
        JaggedArray<GradeCell> buildGradebook() const
        {
            JaggedArray<GradeCell> gb;
            for (int i = 0; i < students.size(); ++i)
            {
                int row = gb.addRow(students[i].enrollments.size());
                int col = 0;
                for (const SinglyLinkedList<Enrollment>::Node *p = students[i].enrollments.head(); p; p = p->next, ++col)
                {
                    gb.at(row, col).code = p->value.courseCode;
                    gb.at(row, col).grade = p->value.grade;
                }
            }
            return gb;
        }
        // Jagged timetable: row r = student r, cell = slot of each enrolled course (-1 if unscheduled).
        JaggedArray<int> buildStudentSlots() const
        {
            JaggedArray<int> table;
            for (int i = 0; i < students.size(); ++i)
            {
                int row = table.addRow(students[i].enrollments.size(), -1);
                int col = 0;
                for (const SinglyLinkedList<Enrollment>::Node *p = students[i].enrollments.head(); p; p = p->next, ++col)
                {
                    int ci = findCourseIndex(p->value.courseCode);
                    table.at(row, col) = ci >= 0 ? courses[ci].slot : -1;
                }
            }
            return table;
        }

        // Fee / registration queue, invigilators, merit list
        bool joinQueue(int sid, const string &purpose, Ticket &out, string &err)
        {
            if (findStudentIndex(sid) < 0)
            {
                err = "No student with that roll number.";
                return false;
            }
            out.number = nextTicket++;
            out.studentId = sid;
            out.purpose = purpose;
            feeQueue.enqueue(out);
            return true;
        }
        // Next invigilator from the ring, skipping the course's own instructor.
        int nextInvigilator(int excludeFacultyId)
        {
            for (int tries = 0; tries < invigilators.size(); ++tries)
            {
                int id = invigilators.next();
                if (id != excludeFacultyId)
                    return id;
            }
            return -1;
        }
        // Top-k students with CGPA >= minCgpa, extracted from a max-heap (best first).
        DynArray<int> meritList(int k, double minCgpa) const
        {
            DynArray<Rank> all;
            for (int i = 0; i < students.size(); ++i)
            {
                Rank r;
                r.index = i;
                r.id = students[i].id;
                r.cgpa = students[i].cgpa;
                all.push_back(r);
            }
            PriorityQueue<Rank, HigherCgpa> heap;
            heap.build(all.data(), all.size());
            DynArray<int> result;
            Rank top;
            while (result.size() < k && heap.pop(top))
            {
                if (top.cgpa < minCgpa)
                    break; // remaining students are all lower
                result.push_back(top.index);
            }
            return result;
        }

        // Timetable generation (topological order + recursive backtracking)
        bool generateTimetable(TimetableResult &res, DynArray<int> &order, string &err)
        {
            int n = courses.size();
            if (n == 0)
            {
                err = "There are no courses to schedule.";
                return false;
            }
            if (!buildPrereqGraph().topologicalSort(order))
            {
                err = "Prerequisite graph contains a cycle.";
                return false;
            }

            DynArray<DynArray<char>> clash(n, DynArray<char>(n, 0));
            // 1) courses sharing at least one student who is currently taking both
            for (int s = 0; s < students.size(); ++s)
            {
                DynArray<int> mine;
                for (const SinglyLinkedList<Enrollment>::Node *p = students[s].enrollments.head(); p; p = p->next)
                {
                    if (p->value.graded())
                        continue; // completed courses are not being attended this term
                    int ci = findCourseIndex(p->value.courseCode);
                    if (ci >= 0)
                        mine.push_back(ci);
                }
                for (int a = 0; a < mine.size(); ++a)
                    for (int b = a + 1; b < mine.size(); ++b)
                        clash[mine[a]][mine[b]] = clash[mine[b]][mine[a]] = 1;
            }
            // 2) courses taught by the same instructor, 3) a course and its direct prerequisite
            for (int a = 0; a < n; ++a)
            {
                for (int b = a + 1; b < n; ++b)
                    if (courses[a].facultyId == courses[b].facultyId)
                        clash[a][b] = clash[b][a] = 1;
                for (int p = 0; p < courses[a].prereqs.size(); ++p)
                {
                    int pi = findCourseIndex(courses[a].prereqs[p]);
                    if (pi >= 0)
                        clash[a][pi] = clash[pi][a] = 1;
                }
            }
            DynArray<DynArray<int>> conflicts(n);
            for (int a = 0; a < n; ++a)
                for (int b = 0; b < n; ++b)
                    if (clash[a][b])
                        conflicts[a].push_back(b);

            // Search order: most-constrained course first (fail-first heuristic). Sorting a copy of the
            // topological order with a stable merge sort keeps prerequisite order as the tie-break.
            DynArray<int> searchOrder = order;
            mergeSort(searchOrder.data(), searchOrder.size(),[&](int a, int b){ return conflicts[a].size() > conflicts[b].size(); });
            res = solveTimetable(conflicts, searchOrder, slotCount);
            if (res.success)
                for (int i = 0; i < n; ++i)
                    courses[i].slot = res.slotOf[i];
            else
                err = res.aborted ? "Search limit reached; try more time slots." : "No clash-free timetable exists with " + to_string(slotCount) + " slots.";
            return res.success;
        }

        // Undo
        bool undoLast(string &message)
        {
            Action a;
            if (!undoStack.pop(a))
            {
                message = "Nothing to undo.";
                return false;
            }
            string err;
            bool ok = true;
            switch (a.type)
            {
            case ActionType::AddStudent:
                ok = deleteStudent(a.student->id, err, false);
                break;
            case ActionType::DeleteStudent:
                ok = addStudent(*a.student, err, false);
                break;
            case ActionType::UpdateStudent:
            {
                int i = findStudentIndex(a.student->id);
                if (i >= 0)
                    students[i] = *a.student;
                else
                    ok = false;
                break;
            }
            case ActionType::AddCourse:
                ok = deleteCourse(a.course->code, err, false);
                break;
            case ActionType::DeleteCourse:
                ok = addCourse(*a.course, err, false);
                break;
            case ActionType::UpdateCourse:
            {
                int i = findCourseIndex(a.course->code);
                if (i >= 0)
                    courses[i] = *a.course;
                else
                    ok = false;
                break;
            }
            case ActionType::AddFaculty:
                ok = deleteFaculty(a.fac->id, err, false);
                break;
            case ActionType::DeleteFaculty:
                ok = addFaculty(*a.fac, err, false);
                break;
            case ActionType::UpdateFaculty:
            {
                int i = findFacultyIndex(a.fac->id);
                if (i >= 0)
                    faculty[i] = *a.fac;
                else
                    ok = false;
                break;
            }
            }
            message = ok ? "Undone: " + a.description : "Could not undo '" + a.description + "' (" + err + ")";
            return ok;
        }

        // Sample data
        void seedSampleData()
        {
            clearAll();
            string e;
            const char *fac[8][2] = {{"Dr. Hamza Qureshi", "CS"}, {"Dr. Ayesha Siddiqui", "CS"}, {"Dr. Bilal Ahmed", "CS"}, {"Dr. Nida Farooq", "CS"}, {"Dr. Imran Shah", "CS"}, {"Dr. Sana Malik", "AI"}, {"Dr. Kamran Raza", "Math"}, {"Ms. Hira Anwar", "Humanities"}};
            for (int i = 0; i < 8; ++i)
                addFaculty(Faculty(101 + i, fac[i][0], fac[i][1]), e, false);

            auto C = [&](const char *code, const char *title, int cr, int fid, initializer_list<const char *> pre)
            {
                Course c(code, title, cr, fid);
                for (const char *p : pre)
                    c.prereqs.push_back(p);
                addCourse(c, e, false);
            };
            C("CS101", "Programming Fundamentals", 3, 101, {});
            C("CS102", "Discrete Mathematics", 3, 102, {});
            C("CS201", "Object Oriented Programming", 3, 101, {"CS101"});
            C("CS202", "Data Structures", 3, 103, {"CS201", "CS102"});
            C("CS301", "Design and Analysis of Algorithms", 3, 103, {"CS202"});
            C("CS302", "Database Systems", 3, 104, {"CS202"});
            C("CS303", "Operating Systems", 3, 105, {"CS202"});
            C("CS401", "Artificial Intelligence", 3, 106, {"CS301"});
            C("MT101", "Calculus", 3, 107, {});
            C("MT201", "Linear Algebra", 3, 107, {"MT101"});
            C("HU101", "Communication Skills", 2, 108, {});

            const char *names[22] = {"Ali Raza", "Sara Khan", "Hamza Tariq", "Ayesha Noor", "Bilal Hussain", "Fatima Zahra",
                                     "Usman Ghani", "Maria Saleem", "Zain Abbas", "Hina Javed", "Omar Farooq", "Laiba Ashraf",
                                     "Danish Malik", "Noor Fatima", "Taha Siddiqui", "Iqra Rauf", "Saad Mehmood", "Mahnoor Aslam",
                                     "Rayyan Shah", "Zoya Hashmi", "Faizan Ahmed", "Amna Kamal"};
            const char *graded[9] = {"CS101", "MT101", "HU101", "CS102", "CS201", "CS202", "MT201", "CS301", "CS302"};
            const int gradedCount[5] = {0, 3, 5, 7, 9};
            const char *current[5][3] = {{"CS101", "MT101", "HU101"}, {"CS102", "CS201", nullptr}, {"CS202", "MT201", nullptr}, {"CS301", "CS302", nullptr}, {"CS303", "CS401", nullptr}};
            const char *sections[3] = {"A", "B", "C"};
            const char *depts[3] = {"CS", "SE", "AI"};
            const double gp[8] = {1.7, 2.0, 2.3, 2.7, 3.0, 3.3, 3.7, 4.0};

            for (int i = 0; i < 24; ++i)
            {
                int id = i < 22 ? 3002 + i * 4 : (i == 22 ? 3081 : 3103);
                string nm = i < 22 ? names[i] : (i == 22 ? "Sarim Sadiq" : "Ayaan Faridi");
                int stage = i == 22 ? 3 : (i == 23 ? 2 : i % 5);
                Student s(id, nm, sections[i % 3], depts[(i / 3) % 3], 2.5 + ((id * 7) % 15) / 10.0);
                for (int g = 0; g < gradedCount[stage]; ++g)
                {
                    unsigned seed = (unsigned)id * 2654435761u + (unsigned)g * 40503u;
                    s.enrollments.pushBack(Enrollment(graded[g], gp[(seed >> 8) % 8]));
                }
                for (int c = 0; c < 3 && current[stage][c]; ++c)
                    s.enrollments.pushBack(Enrollment(current[stage][c], -1.0));
                students.push_back(s);
            }
            for (int i = 0; i < students.size(); ++i)
                recalcCgpa(students[i]);
            rebuildIndices();
            seedCampus();
            undoStack.clear();
        }

        void seedCampus()
        {
            campus = Graph(false);
            const char *b[] = {"Main Gate", "Admin Block", "Library", "CS Department", "EE Department", "Cafeteria",
                               "Auditorium", "Sports Complex", "Hostel", "Mosque", "Labs Block"};
            for (const char *n : b)
                campus.addVertex(n);
            struct E
            {
                const char *a;
                const char *b;
                int w;
            };
            const E edges[] = {{"Main Gate", "Admin Block", 120}, {"Main Gate", "Cafeteria", 200}, {"Admin Block", "Library", 90}, {"Admin Block", "Auditorium", 150}, {"Library", "CS Department", 70}, {"Library", "Cafeteria", 110}, {"CS Department", "Labs Block", 60}, {"CS Department", "EE Department", 100}, {"EE Department", "Labs Block", 80}, {"Cafeteria", "Sports Complex", 180}, {"Cafeteria", "Mosque", 140}, {"Auditorium", "Mosque", 90}, {"Sports Complex", "Hostel", 130}, {"Hostel", "Mosque", 160}, {"Labs Block", "Hostel", 250}};
            for (const E &e : edges)
                campus.addEdge(campus.indexOf(e.a), campus.indexOf(e.b), e.w);
        }

    private:
        void indexStudent(int id, int index)
        {
            avl.insert(id, index);
            bst.insert(id, index);
            btree.insert(id, index);
            t23.insert(id, index);
            idxHash.insert(id, index);
        }
        void logStudent(ActionType t, const Student &snap, const string &desc)
        {
            Action a;
            a.type = t;
            a.description = desc;
            a.student = SafePtr<Student>::make(snap);
            undoStack.push(a);
        }
        void logCourse(ActionType t, const Course &snap, const string &desc)
        {
            Action a;
            a.type = t;
            a.description = desc;
            a.course = SafePtr<Course>::make(snap);
            undoStack.push(a);
        }
    };

} // namespace ums