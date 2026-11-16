#ifndef _MBSF_REF_COUNTED_SET_HH_
#define _MBSF_REF_COUNTED_SET_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBSF: a set that counts how often an element was added
 ******************************************************************************
 * Copyright: (C)2026 British Broadcasting Corporation
 * License: 5G-MAG Public License v1
 ******************************************************************************/
#include <cstddef>
#include <map>

/* The MBS Session Identifiers in use. An identifier is in use once for each MBS Distribution Session that holds it:
 * the Distribution Sessions of a multiplex (TS 29.580 V18.8.0 table 6.2.6.2.4-1, multiplexedServFlag: "multiplexed onto
 * the same MBS Session") hold the same one, and it is free again only when the last of them has gone. */
template <class T>
class RefCountedSet {
public:
    /* Adds one use. Returns how many uses there are now. */
    std::size_t add(const T &element) { return ++m_counts[element]; }

    /* Removes one use. Returns false if there was none. */
    bool remove(const T &element) {
        auto it = m_counts.find(element);
        if (it == m_counts.end()) return false;
        if (--it->second == 0) m_counts.erase(it);
        return true;
    }

    bool contains(const T &element) const { return m_counts.find(element) != m_counts.end(); }

    std::size_t count(const T &element) const {
        auto it = m_counts.find(element);
        return it == m_counts.end() ? 0 : it->second;
    }

    std::size_t size() const { return m_counts.size(); }

private:
    std::map<T, std::size_t> m_counts;
};

#endif /* _MBSF_REF_COUNTED_SET_HH_ */
