#ifndef GRAPHLAYOUT_H
#define GRAPHLAYOUT_H

#include "graphcontainer.h"
#include "graphtype.h"
#include "waterfalldata.h"
#include <QDateTime>
#include <QHBoxLayout>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <map>
#include <vector>

enum class LayoutType
{
    GPW1W = 0,  // 1 window only
    GPW4W = 1,  // 4 windows in 2x2 grid
    GPW2WV = 2, // 2 windows in vertical line
    GPW2WH = 3, // 2 windows in horizontal line
    GPW4WH = 4, // 4 windows in horizontal line
    HIDDEN = 5  // Hidden

};

class GraphLayout : public QWidget
{
    Q_OBJECT
public:
    explicit GraphLayout(QWidget *parent, LayoutType layoutType, QTimer *timer = nullptr);
    ~GraphLayout();

    void setLayoutType(LayoutType layoutType);
    LayoutType getLayoutType() const;

    // Sizing methods
    void setGraphViewSize(int width, int height);
    void updateLayoutSizing();

    // Data options management - operate on specific container by label
    void addDataOption(const QString &containerLabel, const GraphType &graphType, WaterfallData &dataSource);
    void removeDataOption(const QString &containerLabel, const GraphType &graphType);
    void clearDataOptions(const QString &containerLabel);
    void setCurrentDataOption(const QString &containerLabel, const GraphType &graphType);
    GraphType getCurrentDataOption(const QString &containerLabel) const;
    std::vector<GraphType> getAvailableDataOptions(const QString &containerLabel) const;
    WaterfallData *getDataOption(const QString &containerLabel, const GraphType &graphType);
    bool hasDataOption(const QString &containerLabel, const GraphType &graphType) const;

    // Data options management - operate on all visible containers
    void addDataOption(const GraphType &graphType, WaterfallData &dataSource);
    void removeDataOption(const GraphType &graphType);
    void clearDataOptions();
    void setCurrentDataOption(const GraphType &graphType);

    // Data point methods for specific data sources
    void addDataPointToDataSource(const GraphType &graphType, qreal yValue, const QDateTime &timestamp);
    void addDataPointsToDataSource(const GraphType &graphType, const std::vector<qreal> &yValues, const std::vector<QDateTime> &timestamps);
    void setDataToDataSource(const GraphType &graphType, const std::vector<qreal> &yData, const std::vector<QDateTime> &timestamps);
    void setDataToDataSource(const GraphType &graphType, const WaterfallData &data);
    void clearDataSource(const GraphType &graphType);

    // Data source management
    WaterfallData *getDataSource(const GraphType &graphType);
    bool hasDataSource(const GraphType &graphType) const;
    std::vector<GraphType> getDataSourceLabels() const;

    // Container management
    std::vector<QString> getContainerLabels() const;
    bool hasContainer(const GraphType &graphType) const;

    // Set the current time
    void setCurrentTime(const QTime &time);

    // Selection linking methods
    void linkHorizontalContainers();

    // Chevron label control methods - operate on all visible containers
    void setChevronLabel1(const QString &label);
    void setChevronLabel2(const QString &label);
    void setChevronLabel3(const QString &label);
    QString getChevronLabel1() const;
    QString getChevronLabel2() const;
    QString getChevronLabel3() const;

    // Chevron label control methods - operate on specific container by label
    void setChevronLabel1(const QString &containerLabel, const QString &label);
    void setChevronLabel2(const QString &containerLabel, const QString &label);
    void setChevronLabel3(const QString &containerLabel, const QString &label);
    QString getChevronLabel1(const QString &containerLabel) const;
    QString getChevronLabel2(const QString &containerLabel) const;
    QString getChevronLabel3(const QString &containerLabel) const;

    // Set range limits methods
    void setRangeLimits(const GraphType graphType, qreal yMin, qreal yMax);
    void removeRangeLimits(const GraphType graphType);
    void clearAllRangeLimits();
    bool hasRangeLimits(const GraphType graphType) const;
    std::pair<qreal, qreal> getRangeLimits(const GraphType graphType) const;

public slots:
    void onTimerTick();

private:
    LayoutType m_layoutType;
    QTimer *m_timer;

    std::vector<GraphContainer *> m_graphContainers;
    std::vector<QString> m_containerLabels;

    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_graphContainersRow1Layout;
    QHBoxLayout *m_graphContainersRow2Layout;

    std::map<GraphType, WaterfallData *> m_dataSources;

    void attachContainerDataSources();
    void initializeContainers();
    int getContainerIndex(const QString &containerLabel) const;
    void disconnectAllContainerConnections();

signals:
};

#endif // GRAPHLAYOUT_H
