#ifndef GRAPHCONTAINER_H
#define GRAPHCONTAINER_H

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QComboBox>
#include <QDateTime>
#include <QTimer>
#include <vector>
#include <map>
#include <QString>
#include "waterfallgraph.h"
#include "waterfalldata.h"
#include "timeselectionvisualizer.h"
#include "timelineview.h"
#include "zoompanel.h"
#include "timelineutils.h"
#include "graphtype.h"
#include "customwaterfallgraph.h"
#include "fdwgraph.h"
#include "bdwgraph.h"
#include "brwgraph.h"
#include "ltwgraph.h"
#include "btwgraph.h"
#include "rtwgraph.h"
#include "ftwgraph.h"

class GraphContainer : public QWidget
{
    Q_OBJECT
public:
    explicit GraphContainer(QWidget *parent = nullptr, bool showTimelineView = true, QTimer *timer = nullptr);
    ~GraphContainer();
    void setShowTimelineView(bool showTimelineView);
    bool getShowTimelineView();
    
    // Sizing methods
    int getTimelineWidth() const;
    
    // Graph view sizing methods
    void setGraphViewSize(int width, int height);
    QSize getGraphViewSize() const;
    QSize getTotalContainerSize() const;
    
    // Data point methods
    void setData(const std::vector<qreal>& yData, const std::vector<QDateTime>& timestamps);
    void setData(const WaterfallData& data);
    void clearData();
    void addDataPoint(qreal yValue, const QDateTime& timestamp);
    void addDataPoints(const std::vector<qreal>& yValues, const std::vector<QDateTime>& timestamps);
    
    // Data access methods
    WaterfallData getData() const;
    std::vector<std::pair<qreal, QDateTime>> getDataWithinYExtents(qreal yMin, qreal yMax) const;
    std::vector<std::pair<qreal, QDateTime>> getDataWithinTimeRange(const QDateTime& startTime, const QDateTime& endTime) const;
    
    // Min/max data methods
    qreal getMinY() const;
    qreal getMaxY() const;
    std::pair<qreal, qreal> getYRange() const;
    
    // Graph redraw method
    void redrawWaterfallGraph();
    
    // Data options management
    void addDataOption(const GraphType graphType, WaterfallData& dataSource);
    void removeDataOption(const GraphType graphType);
    void clearDataOptions();
    void setCurrentDataOption(const GraphType graphType);
    GraphType getCurrentDataOption() const;
    std::vector<GraphType> getAvailableDataOptions() const;
    WaterfallData* getDataOption(const GraphType graphType);
    bool hasDataOption(const GraphType graphType) const;
    
    // Signal subscription method for external components
    void subscribeToIntervalChange(QObject* subscriber, const char* slot);
    
    // Mouse selection control
    void setMouseSelectionEnabled(bool enabled);
    bool isMouseSelectionEnabled() const;

    // Set the current time 
    void setCurrentTime(const QTime& time);

    
    // Test method
    void testSelectionRectangle();
    
    // Public method for external components to update zoom panel limits
    void initializeZoomPanelLimits();
    
public slots:
    void onTimeIntervalChanged(TimeInterval interval);
    void onSelectionCreated(const TimeSelectionSpan& selection);
    void onZoomValueChanged(ZoomBounds bounds);
    
private:
    void updateTotalContainerSize();
    void updateComboBoxOptions();
    void onDataOptionChanged(QString title);
    void setupEventConnections();
    void setupEventConnectionsForWaterfallGraph();
    WaterfallGraph* createWaterfallGraph(GraphType graphType);
    void initializeWaterfallGraph(GraphType graphType);
    void setupTimer();
    void onTimerTick();
    
signals:
    void NewTimeSelectionCreated(qreal startTime, qreal endTime);
    void DeltaTimeSelectionChanged(qreal deltaTime);
    void TimeSelectionsCleared();
    void IntervalChanged(TimeInterval interval);

private:
    QHBoxLayout *m_mainLayout;
    QVBoxLayout *m_leftLayout;
    QComboBox *m_comboBox;
    ZoomPanel *m_zoomPanel;
    WaterfallGraph *m_waterfallGraph;
    TimeSelectionVisualizer *m_timelineSelectionView;
    TimelineView *m_timelineView;
    bool m_showTimelineView;
    
    // Timer management
    QTimer *m_timer;
    bool m_ownsTimer;
    
    // Sizing properties
    int m_timelineWidth;
    QSize m_graphViewSize;
    
    // Data source management
    WaterfallData waterfallData;
    
    // Data options management
    std::map<GraphType, WaterfallData*> dataOptions;
    GraphType currentDataOption;
};

#endif // GRAPHCONTAINER_H
