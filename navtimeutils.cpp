#include "navtimeutils.h"

/**
 * @brief Convert a NAVIC nav time to a system time
 * 
 * @param navTime 
 * @return QDateTime 
 */
QDateTime NavTimeUtils::covertNavTimeToSystemTime(const ulong navTime)
{
    // The nav time's epoch starts from Jan 1, 2010
    // TODO: Update this based on the actual nav time epoch, it can be NAVIC 
    // based that is 6th January 1980
    QDateTime navTimeEpoch = getNavTimeEpoch();

    QDateTime systemTimeDateTime = navTimeEpoch.addMSecs(navTime);
    return systemTimeDateTime;
}

/**
 * @brief Convert a system time to a NAVIC nav time
 * 
 * @param systemTime 
 * @return QDateTime 
 */
QDateTime NavTimeUtils::covertSystemTimeToNavTime(const QDateTime systemTime)
{
    // The nav time's epoch starts from Jan 1, 2010
    QDateTime navTimeEpoch = getNavTimeEpoch();
    
    ulong navTime = systemTime.toMSecsSinceEpoch() - navTimeEpoch.toMSecsSinceEpoch();
    return navTimeEpoch.addMSecs(navTime);
}

/**
 * @brief Get the NAVIC nav time epoch
 * 
 * @return QDateTime 
 */
QDateTime NavTimeUtils::getNavTimeEpoch()
{
    QDateTime navTimeEpoch = QDateTime(QDate(2010, 1, 1), QTime(0, 0, 0));
    return navTimeEpoch;
}

