#ifndef SHARED_SYNC_STATE_H
#define SHARED_SYNC_STATE_H

#include "timelineutils.h"
#include <QDateTime>
#include <QUuid>
#include <vector>

/**
 * @brief Data structure for synchronized BTW markers
 * 
 * Contains all the data needed to recreate a BTW marker across
 * different graph containers. Named BTWSyncMarkerData to avoid
 * conflict with BTWSyncMarkerData in waterfalldata.h
 */
struct BTWSyncMarkerData
{
    QUuid id;               ///< Unique identifier for the marker
    QDateTime timestamp;    ///< Timestamp position of the marker
    qreal rangeValue;       ///< Range value (X-axis position)
    qreal bearingRate;      ///< Bearing rate value (rotation / 10)
    bool isDeleted;         ///< Flag to mark deleted markers
    
    BTWSyncMarkerData() : rangeValue(0.0), bearingRate(0.0), isDeleted(false) {}
    
    BTWSyncMarkerData(const QDateTime &ts, qreal range, qreal rate)
        : id(QUuid::createUuid())
        , timestamp(ts)
        , rangeValue(range)
        , bearingRate(rate)
        , isDeleted(false)
    {}
    
    bool operator==(const BTWSyncMarkerData &other) const {
        return id == other.id;
    }
};

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
          isGraphContainerInFollowMode(true),
          isAbsoluteTime(true),
          hasAbsoluteTime(false),
          hasManoeuvres(false)
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

    // Absolute/Relative time mode synchronization
    bool isAbsoluteTime = true;
    bool hasAbsoluteTime = false;

    // Time selections synchronization
    std::vector<TimeSelectionSpan> timeSelections;

    // Manoeuvres synchronization
    std::vector<Manoeuvre> manoeuvres;
    bool hasManoeuvres;
    
    // BTW marker synchronization
    std::vector<BTWSyncMarkerData> btwMarkers;
    bool hasBTWMarkers = false;
    
    /**
     * @brief Add or update a BTW marker in the sync state
     * @param marker The marker data to add/update
     */
    void addOrUpdateBTWMarker(const BTWSyncMarkerData &marker)
    {
        for (auto &m : btwMarkers) {
            if (m.id == marker.id) {
                m = marker;
                hasBTWMarkers = true;
                return;
            }
        }
        btwMarkers.push_back(marker);
        hasBTWMarkers = true;
    }
    
    /**
     * @brief Remove a BTW marker from the sync state
     * @param id The unique ID of the marker to remove
     */
    void removeBTWMarker(const QUuid &id)
    {
        for (auto &m : btwMarkers) {
            if (m.id == id) {
                m.isDeleted = true;
                return;
            }
        }
    }
    
    /**
     * @brief Get a BTW marker by ID
     * @param id The unique ID of the marker
     * @return Pointer to the marker data, or nullptr if not found
     */
    BTWSyncMarkerData* getBTWMarker(const QUuid &id)
    {
        for (auto &m : btwMarkers) {
            if (m.id == id && !m.isDeleted) {
                return &m;
            }
        }
        return nullptr;
    }
    
    /**
     * @brief Get all active (non-deleted) BTW markers
     * @return Vector of active marker data
     */
    std::vector<BTWSyncMarkerData> getActiveBTWMarkers() const
    {
        std::vector<BTWSyncMarkerData> active;
        for (const auto &m : btwMarkers) {
            if (!m.isDeleted) {
                active.push_back(m);
            }
        }
        return active;
    }
    
    /**
     * @brief Clear all BTW markers
     */
    void clearBTWMarkers()
    {
        btwMarkers.clear();
        hasBTWMarkers = false;
    }
};


#endif // SHARED_SYNC_STATE_H