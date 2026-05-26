#include "scopebus.h"

TimeScopeBus::TimeScopeBus(QObject *parent)
    : QObject(parent)
{
    m_pendingFlushTimer.setSingleShot(true);
    m_pendingFlushTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_pendingFlushTimer, &QTimer::timeout,
            this, &TimeScopeBus::onPendingFlushTick);
}

void TimeScopeBus::setPendingThrottleMs(int ms)
{
    m_throttleMs = (ms < 0) ? 0 : ms;
}

bool TimeScopeBus::sameSpan(const TimeSelectionSpan& a, const TimeSelectionSpan& b)
{
    return a.startTime == b.startTime && a.endTime == b.endTime;
}

void TimeScopeBus::publishPending(const TimeSelectionSpan& span, Origin origin,
                                  bool isFrozenSource, QObject* source)
{
    if (!span.startTime.isValid() || !span.endTime.isValid())
        return;

    Snapshot s;
    s.span           = span;
    s.origin         = origin;
    s.phase          = Phase::Pending;
    s.isFrozenSource = isFrozenSource;
    s.source         = source;

    if (m_hasPending && sameSpan(m_pendingSnapshot.span, span)
        && m_pendingSnapshot.isFrozenSource == isFrozenSource) {
        return;
    }
    m_pendingSnapshot = s;
    m_hasPending = true;

    if (!m_pendingFlushTimer.isActive())
        m_pendingFlushTimer.start(m_throttleMs);
}

void TimeScopeBus::publishCommitted(const TimeSelectionSpan& span, Origin origin,
                                    bool isFrozenSource, QObject* source)
{
    if (!span.startTime.isValid() || !span.endTime.isValid())
        return;

    m_hasPending = false;
    m_pendingFlushTimer.stop();

    if (m_hasScope && sameSpan(m_currentScope, span))
        return;

    Snapshot s;
    s.span           = span;
    s.origin         = origin;
    s.phase          = Phase::Committed;
    s.isFrozenSource = isFrozenSource;
    s.source         = source;
    broadcast(std::move(s));
}

void TimeScopeBus::onPendingFlushTick()
{
    if (!m_hasPending) return;

    if (m_hasScope && sameSpan(m_currentScope, m_pendingSnapshot.span)) {
        m_hasPending = false;
        return;
    }
    Snapshot s = m_pendingSnapshot;
    m_hasPending = false;
    broadcast(std::move(s));
}

int TimeScopeBus::subscribe(Subscriber sub)
{
    SubEntry e{ m_nextToken++, std::move(sub) };
    int token = e.token;
    m_subs.push_back(std::move(e));
    return token;
}

void TimeScopeBus::unsubscribe(int token)
{
    for (auto it = m_subs.begin(); it != m_subs.end(); ++it) {
        if (it->token == token) { m_subs.erase(it); return; }
    }
}

void TimeScopeBus::broadcast(Snapshot snap)
{
    if (m_broadcasting) {
        m_deferredBroadcasts.push_back(std::move(snap));
        return;
    }

    m_broadcasting = true;

    snap.generation = ++m_generation;
    m_currentScope  = snap.span;
    m_hasScope      = true;

    auto subsCopy = m_subs;
    for (auto& s : subsCopy) {
        if (s.cb) s.cb(snap);
    }

    m_broadcasting = false;

    if (!m_deferredBroadcasts.empty()) {
        std::vector<Snapshot> queue;
        queue.swap(m_deferredBroadcasts);
        for (auto& s : queue) broadcast(std::move(s));
    }
}
