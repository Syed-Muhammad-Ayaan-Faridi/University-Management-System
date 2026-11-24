// Models.h - Core domain entities: Student, Course, Faculty.
// Each entity is built only from our own ADTs (DynArray, SinglyLinkedList) whose
// copy constructors / assignment operators perform deep copies (Rule of Three),
// so copying a Student/Course/Faculty never shares memory between the copies.
#pragma once
#include <cstdio>
#include <string>
#include "ADT.h"
#include "LinkedLists.h"

using namespace std;

namespace ums
{

    struct Enrollment
    {
        string courseCode;
        double grade; // grade points 0.0 - 4.0 ; negative means "not graded yet"
        Enrollment() : grade(-1.0) {}
        Enrollment(const string &c, double g) : courseCode(c), grade(g) {}
        bool graded() const { return grade >= 0.0; }
    };

    class Student
    {
    public:
        int id; // numeric roll number, e.g. 3081  (shown as 25K-3081)
        string name;
        string section;
        string dept;
        double cgpa;
        SinglyLinkedList<Enrollment> enrollments; // course enrollment list

        Student() : id(0), cgpa(0.0) {}
        Student(int id_, const string &n, const string &sec, const string &d, double g)
            : id(id_), name(n), section(sec), dept(d), cgpa(g) {}

        Enrollment *findEnrollment(const string &code)
        {
            return enrollments.find([&](const Enrollment &e)
                                    { return e.courseCode == code; });
        }
        const Enrollment *findEnrollment(const string &code) const
        {
            return enrollments.find([&](const Enrollment &e)
                                    { return e.courseCode == code; });
        }
    };

    class Course
    {
    public:
        string code;
        string title;
        int credits;
        int facultyId;
        DynArray<string> prereqs; // codes of prerequisite courses
        int slot;                 // timetable slot, -1 = not scheduled

        Course() : credits(3), facultyId(0), slot(-1) {}
        Course(const string &c, const string &t, int cr, int fid)
            : code(c), title(t), credits(cr), facultyId(fid), slot(-1) {}

        bool hasPrereq(const string &p) const
        {
            for (int i = 0; i < prereqs.size(); ++i)
                if (prereqs[i] == p)
                    return true;
            return false;
        }
    };

    class Faculty
    {
    public:
        int id;
        string name;
        string dept;
        Faculty() : id(0) {}
        Faculty(int i, const string &n, const string &d) : id(i), name(n), dept(d) {}
    };

    // "25K-3081" style display of a numeric roll number.
    inline string formatRoll(int id)
    {
        char buf[32];
        snprintf(buf, sizeof buf, "25K-%04d", id);
        return buf;
    }

    // Human readable timetable slot: 5 days x 4 periods = 20 slots.
    inline string slotLabel(int s)
    {
        static const char *days[] = {"Mon", "Tue", "Wed", "Thu", "Fri"};
        static const char *times[] = {"09:00", "11:00", "14:00", "16:00"};
        if (s < 0)
            return "Unscheduled";
        if (s < 20)
            return string(days[s / 4]) + " " + times[s % 4];
        return "Extra slot " + to_string(s - 19);
    }

} // namespace ums