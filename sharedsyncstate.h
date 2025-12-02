#ifndef SHARED_SYNC_STATE_H
#define SHARED_SYNC_STATE_H

#include "timelineutils.h"
#include <QDateTime>
#include <vector>

// Shared synchronization state for all graph containers
class GraphContainerSyncState
{
public:
    // Constructor
    GraphContainerSyncState()
        : currentInterval(TimeInterval::OneHour), 
          hasInterval(false), 
          hasTimeScope(false), 
          hasCursorTime(false), 
          hasCurrentNavTime(false),
          isGraphContainerInFollowMode(true)
    {
    }

    // Time interval synchronization
    TimeInterval currentInterval;
    bool hasInterval;

    // Time scope synchronization
    TimeSelectionSpan currentTimeScope;
    bool hasTimeScope;

    // Cursor time synchronization
    QDateTime cursorTime;
    bool hasCursorTime;

    // Current navtime synchronization
    QDateTime currentNavTime;
    bool hasCurrentNavTime;

    // Graph Container data follower synchronization
    bool isGraphContainerInFollowMode = true;

    // Time selections synchronization
    std::vector<TimeSelectionSpan> timeSelections;
};


#endif // SHARED_SYNC_STATE_H