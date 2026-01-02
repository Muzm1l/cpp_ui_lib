# Timer Interval Implementation Guide

**Last Updated:** 2024

## Overview

This document explains how the timer interval system works and how to properly add new interval values, specifically covering how to implement a 10-second interval.

## Current Implementation

### TimeInterval Enum

The `TimeInterval` enum is defined in `timelineutils.h` and represents time intervals in **minutes**:

```cpp
enum class TimeInterval
{
    OneMinute = 1,       // 1 minute
    FiveMinutes = 5,     // 5 minutes
    FifteenMinutes = 15, // 15 minutes
    ThirtyMinutes = 30,  // 30 minutes
    OneHour = 60,        // 1 hour
    TwoHours = 120,      // 2 hours
    ThreeHours = 180,   // 3 hours
    FourHours = 240,     // 4 hours
    SixHours = 360,      // 6 hours
    TwelveHours = 720,   // 12 hours
};
```

### Conversion to Milliseconds

The conversion from `TimeInterval` to milliseconds is done in `WaterfallGraph::getTimeIntervalMs()`:

```cpp
qint64 WaterfallGraph::getTimeIntervalMs() const
{
    return static_cast<qint64>(timeInterval) * 60 * 1000; // Convert minutes to milliseconds
}
```

This assumes all enum values represent **minutes**, which works for the current intervals but doesn't support sub-minute intervals like 10 seconds.

## Adding a 10-Second Interval

To properly add a 10-second interval, you need to modify several parts of the codebase. Here's the step-by-step approach:

### Step 1: Add Enum Value

**File:** `timelineutils.h`

Add a special enum value for 10 seconds. Since the enum currently uses minutes, we need to use a value that can be distinguished from minute-based values:

```cpp
enum class TimeInterval
{
    TenSeconds = 0,      // Special value: 10 seconds (0 indicates sub-minute)
    OneMinute = 1,       // 1 minute
    FiveMinutes = 5,     // 5 minutes
    FifteenMinutes = 15, // 15 minutes
    // ... rest of intervals
};
```

**Alternative Approach (Recommended):** Use a negative value or a large value to distinguish it:

```cpp
enum class TimeInterval
{
    TenSeconds = -1,     // Special value: 10 seconds (negative indicates sub-minute)
    OneMinute = 1,       // 1 minute
    FiveMinutes = 5,     // 5 minutes
    // ... rest
};
```

**Best Approach:** Use a sentinel value that's clearly not a minute value:

```cpp
enum class TimeInterval
{
    TenSeconds = 1000,   // Sentinel value: 10 seconds (1000 = special code)
    OneMinute = 1,       // 1 minute
    FiveMinutes = 5,     // 5 minutes
    // ... rest
};
```

### Step 2: Update Conversion Function

**File:** `waterfallgraph.cpp`

Modify `getTimeIntervalMs()` to handle the special 10-second case:

```cpp
qint64 WaterfallGraph::getTimeIntervalMs() const
{
    // Handle special case: 10 seconds
    if (timeInterval == TimeInterval::TenSeconds) {
        return 10 * 1000; // 10 seconds in milliseconds
    }
    
    // Normal case: convert minutes to milliseconds
    return static_cast<qint64>(timeInterval) * 60 * 1000;
}
```

### Step 3: Update String Conversion

**File:** `timelineutils.h`

Add the 10-second case to `timeIntervalToString()`:

```cpp
inline QString timeIntervalToString(TimeInterval interval)
{
    switch (interval)
    {
    case TimeInterval::TenSeconds:
        return "10 seconds";
    case TimeInterval::OneMinute:
        return "1 minute";
    // ... rest of cases
    default:
        return "Unknown";
    }
}
```

### Step 4: Update QTime Conversion

**File:** `timelineutils.h`

Modify `timeIntervalToQTime()` to handle 10 seconds:

```cpp
inline QTime timeIntervalToQTime(TimeInterval interval)
{
    // Handle special case: 10 seconds
    if (interval == TimeInterval::TenSeconds) {
        return QTime(0, 0, 10); // 0 hours, 0 minutes, 10 seconds
    }
    
    // Normal case: convert minutes to hours/minutes
    int totalMinutes = static_cast<int>(interval);
    int hours = totalMinutes / 60;
    int minutes = totalMinutes % 60;
    return QTime(hours, minutes, 0);
}
```

### Step 5: Add to Valid Intervals List

**File:** `timelineutils.h`

Add `TenSeconds` to the list of valid intervals that users can select:

```cpp
inline std::vector<TimeInterval> getValidTimeIntervals()
{
    return std::vector<TimeInterval>{
        TimeInterval::TenSeconds,      // Add 10 seconds as first option
        TimeInterval::FifteenMinutes,
        TimeInterval::ThirtyMinutes,
        TimeInterval::OneHour,
        TimeInterval::TwoHours,
        TimeInterval::ThreeHours,
        TimeInterval::SixHours,
        TimeInterval::TwelveHours,
    };
}
```

### Step 6: Update Button Text Display

**File:** `timelineview.cpp`

The interval button text is updated in `updateButtonText()`. Ensure it handles the 10-second case:

```cpp
void TimelineView::updateButtonText(TimeInterval interval)
{
    QString buttonText;
    
    if (interval == TimeInterval::TenSeconds) {
        buttonText = "dt: 00:10";  // Display as 10 seconds
    } else {
        // Existing logic for minute-based intervals
        QTime length = timeIntervalToQTime(interval);
        buttonText = QString("dt: %1:%2")
            .arg(length.hour(), 2, 10, QChar('0'))
            .arg(length.minute(), 2, 10, QChar('0'));
    }
    
    m_intervalChangeButton->setText(buttonText);
}
```

## Complete Implementation Example

Here's a complete example showing all the changes needed:

### `timelineutils.h` Changes

```cpp
enum class TimeInterval
{
    TenSeconds = 1000,   // Special sentinel value for 10 seconds
    OneMinute = 1,
    FiveMinutes = 5,
    FifteenMinutes = 15,
    ThirtyMinutes = 30,
    OneHour = 60,
    TwoHours = 120,
    ThreeHours = 180,
    FourHours = 240,
    SixHours = 360,
    TwelveHours = 720,
};

inline QTime timeIntervalToQTime(TimeInterval interval)
{
    if (interval == TimeInterval::TenSeconds) {
        return QTime(0, 0, 10);
    }
    
    int totalMinutes = static_cast<int>(interval);
    int hours = totalMinutes / 60;
    int minutes = totalMinutes % 60;
    return QTime(hours, minutes, 0);
}

inline QString timeIntervalToString(TimeInterval interval)
{
    switch (interval)
    {
    case TimeInterval::TenSeconds:
        return "10 seconds";
    case TimeInterval::OneMinute:
        return "1 minute";
    case TimeInterval::FiveMinutes:
        return "5 minutes";
    // ... rest of cases
    default:
        return "Unknown";
    }
}

inline std::vector<TimeInterval> getValidTimeIntervals()
{
    return std::vector<TimeInterval>{
        TimeInterval::TenSeconds,      // Add as first option
        TimeInterval::FifteenMinutes,
        TimeInterval::ThirtyMinutes,
        TimeInterval::OneHour,
        TimeInterval::TwoHours,
        TimeInterval::ThreeHours,
        TimeInterval::SixHours,
        TimeInterval::TwelveHours,
    };
}
```

### `waterfallgraph.cpp` Changes

```cpp
qint64 WaterfallGraph::getTimeIntervalMs() const
{
    // Handle special case: 10 seconds
    if (timeInterval == TimeInterval::TenSeconds) {
        return 10 * 1000; // 10 seconds = 10,000 milliseconds
    }
    
    // Normal case: convert minutes to milliseconds
    return static_cast<qint64>(timeInterval) * 60 * 1000;
}
```

## Alternative: More Flexible Design

If you need to support multiple sub-minute intervals in the future, consider a more flexible design:

### Option 1: Separate Enum for Sub-Minute Intervals

```cpp
enum class SubMinuteInterval
{
    TenSeconds = 10,
    ThirtySeconds = 30,
};

enum class TimeInterval
{
    // Use negative values to indicate sub-minute intervals
    TenSeconds = -10,    // -10 means 10 seconds
    ThirtySeconds = -30, // -30 means 30 seconds
    OneMinute = 1,
    // ... rest
};
```

### Option 2: Use Fractional Minutes

```cpp
// Store as milliseconds directly
qint64 getTimeIntervalMs() const {
    if (timeInterval == TimeInterval::TenSeconds) {
        return 10000; // 10 seconds
    }
    // ... rest
}
```

### Option 3: Refactor to Milliseconds-Based Enum

A more comprehensive refactoring would change the enum to be milliseconds-based:

```cpp
enum class TimeInterval
{
    TenSeconds = 10000,      // milliseconds
    ThirtySeconds = 30000,
    OneMinute = 60000,
    FiveMinutes = 300000,
    // ... rest in milliseconds
};
```

This would require updating all conversion functions but provides the most flexibility.

## Testing

After implementing the 10-second interval, test:

1. **Interval Selection:** Verify the interval button cycles to "00:10"
2. **Time Window:** Check that the visible time window is exactly 10 seconds
3. **Data Display:** Ensure data within the 10-second window is displayed correctly
4. **Timeline Sync:** Verify timeline view updates correctly with 10-second interval
5. **Animation:** Check that the animation works smoothly with 10-second intervals

## Common Pitfalls

### Pitfall 1: Forgetting Special Case Handling

**Problem:** If you don't add special case handling in `getTimeIntervalMs()`, the 10-second interval will be treated as 1000 minutes.

**Solution:** Always add special case handling for non-minute intervals.

### Pitfall 2: Not Updating All Conversion Functions

**Problem:** Forgetting to update `timeIntervalToString()` or `timeIntervalToQTime()` causes display issues.

**Solution:** Update all conversion functions consistently.

### Pitfall 3: Cache Invalidation

**Problem:** The coordinate mapping cache depends on the time interval. Changing intervals invalidates caches.

**Solution:** The existing `setTimeInterval()` method already handles cache invalidation:

```cpp
void WaterfallGraph::setTimeInterval(TimeInterval interval)
{
    timeInterval = interval;
    m_cachesValid = false;  // Cache invalidation handled
    // ... rest
}
```

## Performance Considerations

### Very Short Intervals

10-second intervals mean:
- **More frequent updates:** Timeline animation updates more often
- **Smaller visible window:** Less data per screen, but more scrolling
- **Higher update rate:** Consider if 1-second timer is appropriate for 10-second windows

### Timer Frequency

The timeline view uses a 1-second timer by default (`m_timer->setInterval(1000)`). For 10-second intervals, this is appropriate. For even shorter intervals (e.g., 1 second), you might want to reduce the timer interval:

```cpp
// In TimelineView::setupTimer()
if (m_visualizerWidget->getTimeInterval() == TimeInterval::TenSeconds) {
    m_timer->setInterval(100); // Update 10 times per second for smooth animation
} else {
    m_timer->setInterval(1000); // Normal 1-second updates
}
```

## Summary

To add a 10-second interval:

1. ✅ Add `TenSeconds` enum value (use sentinel value like 1000)
2. ✅ Update `getTimeIntervalMs()` with special case
3. ✅ Update `timeIntervalToString()` with 10-second case
4. ✅ Update `timeIntervalToQTime()` with 10-second case
5. ✅ Add to `getValidTimeIntervals()` list
6. ✅ Update button text display logic
7. ✅ Test all functionality

The key insight is that the current system assumes **minutes**, so sub-minute intervals require special-case handling throughout the codebase.

