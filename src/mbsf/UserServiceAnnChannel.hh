#ifndef _MBSF_USER_SERVICE_ANN_CHANNEL_HH_
#define _MBSF_USER_SERVICE_ANN_CHANNEL_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: User Service Announcement Channel class
 ******************************************************************************
 * Copyright: (C)2026 British Broadcasting Corporation
 * Author(s): Dev Audsin <dev.audsin@bbc.co.uk>
 * License: 5G-MAG Public License v1
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */

#include <memory>
#include <list>
#include <condition_variable>
#include <mutex>

#include "common.hh"

MBSF_NAMESPACE_START

#define USER_SERVICE_ANN_CHANNEL "USER SERVICE ANNOUNCEMENT CHANNEL"

class DistributionSessionInfo;
class UserDataIngSession;

class UserServiceAnnChannel {
public:
    UserServiceAnnChannel();

    UserServiceAnnChannel(const UserServiceAnnChannel&) = delete;
    UserServiceAnnChannel(UserServiceAnnChannel&&) = delete;

    void abort() {
        m_announcementChannelCancel = true;
        if (m_announcementChannelThread.get_id() != std::this_thread::get_id() && m_announcementChannelThread.joinable()) {
            m_announcementChannelThread.join();
        }

    }

    virtual ~UserServiceAnnChannel() {
	abort();
    };

    UserServiceAnnChannel &operator=(const UserServiceAnnChannel&) = delete;
    UserServiceAnnChannel &operator=(UserServiceAnnChannel&&) = delete;

    UserServiceAnnChannel &addUserDataIngSession(std::weak_ptr<UserDataIngSession> user_data_ing_session);
    const std::shared_ptr<UserDataIngSession> &annChannelUserDataIngSession() const { return m_userServiceAnnChannelDataIngSession;};
    void notify() { m_announcementChannelChange.notify_all(); };
    virtual void processEvent(ogs_event_t *event);

protected:
    void startWorker();

private:
    void populateUserDataIngSession();
    const std::shared_ptr<DistributionSessionInfo > populateDistributionSessionInfo();

    void workerLoop();

    std::list<std::weak_ptr<UserDataIngSession>> m_userDataIngSessions;
    std::shared_ptr<UserDataIngSession> m_userServiceAnnChannelDataIngSession;
    std::condition_variable_any m_announcementChannelChange;
    std::unique_ptr<std::recursive_mutex> m_announcementChannelMutex;
    std::thread m_announcementChannelThread;
    std::atomic_bool m_announcementChannelCancel;
    std::atomic_bool m_announcementChannelRunning;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_USER_SERVICE_ANN_CHANNEL_HH_ */
