#ifndef SCOPECOALESCER_H
#define SCOPECOALESCER_H

#include "rendercommands.h"
#include <optional>

class ScopeCoalescer {
    QDateTime m_pendingMin;
    QDateTime m_pendingMax;
    bool m_pending = false;

public:
    void post(const QDateTime &min, const QDateTime &max)
    {
        m_pendingMin = min;
        m_pendingMax = max;
        m_pending = true;
    }

    std::optional<ScopeChange> flush()
    {
        if (!m_pending)
            return std::nullopt;
        m_pending = false;
        return ScopeChange{m_pendingMin, m_pendingMax};
    }

    void clear() { m_pending = false; }
};

#endif
