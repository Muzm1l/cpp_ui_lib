#ifndef SHAREDCACHESTORE_H
#define SHAREDCACHESTORE_H

#include "circularbuffer.h"
#include "graphtype.h"
#include <QDateTime>
#include <QString>
#include <cstddef>
#include <deque>
#include <map>
#include <optional>
#include <utility>
#include <vector>

struct CacheKey {
    GraphType type = GraphType::BDW;
    qint64 scopeMinMs = 0;
    qint64 scopeMaxMs = 0;
    quint64 dataEpoch = 0;

    bool operator==(const CacheKey &o) const
    {
        return type == o.type && scopeMinMs == o.scopeMinMs && scopeMaxMs == o.scopeMaxMs
            && dataEpoch == o.dataEpoch;
    }

    bool operator<(const CacheKey &o) const
    {
        if (type != o.type)
            return type < o.type;
        if (scopeMinMs != o.scopeMinMs)
            return scopeMinMs < o.scopeMinMs;
        if (scopeMaxMs != o.scopeMaxMs)
            return scopeMaxMs < o.scopeMaxMs;
        return dataEpoch < o.dataEpoch;
    }
};

struct CachedProjection {
    std::map<QString, CircularBuffer<std::pair<float, qint64>>> visibleData;
    std::map<QString, std::pair<qint64, qint64>> timeRangeEpoch;
    std::map<QString, size_t> lastProcessedIndex;
    std::map<QString, size_t> cachedDataSize;
};

class SharedCacheStore {
public:
    explicit SharedCacheStore(size_t maxEntries = 64);

    std::optional<CachedProjection> get(const CacheKey &key) const;
    void put(const CacheKey &key, CachedProjection data);

    quint64 currentEpoch() const { return m_dataEpoch; }
    void invalidateEpoch(quint64 newEpoch);
    void bumpDataEpoch();

private:
    void evictIfNeeded();

    mutable std::map<CacheKey, CachedProjection> m_entries;
    mutable std::deque<CacheKey> m_lru;
    size_t m_maxEntries;
    quint64 m_dataEpoch = 1;
};

#endif
