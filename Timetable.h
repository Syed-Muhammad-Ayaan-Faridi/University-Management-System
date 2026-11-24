// Timetable.h - Recursive backtracking time-slot assignment.
// Courses are placed one by one in the order supplied by the caller. For each
// course every slot is tried; a slot is rejected if a conflicting course already
// holds it. When no slot works the algorithm BACKTRACKS and moves the previous
// course to its next slot.
#pragma once
#include "ADT.h"

using namespace std;

namespace ums
{

    struct TimetableResult
    {
        bool success = false;
        bool aborted = false; // node limit reached before an answer was found
        DynArray<int> slotOf; // slotOf[course] = assigned slot (or -1)
        int nodes = 0;        // (course, slot) placements examined
        int backtracks = 0;   // assignments undone
        int slotsUsed = 0;    // distinct slots in the final schedule
    };

    namespace detail
    {

        struct TtContext
        {
            const DynArray<DynArray<int>> *conflicts;
            const DynArray<int> *order;
            int slots;
            int nodeLimit;
            TimetableResult *res;
        };

        // `used` = number of distinct slots opened so far (slots 0..used-1). Symmetry breaking:
        // a course may reuse an opened slot or open exactly the next one; opening any other
        // empty slot would only produce a relabelled copy of a schedule already explored.
        inline bool ttPlace(TtContext &c, int pos, int used)
        {
            TimetableResult &r = *c.res;
            if (pos == c.order->size())
                return true; // every course placed
            int course = (*c.order)[pos];
            const DynArray<int> &nb = (*c.conflicts)[course];
            const int limit = used + 1 < c.slots ? used + 1 : c.slots;
            for (int s = 0; s < limit; ++s)
            {
                if (++r.nodes > c.nodeLimit)
                {
                    r.aborted = true;
                    return false;
                }
                bool clash = false;
                for (int i = 0; i < nb.size(); ++i)
                    if (r.slotOf[nb[i]] == s)
                    {
                        clash = true;
                        break;
                    }
                if (clash)
                    continue;
                r.slotOf[course] = s; // choose
                if (ttPlace(c, pos + 1, s == used ? used + 1 : used))
                    return true;       // explore
                r.slotOf[course] = -1; // un-choose (backtrack)
                ++r.backtracks;
                if (r.aborted)
                    return false;
            }
            return false;
        }

    } // namespace detail

    // conflicts[i] lists the course indexes that may not share a slot with course i.
    inline TimetableResult solveTimetable(const DynArray<DynArray<int>> &conflicts, const DynArray<int> &order,
                                          int slots, int nodeLimit = 3000000)
    {
        TimetableResult res;
        res.slotOf = DynArray<int>(conflicts.size(), -1);
        detail::TtContext ctx{&conflicts, &order, slots, nodeLimit, &res};
        res.success = detail::ttPlace(ctx, 0, 0);
        if (res.success)
        {
            DynArray<char> used(slots > 0 ? slots : 1, 0);
            for (int i = 0; i < res.slotOf.size(); ++i)
                if (res.slotOf[i] >= 0 && !used[res.slotOf[i]])
                {
                    used[res.slotOf[i]] = 1;
                    ++res.slotsUsed;
                }
        }
        else
        {
            res.slotOf = DynArray<int>(conflicts.size(), -1);
        }
        return res;
    }

} // namespace ums