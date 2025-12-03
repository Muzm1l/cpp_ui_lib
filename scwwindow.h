#ifndef SCWWINDOW_H
#define SCWWINDOW_H

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMap>
#include <QSizePolicy>
#include "timelineview.h"
#include "waterfallgraph.h"
#include "waterfalldata.h"

// SCW_SERIES enum with labels
enum class SCW_SERIES
{
    BRAT,
    BOTV,
    ATMA,
    SERIES_4,
    SERIES_5,
    SERIES_6,
    SERIES_7
};

// Helper function to convert SCW_SERIES to QString
QString scwSeriesToString(SCW_SERIES series);

// Helper function to convert QString to SCW_SERIES
SCW_SERIES stringToScwSeries(const QString& str);

class SCWWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SCWWindow(QWidget* parent = nullptr, QTimer* timer = nullptr);
    ~SCWWindow();

    // Data management APIs
    void setDataPoints(SCW_SERIES series, const std::vector<qreal>& yData, const std::vector<QDateTime>& timestamps);
    void addDataPoints(SCW_SERIES series, const std::vector<qreal>& yData, const std::vector<QDateTime>& timestamps);

private:
    // Layout components
    QHBoxLayout* m_mainLayout;
    TimelineView* m_timelineView;
    
    // 7 vertical layouts, each containing a button and waterfallgraph
    QVBoxLayout* m_seriesLayouts[7];
    QPushButton* m_seriesButtons[7];
    WaterfallGraph* m_waterfallGraphs[7];
    
    // 7 WaterfallData data sources keyed by SCW_SERIES
    QMap<SCW_SERIES, WaterfallData*> m_dataSources;
    
    // Timer for TimelineView
    QTimer* m_timer;
    
    // Helper methods
    void setupLayout();
    void setupDataSources();
    void setupWaterfallGraphs();
    WaterfallData* getDataSource(SCW_SERIES series) const;
};

#endif // SCWWINDOW_H

