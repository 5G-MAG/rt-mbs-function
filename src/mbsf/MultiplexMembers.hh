#ifndef _MBSF_MULTIPLEX_MEMBERS_HH_
#define _MBSF_MULTIPLEX_MEMBERS_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: the members of a multiplexed MB-SMF MBS Session
 ******************************************************************************
 * Copyright: (C)2026 British Broadcasting Corporation
 * License: 5G-MAG Public License v1
 ******************************************************************************/
#include <algorithm>
#include <cstddef>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

/* One MB-SMF MBS Session can carry several MBS Distribution Sessions of an MBS User Data Ingest Session that are
 * flagged as a multiplex, TS 29.580 V18.8.0 table 6.2.6.2.4-1 multiplexedServFlag: "forms part of a set of MBS
 * Distribution Sessions under the same parent MBS User Data Ingest Session with identical or empty set(s) of target
 * service areas and multiplexed onto the same MBS Session". This keeps who the members are, what the MB-SMF said when it
 * created the session, and which of them want it active. It holds no MB-SMF objects, so it can be tested alone. */
class MultiplexMembers {
public:
    using Id = std::pair<std::string, std::string>;

    /* What a member that asks to leave has to do. */
    enum class Leave {
        DeleteSession,  /* the last member: the MB-SMF session is to be released */
        Detached,       /* removed; the others keep the session and this member is told it is gone */
        AlreadyGone     /* it had left before; the others still hold the session, nothing to do */
    };

    /* The session starts with the member it was made for. */
    void reset(const Id &first) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_members.clear();
        m_members.push_back(first);
        m_activity.clear();
    }

    /* Adds a member. Returns false if it was a member already. replay is set to the create result the MB-SMF has given,
     * if it has, which the new member has to be told. */
    bool add(const Id &id, std::optional<int> &replay) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (std::find(m_members.begin(), m_members.end(), id) != m_members.end()) return false;
        m_members.push_back(id);
        replay = m_createResult;
        return true;
    }

    void setCreateResult(int result) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_createResult = result;
    }

    std::vector<Id> members() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_members;
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_members.size();
    }

    /* A member leaves. The last one, or one of a session whose members were never set, gets the session released. */
    Leave leave(const Id &id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = std::find(m_members.begin(), m_members.end(), id);
        if (it != m_members.end()) {
            if (m_members.size() > 1) {
                m_members.erase(it);
                m_activity.erase(id);
                return Leave::Detached;
            }
            return Leave::DeleteSession;
        }
        return m_members.empty() ? Leave::DeleteSession : Leave::AlreadyGone;
    }

    /* A member says whether it wants the session active. Returns what the MB-SMF is told: active while any member is
     * (a session with one member is told what that member says). */
    bool setActive(const Id &id, bool active) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_members.size() <= 1) return active;
        m_activity[id] = active;
        for (const auto &member : m_members) {
            auto it = m_activity.find(member);
            if (it != m_activity.end() && it->second) return true;
        }
        return false;
    }

private:
    mutable std::mutex m_mutex;
    std::vector<Id> m_members;
    std::map<Id, bool> m_activity;
    std::optional<int> m_createResult;
};

#endif /* _MBSF_MULTIPLEX_MEMBERS_HH_ */
