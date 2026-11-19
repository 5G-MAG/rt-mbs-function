/* Covers announcedRepairBaseLocator(): the repair base locator announced for a Distribution Session, for an MBS AS of the MBSTF and for another one. */
#include <cstdio>
#include <optional>
#include <string>
#include "ObjectRepairLocator.hh"
using namespace com::fiveg_mag::ref_tools::mbsf;
static int g_pass = 0, g_fail = 0;
static void check(bool ok, const std::string &what) { (ok ? g_pass : g_fail)++; std::printf("%s: %s\n", ok ? "INFO" : "ERROR", (what + (ok ? " passed." : " failed.")).c_str()); }
int main()
{
    using O = std::optional<std::string>;
    check(announcedRepairBaseLocator("http://as:8889", false, "ing1", "KEY", "m1") == O("http://as:8889/ing1/KEY"), "another MBS AS keeps the ingest session and key scheme");
    check(announcedRepairBaseLocator("http://as:8889/", false, "ing1", "KEY", "") == O("http://as:8889/ing1/KEY"), "a trailing slash is not doubled, and no MBSTF identifier is needed");
    check(announcedRepairBaseLocator("http://as:8889", true, "ing1", "KEY", "m1") == O("http://as:8889/m1"), "the MBSTF's MBS AS is found by the identifier of the session in the MBSTF");
    check(announcedRepairBaseLocator("http://as:8889/", true, "ing1", "KEY", "m1") == O("http://as:8889/m1"), "a trailing slash is not doubled for it either");
    check(!announcedRepairBaseLocator("http://as:8889", true, "ing1", "KEY", ""), "no locator is announced when the identifier is not known");
    const O slash("http://repair.example/"), noslash("http://repair.example/live");
    check(announcedRepairBaseLocator("http://as:8889", true, "ing1", "KEY", "m1", slash) == O("http://as:8889/m1/"), "a distribution base URL that ends with a slash gets a repair base URL that does");
    check(announcedRepairBaseLocator("http://as:8889", true, "ing1", "KEY", "m1", noslash) == O("http://as:8889/m1"), "one that does not gets a repair base URL that does not");
    check(announcedRepairBaseLocator("http://as:8889", false, "ing1", "KEY", "", slash) == O("http://as:8889/ing1/KEY/"), "the same for another MBS AS");
    check(announcedRepairBaseLocator("http://as:8889", true, "ing1", "KEY", "m1", std::string()) == O("http://as:8889/m1"), "an empty distribution base URL is none");
    std::printf("%d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
