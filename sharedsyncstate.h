#ifndef SHARED_SYNC_STATE_H
#define SHARED_SYNC_STATE_H

#include "timelineutils.h"
#include <QColor>
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

/**
 * @brief Data structure for synchronized shaded regions
 * 
 * Contains all the data needed to recreate a shaded region across
 * different BTW graph containers.
 */
struct ShadedRegionSyncData
{
    int id;                 ///< Unique identifier for the region (local ID)
    QUuid syncId;           ///< Global sync identifier across containers
    qreal startX;           ///< Starting X value (left range boundary)
    qreal endX;             ///< Ending X value (right range boundary)
    bool isDeleted;         ///< Flag to mark deleted regions
    
    ShadedRegionSyncData() : id(-1), startX(0.0), endX(0.0), isDeleted(false) {}
    
    ShadedRegionSyncData(int localId, qreal xStart, qreal xEnd)
        : id(localId)
        , syncId(QUuid::createUuid())
        , startX(xStart)
        , endX(xEnd)
        , isDeleted(false)
    {}
    
    bool operator==(const ShadedRegionSyncData &other) const {
        return syncId == other.syncId;
    }
};

/**
 * @brief Data structure for synchronized horizontal time lines (constant-time overlays).
 *
 * Drawn on BTW via user interaction; mirrored on all waterfall graph types and
 * containers through GraphContainerSyncState.
 */
struct HorizontalLineSyncData
{
    QUuid syncId;           ///< Global sync identifier across containers and graph types
    QDateTime timestamp;    ///< Time position (horizontal line = constant time)
    QColor color;           ///< Line color
    qreal width;            ///< Line width in pixels
    bool isDeleted;         ///< Soft-delete flag

    HorizontalLineSyncData()
        : color(Qt::white), width(2.0), isDeleted(false) {}

    HorizontalLineSyncData(const QDateTime &ts, const QColor &c = Qt::white, qreal w = 2.0)
        : syncId(QUuid::createUuid())
        , timestamp(ts)
        , color(c)
        , width(w)
        , isDeleted(false)
    {}

    bool operator==(const HorizontalLineSyncData &other) const {
        return syncId == other.syncId;
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

    // Shared application start time so all timeline views use same range for Y mapping (slider position consistency)
    QDateTime applicationStartTime;
    bool hasApplicationStartTime = false;

    /** When set, timeline slider "now" edge uses this instead of wall clock (e.g. replay paused at T). */
    QDateTime timelineEndOverride;
    bool hasTimelineEndOverride = false;

    QDateTime effectiveTimelineEnd() const
    {
        if (hasTimelineEndOverride && timelineEndOverride.isValid())
            return timelineEndOverride;
        return QDateTime::currentDateTime();
    }

    void setTimelineEndOverride(const QDateTime &t)
    {
        timelineEndOverride = t;
        hasTimelineEndOverride = t.isValid();
    }

    void clearTimelineEndOverride()
    {
        hasTimelineEndOverride = false;
        timelineEndOverride = QDateTime();
    }

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
    
    // ========== Shaded Region Synchronization ==========
    
    std::vector<ShadedRegionSyncData> shadedRegions;
    bool hasShadedRegions = false;
    
    /**
     * @brief Add or update a shaded region in the sync state
     * @param region The region data to add/update
     */
    void addOrUpdateShadedRegion(const ShadedRegionSyncData &region)
    {
        for (auto &r : shadedRegions) {
            if (r.syncId == region.syncId) {
                r = region;
                hasShadedRegions = true;
                return;
            }
        }
        shadedRegions.push_back(region);
        hasShadedRegions = true;
    }
    
    /**
     * @brief Remove a shaded region from the sync state
     * @param syncId The global sync ID of the region to remove
     */
    void removeShadedRegion(const QUuid &syncId)
    {
        for (auto &r : shadedRegions) {
            if (r.syncId == syncId) {
                r.isDeleted = true;
                return;
            }
        }
    }
    
    /**
     * @brief Get a shaded region by sync ID
     * @param syncId The global sync ID of the region
     * @return Pointer to the region data, or nullptr if not found
     */
    ShadedRegionSyncData* getShadedRegion(const QUuid &syncId)
    {
        for (auto &r : shadedRegions) {
            if (r.syncId == syncId && !r.isDeleted) {
                return &r;
            }
        }
        return nullptr;
    }
    
    /**
     * @brief Get all active (non-deleted) shaded regions
     * @return Vector of active region data
     */
    std::vector<ShadedRegionSyncData> getActiveShadedRegions() const
    {
        std::vector<ShadedRegionSyncData> active;
        for (const auto &r : shadedRegions) {
            if (!r.isDeleted) {
                active.push_back(r);
            }
        }
        return active;
    }
    
    /**
     * @brief Clear all shaded regions
     */
    void clearShadedRegions()
    {
        shadedRegions.clear();
        hasShadedRegions = false;
    }

    // ========== Horizontal Line Synchronization ==========

    std::vector<HorizontalLineSyncData> horizontalLines;
    bool hasHorizontalLines = false;

    void addOrUpdateHorizontalLine(const HorizontalLineSyncData &line)
    {
        for (auto &l : horizontalLines) {
            if (l.syncId == line.syncId) {
                l = line;
                hasHorizontalLines = true;
                return;
            }
        }
        horizontalLines.push_back(line);
        hasHorizontalLines = true;
    }

    void removeHorizontalLine(const QUuid &syncId)
    {
        for (auto &l : horizontalLines) {
            if (l.syncId == syncId) {
                l.isDeleted = true;
                return;
            }
        }
    }

    HorizontalLineSyncData* getHorizontalLine(const QUuid &syncId)
    {
        for (auto &l : horizontalLines) {
            if (l.syncId == syncId && !l.isDeleted) {
                return &l;
            }
        }
        return nullptr;
    }

    std::vector<HorizontalLineSyncData> getActiveHorizontalLines() const
    {
        std::vector<HorizontalLineSyncData> active;
        for (const auto &l : horizontalLines) {
            if (!l.isDeleted) {
                active.push_back(l);
            }
        }
        return active;
    }

    void clearHorizontalLines()
    {
        horizontalLines.clear();
        hasHorizontalLines = false;
    }
};


#endif // SHARED_SYNC_STATE_H