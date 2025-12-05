#ifndef SCWWINDOW_H
#define SCWWINDOW_H

#include "timelineview.h"
#include "waterfalldata.h"
#include "waterfallgraph.h"
#include <QHBoxLayout>
#include <QMap>
#include <QPushButton>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QWidget>
#include <QFrame>
#include <QMouseEvent>

// SCW_SERIES enum with labels
enum class SCW_SERIES_R
{
    RULER_1,
    RULER_2,
    RULER_3,
    RULER_4,
};

enum class SCW_SERIES_B
{
    BRAT,
    BOT,
    BFT,
    BOPT,
    BOTC,
};

enum class SCW_SERIES_A
{
    ATMA,
    ATMAF,
};

enum class SCW_SERIES_E
{
    EXTERNAL1,
    EXTERNAL2,
    EXTERNAL3,
    EXTERNAL4,
    EXTERNAL5,
};

enum class SCW_SERIES_ADOPTED
{
    ADOPTED,
};

// Helper function to convert SCW_SERIES to QString
QString scwSeriesRToString(SCW_SERIES_R series);
QString scwSeriesBToString(SCW_SERIES_B series);
QString scwSeriesAToString(SCW_SERIES_A series);
QString scwSeriesEToString(SCW_SERIES_E series);
QString scwSeriesAdoptedToString(SCW_SERIES_ADOPTED series);

// Helper function to convert QString to SCW_SERIES
SCW_SERIES_R stringToScwSeriesR(const QString &str);
SCW_SERIES_B stringToScwSeriesB(const QString &str);
SCW_SERIES_A stringToScwSeriesA(const QString &str);
SCW_SERIES_E stringToScwSeriesE(const QString &str);
SCW_SERIES_ADOPTED stringToScwSeriesAdopted(const QString &str);

class SCWWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SCWWindow(QWidget *parent = nullptr, QTimer *timer = nullptr);
    ~SCWWindow();

    // Data management APIs
    void setDataPoints(SCW_SERIES_ADOPTED series, const std::vector<qreal> &yData, const std::vector<QDateTime> &timestamps);
    void addDataPoints(SCW_SERIES_ADOPTED series, const std::vector<qreal> &yData, const std::vector<QDateTime> &timestamps);
    void setDataPoints(SCW_SERIES_R series, const std::vector<qreal> &yData, const std::vector<QDateTime> &timestamps);
    void addDataPoints(SCW_SERIES_R series, const std::vector<qreal> &yData, const std::vector<QDateTime> &timestamps);
    void setDataPoints(SCW_SERIES_B series, const std::vector<qreal> &yData, const std::vector<QDateTime> &timestamps);
    void addDataPoints(SCW_SERIES_B series, const std::vector<qreal> &yData, const std::vector<QDateTime> &timestamps);
    void setDataPoints(SCW_SERIES_A series, const std::vector<qreal> &yData, const std::vector<QDateTime> &timestamps);
    void addDataPoints(SCW_SERIES_A series, const std::vector<qreal> &yData, const std::vector<QDateTime> &timestamps);
    void setDataPoints(SCW_SERIES_E series, const std::vector<qreal> &yData, const std::vector<QDateTime> &timestamps);
    void addDataPoints(SCW_SERIES_E series, const std::vector<qreal> &yData, const std::vector<QDateTime> &timestamps);

signals:
    void seriesSelected(const QString &seriesName);

private:
    // Layout components
    QHBoxLayout *m_mainLayout;
    TimelineView *m_timelineView;

    // 8 container frames, each containing a vertical layout with button and waterfallgraph
    QFrame *m_seriesContainers[8];
    QVBoxLayout *m_seriesLayouts[8];
    QPushButton *m_seriesButtons[8];
    WaterfallGraph *m_waterfallGraphs[8];
    
    // Currently selected window index (-1 if none selected)
    int m_selectedWindowIndex = -1;

    // 8 WaterfallData data sources keyed by SCW_SERIES
    QMap<SCW_SERIES_ADOPTED, WaterfallData *> m_dataSourcesAdopted;
    QMap<SCW_SERIES_R, WaterfallData *> m_dataSourcesR;
    QMap<SCW_SERIES_B, WaterfallData *> m_dataSourcesB;
    QMap<SCW_SERIES_A, WaterfallData *> m_dataSourcesA;
    QMap<SCW_SERIES_E, WaterfallData *> m_dataSourcesE;

    // Timer for TimelineView
    QTimer *m_timer;
    
    // Current series indices for cycling windows (6, 7, 8)
    int m_currentSeriesBIndex = 0;  // Window 6: cycles through SCW_SERIES_B
    int m_currentSeriesAIndex = 0;  // Window 7: cycles through SCW_SERIES_A
    int m_currentSeriesEIndex = 0;  // Window 8: cycles through SCW_SERIES_E

    // Helper methods
    void setupLayout();
    void setupDataSources();
    void setupWaterfallGraphs();
    WaterfallData *getDataSourceAdopted(SCW_SERIES_ADOPTED series) const;
    WaterfallData *getDataSourceR(SCW_SERIES_R series) const;
    WaterfallData *getDataSourceB(SCW_SERIES_B series) const;
    WaterfallData *getDataSourceA(SCW_SERIES_A series) const;
    WaterfallData *getDataSourceE(SCW_SERIES_E series) const;
    
    // Selection methods
    void selectWindow(int windowIndex);
    QString getCurrentSeriesName(int windowIndex) const;
    bool eventFilter(QObject *obj, QEvent *event) override;
    
    // Helper methods for cycling
    void switchWindow6Series();
    void switchWindow7Series();
    void switchWindow8Series();

private slots:
    void onWindow1ButtonClicked();  // Select window 1 (ADOPTED)
    void onWindow2ButtonClicked();  // Select window 2
    void onWindow3ButtonClicked();  // Select window 3
    void onWindow4ButtonClicked();  // Select window 4
    void onWindow5ButtonClicked();  // Select window 5
    void onWindow6ButtonClicked();  // Cycle through SCW_SERIES_B
    void onWindow7ButtonClicked();  // Cycle through SCW_SERIES_A
    void onWindow8ButtonClicked();  // Cycle through SCW_SERIES_E
};

#endif // SCWWINDOW_H
