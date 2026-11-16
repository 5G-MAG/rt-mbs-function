/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: reference counted set unit test
 ******************************************************************************
 * Copyright: (C)2026 British Broadcasting Corporation
 * License: 5G-MAG Public License v1
 ******************************************************************************/

/* Covers RefCountedSet, which holds the MBS Session Identifiers in use: the Distribution Sessions of a multiplex
 * (TS 29.580 V18.8.0 table 6.2.6.2.4-1, multiplexedServFlag) hold the same one, and it is free only when the last of
 * them has released it. */

#include "RefCountedSet.hh"

#include <cstdio>
#include <string>

static int g_pass = 0;
static int g_fail = 0;

static void check(bool ok, const char *what)
{
    if (ok) { g_pass++; } else { g_fail++; std::printf("FAIL: %s\n", what); }
}

int main()
{
    RefCountedSet<std::string> ids;
    check(!ids.contains("tmgi-1") && ids.count("tmgi-1") == 0 && ids.size() == 0, "an empty set holds nothing");
    check(!ids.remove("tmgi-1"), "removing what is not there reports it");

    check(ids.add("tmgi-1") == 1 && ids.contains("tmgi-1"), "the first use");
    check(ids.add("tmgi-1") == 2 && ids.count("tmgi-1") == 2, "a second use of the same identifier (a multiplex member)");
    check(ids.size() == 1, "one identifier, in use twice");
    check(ids.add("tmgi-2") == 1 && ids.size() == 2, "another identifier");

    check(ids.remove("tmgi-1") && ids.contains("tmgi-1") && ids.count("tmgi-1") == 1,
          "the first member releasing it leaves it in use");
    check(ids.remove("tmgi-1") && !ids.contains("tmgi-1"), "the last member releasing it frees it");
    check(!ids.remove("tmgi-1"), "releasing it once more reports that it was free");
    check(ids.contains("tmgi-2") && ids.size() == 1, "the other identifier is untouched");

    check(ids.add("tmgi-1") == 1, "a freed identifier can be used again, from a count of one");

    std::printf("%d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
