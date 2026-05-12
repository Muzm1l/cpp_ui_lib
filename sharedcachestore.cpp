#include "sharedcachestore.h"
#include <algorithm>

SharedCacheStore::SharedCacheStore(size_t maxEntries)
    : m_maxEntries(maxEntries > 0 ? maxEntries : 1)
{
}

const CachedProjection* SharedCacheStore::get(const CacheKey &key) const
{
    auto it = m_entries.find(key);
    if (it == m_entries.end())
        return nullptr;
    auto lruIt = std::find(m_lru.begin(), m_lru.end(), key);
    if (lruIt != m_lru.end()) {
        m_lru.erase(lruIt);
        m_lru.push_back(key);
    }
    return &it->second;
}

void SharedCacheStore::put(const CacheKey &key, CachedProjection data)
{
    m_entries[key] = std::move(data);
    auto lruIt = std::find(m_lru.begin(), m_lru.end(), key);
    if (lruIt != m_lru.end())
        m_lru.erase(lruIt);
    m_lru.push_back(key);
    evictIfNeeded();
}

void SharedCacheStore::invalidateEpoch(quint64 newEpoch)
{
    m_dataEpoch = newEpoch;
}

void SharedCacheStore::bumpDataEpoch()
{
    ++m_dataEpoch;
}

void SharedCacheStore::evictIfNeeded()
{
    while (m_entries.size() > m_maxEntries && !m_lru.empty()) {
        const CacheKey victim = m_lru.front();
        m_lru.pop_front();
        m_entries.erase(victim);
    }
}
