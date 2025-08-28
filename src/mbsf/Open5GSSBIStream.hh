#ifndef _MBS_TF_OPEN5GS_SBI_STREAM_HH_
#define _MBS_TF_OPEN5GS_SBI_STREAM_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Traffic Function: Open5GS SBI Stream interface
 ******************************************************************************
 * Copyright: (C)2024 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 * License: 5G-MAG Public License v1
 *
 * Licensed under the License terms and conditions for use, reproduction, and
 * distribution of 5G-MAG software (the “License”).  You may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 * https://www.5g-mag.com/reference-tools.  Unless required by applicable law or
 * agreed to in writing, software distributed under the License is distributed on
 * an “AS IS” BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied.
 *
 * See the License for the specific language governing permissions and limitations
 * under the License.
 */

#include "ogs-proto.h"
#include "ogs-sbi.h"

#include <memory>

#include "common.hh"

#include "Open5GSSBIServer.hh"

MBSF_NAMESPACE_START

class Open5GSSBIStream {
public:
    Open5GSSBIStream(ogs_sbi_stream_t *stream) :m_stream(stream) {};
    Open5GSSBIStream(ogs_pool_id_t stream_id)
        :m_stream(reinterpret_cast<ogs_sbi_stream_t*>(ogs_sbi_stream_find_by_id(stream_id)))
        {};
    Open5GSSBIStream() = delete;
    Open5GSSBIStream(Open5GSSBIStream &&other) = delete;
    Open5GSSBIStream(const Open5GSSBIStream &other) = delete;
    Open5GSSBIStream &operator=(Open5GSSBIStream &&other) = delete;
    Open5GSSBIStream &operator=(const Open5GSSBIStream &other) = delete;
    virtual ~Open5GSSBIStream() {};

    ogs_sbi_stream_t *ogsSBIStream() { return m_stream; };
    const ogs_sbi_stream_t *ogsSBIStream() const { return m_stream; };

    operator bool() const { return !!m_stream; };

    Open5GSSBIServer server() const { return Open5GSSBIServer(m_stream?ogs_sbi_server_from_stream(m_stream):nullptr); };

private:
    ogs_sbi_stream_t *m_stream;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBS_TF_OPEN5GS_SBI_STREAM_HH_ */
