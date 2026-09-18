#ifndef _MBSF_USER_SERVICE_ANN_BUNDLE_HH_
#define _MBSF_USER_SERVICE_ANN_BUNDLE_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: User Service Announcement Bundle class
 ******************************************************************************
 * Copyright: (C)2026 British Broadcasting Corporation
 * Author(s): Dev Audsin <dev.audsin@bbc.co.uk>
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include <optional>
#include <memory>
#include <list>
#include <condition_variable>
#include <mutex>

#include "common.hh"
#include "UserDataIngSession.hh"

MBSF_NAMESPACE_START

class DistributionSessionInfo;

class UserServiceAnnBundle {
public:

    UserServiceAnnBundle(const std::shared_ptr<UserDataIngSession> &user_data_ing_session);

    /** The SDP bandwidth value, in bits per second, for a session paced at @p content_bit_rate.
     *
     * TS 26.346 V18.2.0 clause 7.3.2.10 fixes what the value counts:
     * “The size of the packet shall be the complete packet, i.e. IP, UDP and FLUTE headers, and the data payload.”
     * The rate the MBSF holds paces the ALC bytes, so each packet adds a transport header that the
     * bandwidth line still has to account for. With a packet carrying (mtu - transport header) bytes of
     * ALC, the wire rate is content_bit_rate * mtu / (mtu - transport header).
     *
     * A named rule rather than inline arithmetic so it can be tested without building a session
     * description, which needs an ingest session and a live context.
     *
     * \param content_bit_rate The provisioned rate, in bits per second.
     * \param mtu              The link MTU in bytes, unset when the operator has not configured one.
     * \param ipv6             True when the session's connection address is IPv6.
     * \return the bandwidth to write, in bits per second; unchanged when no usable MTU is given.
     */
    /** Which of the FLUTE and FEC session description attributes a Distribution Session announces.
     *
     * Named and returned together rather than decided at four emission sites, so the mapping can be
     * read and tested in one place. Every one of these was emitted unconditionally before, which is
     * what review on 5G-MAG/rt-mbs-function#52 reported.
     */
    struct AnnouncedAttributes {
        bool fluteTsi;             //!< a=flute-tsi
        bool fec;                  //!< a=FEC, media level
        bool fecDeclaration;       //!< a=FEC-declaration
        bool fecRedundancyLevel;   //!< a=FEC-redundancy-level
    };

    /** Decide the attribute set from the distribution method and whether FEC is provisioned.
     *
     * \param object_distribution True for the Object Distribution Method, which runs FLUTE.
     * \param has_fec             True when the session carries a FEC configuration.
     *
     * The TSI identifies a FLUTE session, so it belongs only to one. TS 26.346 V18.2.0 clause
     * 7.3.2.4: "There shall be exactly one occurrence of this descriptor in a complete FLUTE SDP
     * session description and it shall appear at session level."
     *
     * The FEC declaration is optional and its absence is meaningful. Clause 7.3.2.8: "If this
     * attribute is not used, and no other FEC-OTI information is signalled to the UE by other
     * means, the UE may assume that support for FEC id 0 is sufficient capability to enter the
     * session." a=FEC only references a declaration, so it cannot stand without one: the same
     * clause calls it "a short hand to reference one of one or more FEC-declarations".
     *
     * The FEC attributes do not depend on the distribution method: a Packet Distribution Session
     * may carry FEC, and clause 7.3.2.8's attributes are not scoped to download delivery.
     */
    static AnnouncedAttributes announcedAttributes(bool object_distribution, bool has_fec) {
        return AnnouncedAttributes{
            .fluteTsi = object_distribution,
            .fec = has_fec,
            .fecDeclaration = has_fec,
            .fecRedundancyLevel = has_fec
        };
    };

    static uint64_t sdpBandwidthBitRate(uint64_t content_bit_rate, const std::optional<size_t> &mtu, bool ipv6) {
        if (!mtu) return content_bit_rate;
        /* RFC 768 makes the UDP header 8 octets, and RFC 8200 gives 20 octets for a minimum-length
           IPv4 header and 20 more than that for a minimum-length IPv6 one. */
        const uint64_t transport_header = (ipv6 ? 40 : 20) + 8;
        if (*mtu <= transport_header) return content_bit_rate;
        return static_cast<uint64_t>(static_cast<double>(content_bit_rate) * *mtu / (*mtu - transport_header));
    };
    UserServiceAnnBundle() = delete;
    UserServiceAnnBundle(const UserServiceAnnBundle &) = delete;
    UserServiceAnnBundle(UserServiceAnnBundle &&) = delete;

    void abort() {
        m_userServiceAnnThreadCancel = true;
        if (m_userServiceAnnThread.get_id() != std::this_thread::get_id() && m_userServiceAnnThread.joinable()) {
            m_userServiceAnnThread.join();
        }

    }

    void stop() {
        {
            std::lock_guard<std::recursive_mutex> lock(*m_userServiceAnnMutex);
            m_userServiceAnnThreadCancel = true;
        }
        if (m_userServiceAnnThread.get_id() != std::this_thread::get_id() && m_userServiceAnnThread.joinable()) {
            m_userServiceAnnThread.join();
        }

    }


    virtual ~UserServiceAnnBundle() {
        abort();
    };

    UserServiceAnnBundle &operator=(const UserServiceAnnBundle &) = delete;
    UserServiceAnnBundle &operator=(UserServiceAnnBundle &&) = delete;

    UserServiceAnnBundle &addToServingFiles(const std::string &file_name) { m_nameOfFilesToServe.push_back(file_name); return *this; };
    UserServiceAnnBundle &removeFromServingFiles(const std::string &file_name) { m_nameOfFilesToServe.remove(file_name); return *this; };
    UserServiceAnnBundle &clearServingFiles() { m_nameOfFilesToServe.clear(); return *this; };

    const std::list<std::string> &filesToServe() const { return m_nameOfFilesToServe; };

    void notify() { m_userServiceAnnChange.notify_all(); };
    void wait();
    bool completed() const {return m_done.load();};

    void rebuildBundle() { startWorker(); };

    virtual void processEvent(ogs_event_t *event);

protected:
    void startWorker();

private:
    void worker();
    bool writeAnnouncement();
    bool writeServiceDescriptionProtocolDoc(const std::shared_ptr<UserDataIngSession::ContextData> &dist_session_ctx);
    bool writeToFile(const std::string &abs_dir_path, const std::string &file_name,
                const std::string &content, std::string &err);
    void finish();

    std::weak_ptr<UserDataIngSession> m_userDataIngSession;
    std::list<std::string> m_nameOfFilesToServe;
    std::condition_variable_any m_userServiceAnnChange;
    std::unique_ptr<std::recursive_mutex> m_userServiceAnnMutex;
    std::thread m_userServiceAnnThread;
    std::atomic_bool m_userServiceAnnThreadCancel;
    std::atomic_bool m_userServiceAnnThreadRunning;
    std::atomic_bool m_rebuild;
    std::atomic_bool m_done;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_USER_SERVICE_ANN_BUNDLE_HH_ */
