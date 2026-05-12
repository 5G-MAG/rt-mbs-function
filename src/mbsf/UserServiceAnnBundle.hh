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

#include <memory>
#include <list>
#include <condition_variable>
#include <mutex>

#include "common.hh"

MBSF_NAMESPACE_START

class DistributionSessionInfo;
class UserDataIngSession;

class UserServiceAnnBundle {
public:

    UserServiceAnnBundle(std::shared_ptr<UserDataIngSession> user_data_ing_session);
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

    const std::list<std::string> &filesToServe() const { return m_nameOfFilesToServe; };

    void notify() { m_userServiceAnnChange.notify_all(); };
    void wait();
    bool completed() const {return m_done.load();};

    virtual void processEvent(ogs_event_t *event);

protected:
    void startWorker();

private:
    void worker();
    bool writeAnnouncement();
    bool writeToFile(const std::string &abs_dir_path, const std::string &file_name,
                const std::string &content, std::string &err);
    void finish();

    std::shared_ptr<UserDataIngSession> m_userDataIngSession;
    std::list<std::string> m_nameOfFilesToServe;
    std::condition_variable_any m_userServiceAnnChange;
    std::unique_ptr<std::recursive_mutex> m_userServiceAnnMutex;
    std::thread m_userServiceAnnThread;
    std::atomic_bool m_userServiceAnnThreadCancel;
    std::atomic_bool m_userServiceAnnThreadRunning;
    std::atomic_bool m_done;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_USER_SERVICE_ANN_BUNDLE_HH_ */
