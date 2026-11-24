// Storage.h - Simple pipe-delimited text-file persistence (no database server).
//   ums_faculty.txt : id|name|dept
//   ums_courses.txt : code|title|credits|facultyId|prereq1,prereq2|slot
//   ums_students.txt: id|name|section|dept|cgpa|CODE:grade;CODE:grade
//   ums_campus.txt  : V|building      /      E|buildingA|buildingB|metres
// Files are written in the current working directory.
#pragma once
#include <cstdio>
#include <fstream>
#include <string>
#include "University.h"

using namespace std;

namespace ums
{

    struct LoadReport
    {
        bool found = false;
        int students = 0, courses = 0, faculty = 0, buildings = 0, rejected = 0;
    };

    inline string fmtGrade(double g)
    {
        char buf[32];
        snprintf(buf, sizeof buf, "%.2f", g);
        return buf;
    }

    inline bool saveAll(const University &u, const string &prefix = "ums_")
    {
        ofstream f(prefix + "faculty.txt"), c(prefix + "courses.txt"), s(prefix + "students.txt"), m(prefix + "campus.txt");
        if (!f || !c || !s || !m)
            return false;

        for (int i = 0; i < u.faculty.size(); ++i)
            f << u.faculty[i].id << '|' << sanitize(u.faculty[i].name) << '|' << sanitize(u.faculty[i].dept) << '\n';

        for (int i = 0; i < u.courses.size(); ++i)
        {
            const Course &co = u.courses[i];
            c << co.code << '|' << sanitize(co.title) << '|' << co.credits << '|' << co.facultyId << '|';
            for (int p = 0; p < co.prereqs.size(); ++p)
                c << (p ? "," : "") << co.prereqs[p];
            c << '|' << co.slot << '\n';
        }

        for (int i = 0; i < u.students.size(); ++i)
        {
            const Student &st = u.students[i];
            s << st.id << '|' << sanitize(st.name) << '|' << sanitize(st.section) << '|' << sanitize(st.dept) << '|'
              << fmtGrade(st.cgpa) << '|';
            bool first = true;
            for (const SinglyLinkedList<Enrollment>::Node *p = st.enrollments.head(); p; p = p->next)
            {
                s << (first ? "" : ";") << p->value.courseCode << ':' << fmtGrade(p->value.grade);
                first = false;
            }
            s << '\n';
        }

        for (int v = 0; v < u.campus.vertexCount(); ++v)
            m << "V|" << u.campus.name(v) << '\n';
        for (int v = 0; v < u.campus.vertexCount(); ++v)
            for (int i = 0; i < u.campus.neighbors(v).size(); ++i)
            {
                const Edge &e = u.campus.neighbors(v)[i];
                if (v < e.to)
                    m << "E|" << u.campus.name(v) << '|' << u.campus.name(e.to) << '|' << e.weight << '\n';
            }
        return true;
    }

    // Loads whatever files exist. Malformed or duplicate lines are skipped and counted.
    inline LoadReport loadAll(University &u, const string &prefix = "ums_")
    {
        LoadReport rep;
        ifstream sf(prefix + "students.txt");
        if (!sf)
            return rep; // nothing saved yet
        rep.found = true;

        University tmp;
        string line, err;

        ifstream ff(prefix + "faculty.txt");
        while (ff && getline(ff, line))
        {
            if (trim(line).empty())
                continue;
            DynArray<string> p = splitStr(line, '|');
            int id;
            if (p.size() >= 3 && parseIntStrict(p[0], id) && tmp.addFaculty(Faculty(id, p[1], p[2]), err, false))
                ++rep.faculty;
            else
                ++rep.rejected;
        }

        ifstream cf(prefix + "courses.txt");
        while (cf && getline(cf, line))
        {
            if (trim(line).empty())
                continue;
            DynArray<string> p = splitStr(line, '|');
            int cr, fid, slot = -1;
            if (p.size() >= 4 && parseIntStrict(p[2], cr) && parseIntStrict(p[3], fid))
            {
                Course co(p[0], p[1], cr, fid);
                if (p.size() >= 5 && !p[4].empty())
                {
                    DynArray<string> pre = splitStr(p[4], ',');
                    for (int i = 0; i < pre.size(); ++i)
                        co.prereqs.push_back(pre[i]);
                }
                if (p.size() >= 6)
                    parseIntStrict(p[5], slot);
                co.slot = slot;
                if (tmp.addCourse(co, err, false))
                {
                    // addCourse resets nothing, but make sure the stored slot survives
                    tmp.courses[tmp.courses.size() - 1].slot = slot;
                    ++rep.courses;
                    continue;
                }
            }
            ++rep.rejected;
        }

        while (getline(sf, line))
        {
            if (trim(line).empty())
                continue;
            DynArray<string> p = splitStr(line, '|');
            int id;
            double cg;
            if (p.size() >= 5 && parseIntStrict(p[0], id) && parseDoubleStrict(p[4], cg))
            {
                Student st(id, p[1], p[2], p[3], cg);
                if (p.size() >= 6 && !p[5].empty())
                {
                    DynArray<string> items = splitStr(p[5], ';');
                    for (int i = 0; i < items.size(); ++i)
                    {
                        DynArray<string> kv = splitStr(items[i], ':');
                        double g;
                        if (kv.size() == 2 && parseDoubleStrict(kv[1], g))
                            st.enrollments.pushBack(Enrollment(kv[0], g));
                    }
                }
                if (tmp.addStudent(st, err, false))
                {
                    ++rep.students;
                    continue;
                }
            }
            ++rep.rejected;
        }

        ifstream mf(prefix + "campus.txt");
        Graph g(false);
        bool haveCampus = false;
        while (mf && getline(mf, line))
        {
            if (trim(line).empty())
                continue;
            DynArray<string> p = splitStr(line, '|');
            if (p[0] == "V" && p.size() >= 2)
            {
                g.addVertex(p[1]);
                haveCampus = true;
                ++rep.buildings;
            }
            else if (p[0] == "E" && p.size() >= 4)
            {
                int w;
                if (parseIntStrict(p[3], w))
                    g.addEdge(g.indexOf(p[1]), g.indexOf(p[2]), w);
            }
        }
        if (haveCampus)
            tmp.campus = g;
        else
            tmp.seedCampus();

        // commit
        u.clearAll();
        u.faculty = tmp.faculty;
        u.courses = tmp.courses;
        u.students = tmp.students;
        u.campus = tmp.campus;
        u.rebuildIndices();
        u.rebuildInvigilators();
        u.undoStack.clear();
        return rep;
    }

} // namespace ums