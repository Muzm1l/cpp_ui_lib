#ifndef SCOPEBUS_H
#define SCOPEBUS_H

#include "timelineutils.h"
#include <QDateTime>
#include <QObject>
#include <QTimer>
#include <functional>
#include <optional>
#include <vector>

/**
 * TimeScopeBus
 * ============
 *
 * Single source of truth for *time-scope* (visible time-window) propagation
 * across all timeline-driven widgets within one logical view (a GraphLayout,
 * an SCWWindow, etc.).
 *
 * Producers publish *intents*:
 *   - publishPending(span, ...)   for live drag updates (coalesced).
 *   - publishCommitted(span, ...) for finalized values (slider release,
 *                                 programmatic apply, interval change, etc.).
 *
 * Consumers subscribe via subscribe() and react to a Snapshot. The bus is the
 * one and only writer of GraphContainerSyncState::currentTimeScope (the layout
 * installs a writer-only subscriber).
 *
 * Guarantees:
 *   - Exactly one outgoing Snapshot per coalesced pending change (default 16ms).
 *   - Exactly one outgoing Snapshot per commit (synchronous).
 *   - Snapshot.generation is monotonically increasing per bus.
 *   - Identical, no-op publishes are dropped.
 *   - Subscribers can drop stale snapshots by comparing generation.
 *   - Re-entrant publishes during a broadcast are deferred until broadcast
 *     completes (no recursion blow-up).
 */
class TimeScopeBus : public QObject
{
    Q_OBJECT
public:
    enum class Origin {
        Local,         ///< User interaction in our own timeline / container.
        Remote,        ///< From another bus or external system.
        Programmatic,  ///< API call (interval change, replay seek, etc.).
        InitialState   ///< First publish after construction / late-attach.
    };

    enum class Phase {
        Pending,    ///< User is still dragging; updates are throttled.
        Committed   ///< Final value (mouse release, programmatic apply, ...).
    };

    struct Snapshot {
        TimeSelectionSpan span;
        Origin            origin          = Origin::Programmatic;
        Phase             phase           = Phase::Committed;
        bool              isFrozenSource  = false; ///< Source was in FROZEN_MODE
        quint64           generation      = 0;     ///< Monotonic per bus
        QObject*          source          = nullptr; ///< For diagnostics only
    };

    using Subscriber = std::function<void(const Snapshot&)>;

    explicit TimeScopeBus(QObject *parent = nullptr);

    void publishPending  (const TimeSelectionSpan& span, Origin origin,
                          bool isFrozenSource, QObject* source);
    void publishCommitted(const TimeSelectionSpan& span, Origin origin,
                          bool isFrozenSource, QObject* source);

    int  subscribe  (Subscriber sub);
    void unsubscribe(int token);

    bool                 hasScope()           const { return m_hasScope; }
    TimeSelectionSpan    currentScope()       const { return m_currentScope; }
    quint64              currentGeneration()  const { return m_generation; }
    void                 setPendingThrottleMs(int ms);

private slots:
    void onPendingFlushTick();

private:
    void broadcast(Snapshot snap);
    static bool sameSpan(const TimeSelectionSpan& a, const TimeSelectionSpan& b);

    TimeSelectionSpan m_currentScope;
    bool              m_hasScope     = false;
    quint64           m_generation   = 0;
    int               m_throttleMs   = 16;

    std::optional<Snapshot> m_pending;
    QTimer                  m_pendingFlushTimer;

    struct SubEntry { int token; Subscriber cb; };
    std::vector<SubEntry>   m_subs;
    int                     m_nextToken = 1;

    bool                    m_broadcasting = false;
    std::vector<Snapshot>   m_deferredBroadcasts;
};

#endif // SCOPEBUS_H
