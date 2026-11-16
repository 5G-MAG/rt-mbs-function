/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: multiplex members unit test
 ******************************************************************************
 * Copyright: (C)2026 British Broadcasting Corporation
 * License: 5G-MAG Public License v1
 ******************************************************************************/

/* Covers MultiplexMembers, which keeps who shares one MB-SMF MBS Session when several MBS Distribution Sessions of an
 * Ingest Session are flagged as a multiplex (TS 29.580 V18.8.0 table 6.2.6.2.4-1, multiplexedServFlag): that a member
 * added later is told the create result the MB-SMF gave earlier, that the session is released only when the last member
 * leaves, that leaving twice does no harm, and that the session is active while any member wants it so.
 */

#include "MultiplexMembers.hh"

#include <cstdio>

static int g_pass = 0;
static int g_fail = 0;

static void check(bool ok, const char *what)
{
    if (ok) { g_pass++; } else { g_fail++; std::printf("FAIL: %s\n", what); }
}

using Id = MultiplexMembers::Id;
using Leave = MultiplexMembers::Leave;

int main()
{
    const Id a{"ing", "AP_MBS_SESSION_1"};
    const Id b{"ing", "AP_MBS_SESSION_2"};
    const Id c{"ing", "AP_MBS_SESSION_3"};

    {   /* membership */
        MultiplexMembers m;
        m.reset(a);
        check(m.size() == 1 && m.members().front() == a, "a session starts with the member it was made for");
        std::optional<int> replay;
        check(m.add(b, replay), "a new member is added");
        check(!m.add(b, replay), "the same member is not added twice");
        check(m.size() == 2, "two members");
        check(m.members().size() == 2 && m.members()[0] == a && m.members()[1] == b, "members are kept in the order added");
        m.reset(c);
        check(m.size() == 1 && m.members().front() == c, "reset starts again with the one member");
    }
    {   /* the create result is replayed to a member added after it */
        MultiplexMembers m;
        m.reset(a);
        std::optional<int> replay;
        check(m.add(b, replay) && !replay, "a member added before the MB-SMF answered is told nothing yet");
        m.setCreateResult(0);
        replay.reset();
        check(m.add(c, replay) && replay && *replay == 0, "a member added after the answer is told the answer");
        m.setCreateResult(-1);
        replay.reset();
        MultiplexMembers n;
        n.reset(a);
        n.setCreateResult(-1);
        check(n.add(b, replay) && replay && *replay == -1, "a failure is told as well");
    }
    {   /* leaving */
        MultiplexMembers m;
        m.reset(a);
        std::optional<int> replay;
        m.add(b, replay);
        m.add(c, replay);
        check(m.leave(b) == Leave::Detached, "a member leaves while others remain: detached");
        check(m.size() == 2, "two remain");
        check(m.leave(b) == Leave::AlreadyGone, "the same member leaving again is already gone, the session is kept");
        check(m.size() == 2, "still two");
        check(m.leave(a) == Leave::Detached, "the first member can leave before the others");
        check(m.leave(c) == Leave::DeleteSession, "the last member leaving releases the session");
        check(m.leave(c) == Leave::DeleteSession, "and again, which is harmless: the release itself happens once");
        MultiplexMembers none;
        check(none.leave(a) == Leave::DeleteSession, "a session whose members were never set is released");
        MultiplexMembers single;
        single.reset(a);
        check(single.leave(b) == Leave::AlreadyGone, "a stranger leaving a one member session is not a member: nothing to do");
        check(single.size() == 1, "the member of that session is still there");
    }
    {   /* activity */
        MultiplexMembers m;
        m.reset(a);
        check(m.setActive(a, true) && !m.setActive(a, false), "one member: told what that member says");
        std::optional<int> replay;
        m.add(b, replay);
        check(m.setActive(a, true), "a wants it active: active");
        check(m.setActive(b, false), "b does not, a does: still active");
        check(!m.setActive(a, false), "neither does: inactive");
        check(m.setActive(b, true), "b wants it: active");
        check(m.leave(b) == Leave::Detached, "b leaves");
        check(!m.setActive(a, false), "one member left: what a says, inactive");
    }

    std::printf("%d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
