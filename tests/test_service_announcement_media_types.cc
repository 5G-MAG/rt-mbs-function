/* The media type the MBSF gives a User Service Descriptions document, TS 26.517 V19.2.0 annex A.2.1: "Documents following this schema
 * shall be identified with the MIME media type application/3gpp-mbs-user-service-descriptions+json ... including the parameters: The
 * profiles parameter ... shall include a fully-qualified term identifier from the controlled vocabulary specified in clause C.2. The
 * version parameter (see clause E.2.3) shall contain the value "Rel19" to indicate conformance with this version of the present document."
 */

#include "ServiceAnnouncementMediaTypes.hh"

#include <cstdio>
#include <cstring>

static int g_pass = 0;
static int g_fail = 0;

static void check(bool ok, const char *what)
{
    if (ok) { g_pass++; } else { g_fail++; std::printf("FAIL: %s\n", what); }
}

int main()
{
    check(std::strcmp(USER_SERVICE_DESCRIPTIONS_MIME_TYPE,
                      "application/3gpp-mbs-user-service-descriptions+json;profiles=\"urn:3GPP:26517:17:baseline\";version=\"Rel19\"") == 0,
          "the User Service Descriptions media type carries the baseline profile and version Rel19");
    check(std::strstr(USER_SERVICE_DESCRIPTIONS_MIME_TYPE, "Rel18") == nullptr, "and no longer Rel18");
    std::printf("%d checks, %d failed\n", g_pass + g_fail, g_fail);
    return g_fail == 0 ? 0 : 1;
}
