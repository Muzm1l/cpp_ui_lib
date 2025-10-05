#ifndef NAVTIMEUTILS_H
#define NAVTIMEUTILS_H

#include <QTime>
#include <QDateTime>
#include <QDate>

using namespace std;

class NavTimeUtils
{

public:

    QDateTime covertNavTimeToSystemTime(const ulong navTime);

    QDateTime covertSystemTimeToNavTime(const QDateTime systemTime);

    QDateTime getNavTimeEpoch();

};

#endif // NAVTIMEUTILS_H
