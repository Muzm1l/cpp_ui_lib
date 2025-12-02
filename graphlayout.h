#ifndef GRAPHLAYOUT_H
#define GRAPHLAYOUT_H

#include "graphcontainer.h"
#include "graphtype.h"
#include "waterfalldata.h"
#include <QDateTime>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <map>
#include <vector>
#include "sharedsyncstate.h"

enum class LayoutType
{
    GPW1W = 0,  // 1 window only
    GPW4W = 1,  // 4 windows in 2x2 grid
    GPW2WV = 2, // 2 windows in vertical line
    GPW2WH = 3, // 2 windows in horizontal line
    GPW4WH = 4, // 4 windows in horizontal line
    NOGPW2WH = 5, // 2 windows in horizontal line, but take up whole screen
    HIDDEN = 6  // Hidden
};

class GraphLayout : public QWidget
{
    Q_OBJECT
public:
    explicit GraphLayout(QWidget *parent, LayoutType layoutType, QTimer *timer = nullptr, std::map<GraphType, std::vector<QPair<QString, QColor>>> seriesLabelsMap = std::map<GraphType, std::vector<QPair<QString, QColor>>>());
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
    void addDataPointToDataSource(const GraphType &graphType, const QString &seriesLabel, qreal yValue, const QDateTime &timestamp);
    void addDataPointsToDataSource(const GraphType &graphType, const QString &seriesLabel, const std::vector<qreal> &yValues, const std::vector<QDateTime> &timestamps);
    void setDataToDataSource(const GraphType &graphType, const QString &seriesLabel, const std::vector<qreal> &yData, const std::vector<QDateTime> &timestamps);
    void setDataToDataSource(const GraphType &graphType, const QString &seriesLabel, const WaterfallData &data);
    void clearDataSource(const GraphType &graphType, const QString &seriesLabel);

    // Data source management
    WaterfallData *getDataSource(const GraphType &graphType);
    bool hasDataSource(const GraphType &graphType) const;
    std::vector<GraphType> getDataSourceLabels() const;
    
    // Series-specific data source management
    bool hasSeriesInDataSource(const GraphType &graphType, const QString &seriesLabel) const;
    std::vector<QString> getSeriesLabelsInDataSource(const GraphType &graphType) const;
    void addSeriesToDataSource(const GraphType &graphType, const QString &seriesLabel);
    void removeSeriesFromDataSource(const GraphType &graphType, const QString &seriesLabel);

    // Container management
    std::vector<QString> getContainerLabels() const;
    bool hasContainer(const GraphType &graphType) const;

    // Set the current time
    void setCurrentTime(const QTime &time);
    void deleteInteractiveMarkers();

    // Selection linking methods
    void linkHorizontalContainers();
    
    // Timeline view syncing methods
    void syncAllTimelineViews();

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
    void setHardRangeLimits(const GraphType graphType, qreal yMin, qreal yMax);
    void removeHardRangeLimits(const GraphType graphType);
    void clearAllHardRangeLimits();
    bool hasHardRangeLimits(const GraphType graphType) const;
    std::pair<qreal, qreal> getHardRangeLimits(const GraphType graphType) const;

protected:
    void resizeEvent(QResizeEvent *event) override;

public slots:
    void onTimerTick();
    void onTimeSelectionCreated(const TimeSelectionSpan &selection);
    void onTimeSelectionsCleared();

public slots:
    void onContainerIntervalChanged(TimeInterval interval);

private:
    LayoutType m_layoutType;
    QTimer *m_timer;
    std::vector<GraphContainer *> m_graphContainers;
    std::vector<QString> m_containerLabels;

    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_graphContainersRow1Layout;
    QHBoxLayout *m_graphContainersRow2Layout;

    std::map<GraphType, WaterfallData *> m_dataSources;

    // Series colors map
    std::map<QString, QColor> m_seriesColorsMap;

    void attachContainerDataSources();
    void initializeContainers();
    void initializeDataSources(std::map<GraphType, std::vector<QPair<QString, QColor>>> seriesLabelsMap);
    int getContainerIndex(const QString &containerLabel) const;
    void disconnectAllContainerConnections();
    void propagateTimeSelectionToAllContainers(const TimeSelectionSpan &selection);
    void registerCursorSyncCallbacks();
    void onContainerCursorTimeChanged(GraphContainer *source, const QDateTime &time);
    void onContainerTimeScopeChanged(const TimeSelectionSpan &selection);

    // Container synchronization state
    GraphContainerSyncState m_syncState;

signals:
    void TimeSelectionCreated(const TimeSelectionSpan &selection);
    void TimeSelectionsCleared();
    
    // Marker timestamp signals for external integration
    /**
     * @brief Emitted when an RTW R marker is clicked
     * @param timestamp The timestamp of the clicked R marker
     * @param position The scene position where the marker was clicked
     */
    void RTWRMarkerTimestampCaptured(const QDateTime &timestamp, const QPointF &position);
    
    /**
     * @brief Emitted when a BTW manual marker is placed
     * @param timestamp The timestamp of the placed marker
     * @param position The scene position where the marker was placed
     */
    void BTWManualMarkerPlaced(const QDateTime &timestamp, const QPointF &position);
    
    /**
     * @brief Emitted when a BTW manual marker is clicked
     * @param timestamp The timestamp of the clicked marker
     * @param position The scene position where the marker was clicked
     */
    void BTWManualMarkerClicked(const QDateTime &timestamp, const QPointF &position);
};

#endif // GRAPHLAYOUT_H
