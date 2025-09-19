#ifndef TIMELINEUTILS_H
#define TIMELINEUTILS_H

#include <QTime>
#include <QString>
#include <vector>



enum class TimeInterval {
    OneMinute = 1,      // 1 minute
    FiveMinutes = 5,    // 5 minutes
    FifteenMinutes = 15, // 15 minutes
    ThirtyMinutes = 30,  // 30 minutes
    OneHour = 60,        // 1 hour
    TwoHours = 120,      // 2 hours
    ThreeHours = 180,    // 3 hours
    FourHours = 240,     // 4 hours
    SixHours = 360,     // 6 hours
    TwelveHours = 720,   // 12 hours
};

inline std::vector<TimeInterval> getValidTimeIntervals() {
    return std::vector<TimeInterval>{
        TimeInterval::FifteenMinutes,
        TimeInterval::ThirtyMinutes,
        TimeInterval::OneHour,
        TimeInterval::TwoHours,
        TimeInterval::ThreeHours,
        TimeInterval::SixHours,
        TimeInterval::TwelveHours,
    };
}

struct TimeSelectionSpan {
    QTime startTime;
    QTime endTime;
    
    TimeSelectionSpan() = default;
    TimeSelectionSpan(const QTime& start, const QTime& end) : startTime(start), endTime(end) {}
};

// Utility function to convert TimeInterval enum to QTime
inline QTime timeIntervalToQTime(TimeInterval interval) {
    int totalMinutes = static_cast<int>(interval);
    int hours = totalMinutes / 60;
    int minutes = totalMinutes % 60;
    return QTime(hours, minutes, 0);
}

// Utility function to get display name for TimeInterval
inline QString timeIntervalToString(TimeInterval interval) {
    switch (interval) {
        case TimeInterval::OneMinute: return "1 minute";
        case TimeInterval::FiveMinutes: return "5 minutes";
        case TimeInterval::FifteenMinutes: return "15 minutes";
        case TimeInterval::ThirtyMinutes: return "30 minutes";
        case TimeInterval::OneHour: return "1 hour";
        case TimeInterval::TwoHours: return "2 hours";
        case TimeInterval::ThreeHours: return "3 hours";
        case TimeInterval::FourHours: return "4 hours";
        case TimeInterval::SixHours: return "6 hours";
        case TimeInterval::TwelveHours: return "12 hours";
        default: return "Unknown";
    }
}

#endif // TIMELINEUTILS_H