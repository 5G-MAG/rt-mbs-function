#ifndef MBSF_SERVICE_ANNOUNCEMENT_MEDIA_TYPES_HH
#define MBSF_SERVICE_ANNOUNCEMENT_MEDIA_TYPES_HH
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: service announcement media types
 ******************************************************************************
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

/* The User Service Descriptions media type and the parameters TS 26.517 annex A.2.1 requires with
   it.

   The two announcement documents carry different version values, and that is the specification's
   own wording rather than an oversight here: annex A.2.1 states "Rel18" for these descriptions,
   while the object manifest's own annex states "Rel17", its schema not having been revised.

   The profile is the one clause 12.4 names for the Baseline MBS Distribution Session Profile,
   spelled with 17 in its third field in the Release 18 document. */
#define USER_SERVICE_DESCRIPTIONS_MEDIA_TYPE "application/3gpp-mbs-user-service-descriptions+json"
#define BASELINE_DISTRIBUTION_SESSION_PROFILE "urn:3GPP:26517:17:baseline"
#define USER_SERVICE_DESCRIPTIONS_MIME_TYPE \
    USER_SERVICE_DESCRIPTIONS_MEDIA_TYPE ";profiles=\"" BASELINE_DISTRIBUTION_SESSION_PROFILE "\";version=\"Rel18\""

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* MBSF_SERVICE_ANNOUNCEMENT_MEDIA_TYPES_HH */
