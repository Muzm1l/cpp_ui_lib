#include "scwwindow.h"
#include <QDebug>
#include <QString>

// Helper function implementations
QString scwSeriesRToString(SCW_SERIES_R series)
{
    switch (series)
    {
        case SCW_SERIES_R::RULER_1:
            return "RULER_1";
        case SCW_SERIES_R::RULER_2:
            return "RULER_2";
        case SCW_SERIES_R::RULER_3:
            return "RULER_3";
        case SCW_SERIES_R::RULER_4:
            return "RULER_4";
        default:
            return "UNKNOWN";
    }
}

SCW_SERIES_R stringToScwSeriesR(const QString& str)
{
    if (str == "RULER_1")
        return SCW_SERIES_R::RULER_1;
    else if (str == "RULER_2")
        return SCW_SERIES_R::RULER_2;
    else if (str == "RULER_3")
        return SCW_SERIES_R::RULER_3;
    else if (str == "RULER_4")
        return SCW_SERIES_R::RULER_4;
    else
        return SCW_SERIES_R::RULER_1; // Default
}

void SCWWindow::setDataPoints(SCW_SERIES_R series, const std::vector<qreal>& yData, const std::vector<QDateTime>& timestamps)
{
    WaterfallData* dataSource = getDataSourceR(series);
    if (!dataSource)
    {
        qDebug() << "Error: No data source found for series:" << scwSeriesRToString(series);
        return;
    }

    QString seriesLabel = scwSeriesRToString(series);
    
    // Find the corresponding WaterfallGraph and use its setData method
    // This will handle both the data source update and the redraw
    SCW_SERIES_R seriesList[] = {
        SCW_SERIES_R::RULER_1,
        SCW_SERIES_R::RULER_2,
        SCW_SERIES_R::RULER_3,
        SCW_SERIES_R::RULER_4,
    };
    
    for (int i = 0; i < 4; ++i)
    {
        if (seriesList[i] == series && m_waterfallGraphs[i])
        {
            // Use setData method which handles data source update and redraw
            m_waterfallGraphs[i]->setData(seriesLabel, yData, timestamps);
            break;
        }
    }
    
    qDebug() << "setDataPoints called for series:" << seriesLabel << "with" << yData.size() << "points";
}

void SCWWindow::addDataPoints(SCW_SERIES_R series, const std::vector<qreal>& yData, const std::vector<QDateTime>& timestamps)
{
    WaterfallData* dataSource = getDataSourceR(series);
    if (!dataSource)
    {
        qDebug() << "Error: No data source found for series:" << scwSeriesRToString(series);
        return;
    }

    QString seriesLabel = scwSeriesRToString(series);
    
    // Find the corresponding WaterfallGraph and use its addDataPoints method
    // This will handle both the data source update and the incremental redraw
    SCW_SERIES_R seriesList[] = {
        SCW_SERIES_R::RULER_1,
        SCW_SERIES_R::RULER_2,
        SCW_SERIES_R::RULER_3,
        SCW_SERIES_R::RULER_4,
    };
    
    for (int i = 0; i < 4; ++i)
    {
        if (seriesList[i] == series && m_waterfallGraphs[i])
        {
            // Use addDataPoints method which handles data source update and incremental redraw
            m_waterfallGraphs[i]->addDataPoints(seriesLabel, yData, timestamps);
            break;
        }
    }
    
    qDebug() << "addDataPoints called for series:" << seriesLabel << "with" << yData.size() << "points";
}

WaterfallData* SCWWindow::getDataSourceR(SCW_SERIES_R series) const
{
    return m_dataSourcesR[series];
}

QString scwSeriesBToString(SCW_SERIES_B series)
{
    switch (series)
    {
        case SCW_SERIES_B::BRAT:
            return "BRAT";
        case SCW_SERIES_B::BOT:
            return "BOT";
        case SCW_SERIES_B::BFT:
            return "BFT";
        case SCW_SERIES_B::BOPT:
            return "BOPT";
        case SCW_SERIES_B::BOTC:
            return "BOTC";
        default:
            return "UNKNOWN";
    }
}

SCW_SERIES_B stringToScwSeriesB(const QString& str)
{
    if (str == "BRAT")
        return SCW_SERIES_B::BRAT;
    else if (str == "BOT")
        return SCW_SERIES_B::BOT;
    else if (str == "BFT")
        return SCW_SERIES_B::BFT;
    else if (str == "BOPT")
        return SCW_SERIES_B::BOPT;
    else if (str == "BOTC")
        return SCW_SERIES_B::BOTC;
    else
        return SCW_SERIES_B::BRAT; // Default
}

QString scwSeriesAToString(SCW_SERIES_A series)
{
    switch (series)
    {
        case SCW_SERIES_A::ATMA:
            return "ATMA";
        case SCW_SERIES_A::ATMAF:
            return "ATMAF";
        default:
            return "UNKNOWN";
    }
}

SCW_SERIES_A stringToScwSeriesA(const QString& str)
{
    if (str == "ATMA")
        return SCW_SERIES_A::ATMA;
    else if (str == "ATMAF")
        return SCW_SERIES_A::ATMAF;
    else
        return SCW_SERIES_A::ATMA; // Default
}

QString scwSeriesEToString(SCW_SERIES_E series)
{
    switch (series)
    {
        case SCW_SERIES_E::EXTERNAL1:
            return "EXTERNAL1";
        case SCW_SERIES_E::EXTERNAL2:
            return "EXTERNAL2";
        case SCW_SERIES_E::EXTERNAL3:
            return "EXTERNAL3";
        case SCW_SERIES_E::EXTERNAL4:
            return "EXTERNAL4";
        case SCW_SERIES_E::EXTERNAL5:
            return "EXTERNAL5";
        default:
            return "UNKNOWN";
    }
}

SCW_SERIES_E stringToScwSeriesE(const QString& str)
{
    if (str == "EXTERNAL1")
        return SCW_SERIES_E::EXTERNAL1;
    else if (str == "EXTERNAL2")
        return SCW_SERIES_E::EXTERNAL2;
    else if (str == "EXTERNAL3")
        return SCW_SERIES_E::EXTERNAL3;
    else if (str == "EXTERNAL4")
        return SCW_SERIES_E::EXTERNAL4;
    else if (str == "EXTERNAL5")
        return SCW_SERIES_E::EXTERNAL5;
    else
        return SCW_SERIES_E::EXTERNAL1; // Default
}

SCWWindow::SCWWindow(QWidget* parent, QTimer* timer)
    : QWidget(parent), m_mainLayout(nullptr), m_timelineView(nullptr), m_timer(timer)
{
    // Initialize arrays
    for (int i = 0; i < 7; ++i)
    {
        m_seriesLayouts[i] = nullptr;
        m_seriesButtons[i] = nullptr;
        m_waterfallGraphs[i] = nullptr;
    }
    
    // Remove all margins and padding for snug fit
    setContentsMargins(0, 0, 0, 0);
    
    // Set size policy to allow the window to expand and shrink with available space
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Setup data sources first
    setupDataSources();
    
    // Setup layout
    setupLayout();
    
    // Setup waterfall graphs
    setupWaterfallGraphs();
    
    qDebug() << "SCWWindow created successfully";
}

SCWWindow::~SCWWindow()
{
    // Clean up WaterfallData objects for all series types
    for (auto it = m_dataSourcesR.begin(); it != m_dataSourcesR.end(); ++it)
    {
        delete it.value();
    }
    m_dataSourcesR.clear();
    
    for (auto it = m_dataSourcesB.begin(); it != m_dataSourcesB.end(); ++it)
    {
        delete it.value();
    }
    m_dataSourcesB.clear();
    
    for (auto it = m_dataSourcesA.begin(); it != m_dataSourcesA.end(); ++it)
    {
        delete it.value();
    }
    m_dataSourcesA.clear();
    
    for (auto it = m_dataSourcesE.begin(); it != m_dataSourcesE.end(); ++it)
    {
        delete it.value();
    }
    m_dataSourcesE.clear();
    
    // Clean up waterfall graphs (they are child widgets, so Qt will handle them)
    // But we need to clear the pointers
    for (int i = 0; i < 7; ++i)
    {
        m_waterfallGraphs[i] = nullptr;
        m_seriesButtons[i] = nullptr;
        m_seriesLayouts[i] = nullptr;
    }
}

void SCWWindow::setupDataSources()
{
    // Create data sources for SCW_SERIES_R (RULER series)
    SCW_SERIES_R rulerSeries[] = {
        SCW_SERIES_R::RULER_1,
        SCW_SERIES_R::RULER_2,
        SCW_SERIES_R::RULER_3,
        SCW_SERIES_R::RULER_4,
    };
    for (int i = 0; i < 4; ++i)
    {
        SCW_SERIES_R series = rulerSeries[i];
        QString seriesLabel = scwSeriesRToString(series);
        WaterfallData* dataSource = new WaterfallData(seriesLabel);
        m_dataSourcesR[series] = dataSource;
        qDebug() << "Created WaterfallData for series:" << seriesLabel;
    }
    
    // Create data sources for SCW_SERIES_B
    SCW_SERIES_B seriesBList[] = {
        SCW_SERIES_B::BRAT,
        SCW_SERIES_B::BOT,
        SCW_SERIES_B::BFT,
        SCW_SERIES_B::BOPT,
        SCW_SERIES_B::BOTC,
    };
    for (int i = 0; i < 5; ++i)
    {
        SCW_SERIES_B series = seriesBList[i];
        QString seriesLabel = scwSeriesBToString(series);
        WaterfallData* dataSource = new WaterfallData(seriesLabel);
        m_dataSourcesB[series] = dataSource;
        qDebug() << "Created WaterfallData for series:" << seriesLabel;
    }
    
    // Create data sources for SCW_SERIES_A
    SCW_SERIES_A seriesAList[] = {
        SCW_SERIES_A::ATMA,
        SCW_SERIES_A::ATMAF,
    };
    for (int i = 0; i < 2; ++i)
    {
        SCW_SERIES_A series = seriesAList[i];
        QString seriesLabel = scwSeriesAToString(series);
        WaterfallData* dataSource = new WaterfallData(seriesLabel);
        m_dataSourcesA[series] = dataSource;
        qDebug() << "Created WaterfallData for series:" << seriesLabel;
    }
    
    // Create data sources for SCW_SERIES_E
    SCW_SERIES_E seriesEList[] = {
        SCW_SERIES_E::EXTERNAL1,
        SCW_SERIES_E::EXTERNAL2,
        SCW_SERIES_E::EXTERNAL3,
        SCW_SERIES_E::EXTERNAL4,
        SCW_SERIES_E::EXTERNAL5,
    };
    for (int i = 0; i < 5; ++i)
    {
        SCW_SERIES_E series = seriesEList[i];
        QString seriesLabel = scwSeriesEToString(series);
        WaterfallData* dataSource = new WaterfallData(seriesLabel);
        m_dataSourcesE[series] = dataSource;
        qDebug() << "Created WaterfallData for series:" << seriesLabel;
    }
}
void SCWWindow::setupLayout()
{
    // Create main horizontal layout
    m_mainLayout = new QHBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(5);
    
    // Create TimelineView
    m_timelineView = new TimelineView(this, m_timer, nullptr, false, false);
    m_timelineView->setObjectName("scwTimelineView");
    
    // Set size policy for TimelineView - fixed width, expanding height
    m_timelineView->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    
    // Disable mouse interaction on the visualizer widget
    QList<QWidget*> allChildren = m_timelineView->findChildren<QWidget*>();
    for (QWidget* child : allChildren)
    {
        if (qobject_cast<QPushButton*>(child) == nullptr)
        {
            child->setMouseTracking(false);
            child->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        }
    }
    
    // Add TimelineView to main layout
    m_mainLayout->addWidget(m_timelineView);
    
    // Windows 1-4: Fixed RULER series
    SCW_SERIES_R rulerSeries[] = {
        SCW_SERIES_R::RULER_1,
        SCW_SERIES_R::RULER_2,
        SCW_SERIES_R::RULER_3,
        SCW_SERIES_R::RULER_4,
    };
    
    for (int i = 0; i < 4; ++i)
    {
        SCW_SERIES_R series = rulerSeries[i];
        QString seriesLabel = scwSeriesRToString(series);
        
        // Create container frame for this series
        m_seriesContainers[i] = new QFrame(this);
        m_seriesContainers[i]->setFrameStyle(QFrame::NoFrame);
        m_seriesContainers[i]->setStyleSheet("QFrame { border: 2px solid transparent; }");
        m_seriesContainers[i]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        
        // Create vertical layout for this series
        m_seriesLayouts[i] = new QVBoxLayout(m_seriesContainers[i]);
        m_seriesLayouts[i]->setContentsMargins(0, 0, 0, 0);
        m_seriesLayouts[i]->setSpacing(2);
        
        // Create button for this series (fixed, no cycling)
        m_seriesButtons[i] = new QPushButton(seriesLabel, m_seriesContainers[i]);
        m_seriesButtons[i]->setFixedHeight(30);
        m_seriesButtons[i]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_seriesButtons[i]->setStyleSheet(
            "QPushButton {"
            "    background-color: black;"
            "    border: 2px solid white;"
            "    color: white;"
            "    font-weight: bold;"
            "    margin: 0px;"
            "    padding: 0px;"
            "}"
            "QPushButton:hover {"
            "    background-color: darkgrey;"
            "}"
            "QPushButton:pressed {"
            "    background-color: dimgrey;"
            "}");
        
        // Connect button click to selection handler
        switch(i) {
            case 0: connect(m_seriesButtons[i], &QPushButton::clicked, this, &SCWWindow::onWindow1ButtonClicked); break;
            case 1: connect(m_seriesButtons[i], &QPushButton::clicked, this, &SCWWindow::onWindow2ButtonClicked); break;
            case 2: connect(m_seriesButtons[i], &QPushButton::clicked, this, &SCWWindow::onWindow3ButtonClicked); break;
            case 3: connect(m_seriesButtons[i], &QPushButton::clicked, this, &SCWWindow::onWindow4ButtonClicked); break;
        }
        
        // Add button to vertical layout
        m_seriesLayouts[i]->addWidget(m_seriesButtons[i]);
        
        // Add container to main horizontal layout
        m_mainLayout->addWidget(m_seriesContainers[i], 1);
    }
    
    // Window 5: Cycle through SCW_SERIES_B
    m_seriesContainers[4] = new QFrame(this);
    m_seriesContainers[4]->setFrameStyle(QFrame::NoFrame);
    m_seriesContainers[4]->setStyleSheet("QFrame { border: 2px solid transparent; }");
    m_seriesContainers[4]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    m_seriesLayouts[4] = new QVBoxLayout(m_seriesContainers[4]);
    m_seriesLayouts[4]->setContentsMargins(0, 0, 0, 0);
    m_seriesLayouts[4]->setSpacing(2);
    
    SCW_SERIES_B initialSeriesB = SCW_SERIES_B::BRAT;
    m_seriesButtons[4] = new QPushButton(scwSeriesBToString(initialSeriesB), m_seriesContainers[4]);
    m_seriesButtons[4]->setFixedHeight(30);
    m_seriesButtons[4]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_seriesButtons[4]->setStyleSheet(
        "QPushButton {"
        "    background-color: black;"
        "    border: 2px solid white;"
        "    color: white;"
        "    font-weight: bold;"
        "    margin: 0px;"
        "    padding: 0px;"
        "}"
        "QPushButton:hover {"
        "    background-color: darkgrey;"
        "}"
        "QPushButton:pressed {"
        "    background-color: dimgrey;"
        "}");
    connect(m_seriesButtons[4], &QPushButton::clicked, this, &SCWWindow::onWindow5ButtonClicked);
    m_seriesLayouts[4]->addWidget(m_seriesButtons[4]);
    m_mainLayout->addWidget(m_seriesContainers[4], 1);
    
    // Window 6: Cycle through SCW_SERIES_A
    m_seriesContainers[5] = new QFrame(this);
    m_seriesContainers[5]->setFrameStyle(QFrame::NoFrame);
    m_seriesContainers[5]->setStyleSheet("QFrame { border: 2px solid transparent; }");
    m_seriesContainers[5]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    m_seriesLayouts[5] = new QVBoxLayout(m_seriesContainers[5]);
    m_seriesLayouts[5]->setContentsMargins(0, 0, 0, 0);
    m_seriesLayouts[5]->setSpacing(2);
    
    SCW_SERIES_A initialSeriesA = SCW_SERIES_A::ATMA;
    m_seriesButtons[5] = new QPushButton(scwSeriesAToString(initialSeriesA), m_seriesContainers[5]);
    m_seriesButtons[5]->setFixedHeight(30);
    m_seriesButtons[5]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_seriesButtons[5]->setStyleSheet(
        "QPushButton {"
        "    background-color: black;"
        "    border: 2px solid white;"
        "    color: white;"
        "    font-weight: bold;"
        "    margin: 0px;"
        "    padding: 0px;"
        "}"
        "QPushButton:hover {"
        "    background-color: darkgrey;"
        "}"
        "QPushButton:pressed {"
        "    background-color: dimgrey;"
        "}");
    connect(m_seriesButtons[5], &QPushButton::clicked, this, &SCWWindow::onWindow6ButtonClicked);
    m_seriesLayouts[5]->addWidget(m_seriesButtons[5]);
    m_mainLayout->addWidget(m_seriesContainers[5], 1);
    
    // Window 7: Cycle through SCW_SERIES_E
    m_seriesContainers[6] = new QFrame(this);
    m_seriesContainers[6]->setFrameStyle(QFrame::NoFrame);
    m_seriesContainers[6]->setStyleSheet("QFrame { border: 2px solid transparent; }");
    m_seriesContainers[6]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    m_seriesLayouts[6] = new QVBoxLayout(m_seriesContainers[6]);
    m_seriesLayouts[6]->setContentsMargins(0, 0, 0, 0);
    m_seriesLayouts[6]->setSpacing(2);
    
    SCW_SERIES_E initialSeriesE = SCW_SERIES_E::EXTERNAL1;
    m_seriesButtons[6] = new QPushButton(scwSeriesEToString(initialSeriesE), m_seriesContainers[6]);
    m_seriesButtons[6]->setFixedHeight(30);
    m_seriesButtons[6]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_seriesButtons[6]->setStyleSheet(
        "QPushButton {"
        "    background-color: black;"
        "    border: 2px solid white;"
        "    color: white;"
        "    font-weight: bold;"
        "    margin: 0px;"
        "    padding: 0px;"
        "}"
        "QPushButton:hover {"
        "    background-color: darkgrey;"
        "}"
        "QPushButton:pressed {"
        "    background-color: dimgrey;"
        "}");
    connect(m_seriesButtons[6], &QPushButton::clicked, this, &SCWWindow::onWindow7ButtonClicked);
    m_seriesLayouts[6]->addWidget(m_seriesButtons[6]);
    m_mainLayout->addWidget(m_seriesContainers[6], 1);
    
    // Set the layout
    setLayout(m_mainLayout);
    
    qDebug() << "SCWWindow layout setup completed";
}

void SCWWindow::setupWaterfallGraphs()
{
    // Windows 1-4: Connect to RULER series
    SCW_SERIES_R rulerSeries[] = {
        SCW_SERIES_R::RULER_1,
        SCW_SERIES_R::RULER_2,
        SCW_SERIES_R::RULER_3,
        SCW_SERIES_R::RULER_4,
    };
    
    for (int i = 0; i < 4; ++i)
    {
        SCW_SERIES_R series = rulerSeries[i];
        QString seriesLabel = scwSeriesRToString(series);
        
        WaterfallData* dataSource = getDataSourceR(series);
        if (!dataSource)
        {
            qDebug() << "Error: No data source found for series:" << seriesLabel;
            continue;
        }
        
        m_waterfallGraphs[i] = new WaterfallGraph(m_seriesContainers[i], false, 8, TimeInterval::FifteenMinutes);
        m_waterfallGraphs[i]->setObjectName(QString("scwWaterfallGraph_%1").arg(seriesLabel));
        m_waterfallGraphs[i]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_waterfallGraphs[i]->setCrosshairEnabled(false);
        m_waterfallGraphs[i]->setCursorLayerEnabled(false);
        m_waterfallGraphs[i]->setDataSource(*dataSource);
        m_waterfallGraphs[i]->installEventFilter(this);
        m_seriesLayouts[i]->addWidget(m_waterfallGraphs[i], 1);
        
        qDebug() << "Created and connected WaterfallGraph for window" << (i+1) << "series:" << seriesLabel;
    }
    
    // Window 5: Connect to initial SCW_SERIES_B (BRAT)
    SCW_SERIES_B initialSeriesB = SCW_SERIES_B::BRAT;
    QString seriesLabelB = scwSeriesBToString(initialSeriesB);
    WaterfallData* dataSourceB = getDataSourceB(initialSeriesB);
    if (dataSourceB)
    {
        m_waterfallGraphs[4] = new WaterfallGraph(m_seriesContainers[4], false, 8, TimeInterval::FifteenMinutes);
        m_waterfallGraphs[4]->setObjectName(QString("scwWaterfallGraph_%1").arg(seriesLabelB));
        m_waterfallGraphs[4]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_waterfallGraphs[4]->setCrosshairEnabled(false);
        m_waterfallGraphs[4]->setCursorLayerEnabled(false);
        m_waterfallGraphs[4]->setDataSource(*dataSourceB);
        m_waterfallGraphs[4]->installEventFilter(this);
        m_seriesLayouts[4]->addWidget(m_waterfallGraphs[4], 1);
        qDebug() << "Created and connected WaterfallGraph for window 5, initial series:" << seriesLabelB;
    }
    
    // Window 6: Connect to initial SCW_SERIES_A (ATMA)
    SCW_SERIES_A initialSeriesA = SCW_SERIES_A::ATMA;
    QString seriesLabelA = scwSeriesAToString(initialSeriesA);
    WaterfallData* dataSourceA = getDataSourceA(initialSeriesA);
    if (dataSourceA)
    {
        m_waterfallGraphs[5] = new WaterfallGraph(m_seriesContainers[5], false, 8, TimeInterval::FifteenMinutes);
        m_waterfallGraphs[5]->setObjectName(QString("scwWaterfallGraph_%1").arg(seriesLabelA));
        m_waterfallGraphs[5]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_waterfallGraphs[5]->setCrosshairEnabled(false);
        m_waterfallGraphs[5]->setCursorLayerEnabled(false);
        m_waterfallGraphs[5]->setDataSource(*dataSourceA);
        m_waterfallGraphs[5]->installEventFilter(this);
        m_seriesLayouts[5]->addWidget(m_waterfallGraphs[5], 1);
        qDebug() << "Created and connected WaterfallGraph for window 6, initial series:" << seriesLabelA;
    }
    
    // Window 7: Connect to initial SCW_SERIES_E (EXTERNAL1)
    SCW_SERIES_E initialSeriesE = SCW_SERIES_E::EXTERNAL1;
    QString seriesLabelE = scwSeriesEToString(initialSeriesE);
    WaterfallData* dataSourceE = getDataSourceE(initialSeriesE);
    if (dataSourceE)
    {
        m_waterfallGraphs[6] = new WaterfallGraph(m_seriesContainers[6], false, 8, TimeInterval::FifteenMinutes);
        m_waterfallGraphs[6]->setObjectName(QString("scwWaterfallGraph_%1").arg(seriesLabelE));
        m_waterfallGraphs[6]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_waterfallGraphs[6]->setCrosshairEnabled(false);
        m_waterfallGraphs[6]->setCursorLayerEnabled(false);
        m_waterfallGraphs[6]->setDataSource(*dataSourceE);
        m_waterfallGraphs[6]->installEventFilter(this);
        m_seriesLayouts[6]->addWidget(m_waterfallGraphs[6], 1);
        qDebug() << "Created and connected WaterfallGraph for window 7, initial series:" << seriesLabelE;
    }
    
    qDebug() << "SCWWindow waterfall graphs setup completed";
}

WaterfallData* SCWWindow::getDataSourceB(SCW_SERIES_B series) const
{
    return m_dataSourcesB[series];
}

WaterfallData* SCWWindow::getDataSourceA(SCW_SERIES_A series) const
{
    return m_dataSourcesA[series];
}

WaterfallData* SCWWindow::getDataSourceE(SCW_SERIES_E series) const
{
    return m_dataSourcesE[series];
}

void SCWWindow::setDataPoints(SCW_SERIES_B series, const std::vector<qreal>& yData, const std::vector<QDateTime>& timestamps)
{
    WaterfallData* dataSource = getDataSourceB(series);
    if (!dataSource)
    {
        qDebug() << "Error: No data source found for series:" << scwSeriesBToString(series);
        return;
    }
    
    QString seriesLabel = scwSeriesBToString(series);
    
    // For SCW_SERIES_B, window 5 cycles through these series
    SCW_SERIES_B seriesBList[] = {
        SCW_SERIES_B::BRAT,
        SCW_SERIES_B::BOT,
        SCW_SERIES_B::BFT,
        SCW_SERIES_B::BOPT,
        SCW_SERIES_B::BOTC,
    };
    
    // Check if this is the currently displayed series in window 5
    if (seriesBList[m_currentSeriesBIndex] == series && m_waterfallGraphs[4])
    {
        m_waterfallGraphs[4]->setData(seriesLabel, yData, timestamps);
    }
    else
    {
        // Update the data source even if not currently displayed
        dataSource->setDataSeries(seriesLabel, yData, timestamps);
    }
    
    qDebug() << "setDataPoints called for series:" << seriesLabel << "with" << yData.size() << "points";
}

void SCWWindow::addDataPoints(SCW_SERIES_B series, const std::vector<qreal>& yData, const std::vector<QDateTime>& timestamps)
{
    WaterfallData* dataSource = getDataSourceB(series);
    if (!dataSource)
    {
        qDebug() << "Error: No data source found for series:" << scwSeriesBToString(series);
        return;
    }
    
    QString seriesLabel = scwSeriesBToString(series);
    
    // For SCW_SERIES_B, window 5 cycles through these series
    SCW_SERIES_B seriesBList[] = {
        SCW_SERIES_B::BRAT,
        SCW_SERIES_B::BOT,
        SCW_SERIES_B::BFT,
        SCW_SERIES_B::BOPT,
        SCW_SERIES_B::BOTC,
    };
    
    // Check if this is the currently displayed series in window 5
    if (seriesBList[m_currentSeriesBIndex] == series && m_waterfallGraphs[4])
    {
        m_waterfallGraphs[4]->addDataPoints(seriesLabel, yData, timestamps);
    }
    else
    {
        // Update the data source even if not currently displayed
        dataSource->addDataPointsToSeries(seriesLabel, yData, timestamps);
    }
    
    qDebug() << "addDataPoints called for series:" << seriesLabel << "with" << yData.size() << "points";
}

// Implementations for SCW_SERIES_A
void SCWWindow::setDataPoints(SCW_SERIES_A series, const std::vector<qreal>& yData, const std::vector<QDateTime>& timestamps)
{
    WaterfallData* dataSource = getDataSourceA(series);
    if (!dataSource)
    {
        qDebug() << "Error: No data source found for series:" << scwSeriesAToString(series);
        return;
    }
    
    QString seriesLabel = scwSeriesAToString(series);
    
    // For SCW_SERIES_A, we need to find which window (6) is currently showing this series
    // Since window 6 cycles through SCW_SERIES_A, we check if it's currently showing this series
    SCW_SERIES_A seriesAList[] = {
        SCW_SERIES_A::ATMA,
        SCW_SERIES_A::ATMAF,
    };
    
    // Check if this is the currently displayed series in window 6
    if (seriesAList[m_currentSeriesAIndex] == series && m_waterfallGraphs[5])
    {
        m_waterfallGraphs[5]->setData(seriesLabel, yData, timestamps);
    }
    else
    {
        // Update the data source even if not currently displayed
        dataSource->setDataSeries(seriesLabel, yData, timestamps);
    }
    
    qDebug() << "setDataPoints called for series:" << seriesLabel << "with" << yData.size() << "points";
}

void SCWWindow::addDataPoints(SCW_SERIES_A series, const std::vector<qreal>& yData, const std::vector<QDateTime>& timestamps)
{
    WaterfallData* dataSource = getDataSourceA(series);
    if (!dataSource)
    {
        qDebug() << "Error: No data source found for series:" << scwSeriesAToString(series);
        return;
    }
    
    QString seriesLabel = scwSeriesAToString(series);
    
    SCW_SERIES_A seriesAList[] = {
        SCW_SERIES_A::ATMA,
        SCW_SERIES_A::ATMAF,
    };
    
    // Check if this is the currently displayed series in window 6
    if (seriesAList[m_currentSeriesAIndex] == series && m_waterfallGraphs[5])
    {
        m_waterfallGraphs[5]->addDataPoints(seriesLabel, yData, timestamps);
    }
    else
    {
        // Update the data source even if not currently displayed
        dataSource->addDataPointsToSeries(seriesLabel, yData, timestamps);
    }
    
    qDebug() << "addDataPoints called for series:" << seriesLabel << "with" << yData.size() << "points";
}

// Implementations for SCW_SERIES_E
void SCWWindow::setDataPoints(SCW_SERIES_E series, const std::vector<qreal>& yData, const std::vector<QDateTime>& timestamps)
{
    WaterfallData* dataSource = getDataSourceE(series);
    if (!dataSource)
    {
        qDebug() << "Error: No data source found for series:" << scwSeriesEToString(series);
        return;
    }
    
    QString seriesLabel = scwSeriesEToString(series);
    
    // For SCW_SERIES_E, we need to find which window (7) is currently showing this series
    SCW_SERIES_E seriesEList[] = {
        SCW_SERIES_E::EXTERNAL1,
        SCW_SERIES_E::EXTERNAL2,
        SCW_SERIES_E::EXTERNAL3,
        SCW_SERIES_E::EXTERNAL4,
        SCW_SERIES_E::EXTERNAL5,
    };
    
    // Check if this is the currently displayed series in window 7
    if (seriesEList[m_currentSeriesEIndex] == series && m_waterfallGraphs[6])
    {
        m_waterfallGraphs[6]->setData(seriesLabel, yData, timestamps);
    }
    else
    {
        // Update the data source even if not currently displayed
        dataSource->setDataSeries(seriesLabel, yData, timestamps);
    }
    
    qDebug() << "setDataPoints called for series:" << seriesLabel << "with" << yData.size() << "points";
}

void SCWWindow::addDataPoints(SCW_SERIES_E series, const std::vector<qreal>& yData, const std::vector<QDateTime>& timestamps)
{
    WaterfallData* dataSource = getDataSourceE(series);
    if (!dataSource)
    {
        qDebug() << "Error: No data source found for series:" << scwSeriesEToString(series);
        return;
    }
    
    QString seriesLabel = scwSeriesEToString(series);
    
    SCW_SERIES_E seriesEList[] = {
        SCW_SERIES_E::EXTERNAL1,
        SCW_SERIES_E::EXTERNAL2,
        SCW_SERIES_E::EXTERNAL3,
        SCW_SERIES_E::EXTERNAL4,
        SCW_SERIES_E::EXTERNAL5,
    };
    
    // Check if this is the currently displayed series in window 7
    if (seriesEList[m_currentSeriesEIndex] == series && m_waterfallGraphs[6])
    {
        m_waterfallGraphs[6]->addDataPoints(seriesLabel, yData, timestamps);
    }
    else
    {
        // Update the data source even if not currently displayed
        dataSource->addDataPointsToSeries(seriesLabel, yData, timestamps);
    }
    
    qDebug() << "addDataPoints called for series:" << seriesLabel << "with" << yData.size() << "points";
}

// Slot implementations for button clicks (old definitions removed - see below)

// Helper methods for cycling series
void SCWWindow::switchWindow5Series()
{
    SCW_SERIES_B seriesBList[] = {
        SCW_SERIES_B::BRAT,
        SCW_SERIES_B::BOT,
        SCW_SERIES_B::BFT,
        SCW_SERIES_B::BOPT,
        SCW_SERIES_B::BOTC,
    };
    
    // Cycle to next series
    m_currentSeriesBIndex = (m_currentSeriesBIndex + 1) % 5;
    SCW_SERIES_B newSeries = seriesBList[m_currentSeriesBIndex];
    QString seriesLabel = scwSeriesBToString(newSeries);
    
    // Update button text
    m_seriesButtons[4]->setText(seriesLabel);
    
    // Switch data source for the graph
    WaterfallData* newDataSource = getDataSourceB(newSeries);
    if (newDataSource && m_waterfallGraphs[4])
    {
        m_waterfallGraphs[4]->setDataSource(*newDataSource);
        m_waterfallGraphs[4]->draw();
        qDebug() << "Window 5 switched to series:" << seriesLabel;
    }
}

void SCWWindow::switchWindow6Series()
{
    SCW_SERIES_A seriesAList[] = {
        SCW_SERIES_A::ATMA,
        SCW_SERIES_A::ATMAF,
    };
    
    // Cycle to next series
    m_currentSeriesAIndex = (m_currentSeriesAIndex + 1) % 2;
    SCW_SERIES_A newSeries = seriesAList[m_currentSeriesAIndex];
    QString seriesLabel = scwSeriesAToString(newSeries);
    
    // Update button text
    m_seriesButtons[5]->setText(seriesLabel);
    
    // Switch data source for the graph
    WaterfallData* newDataSource = getDataSourceA(newSeries);
    if (newDataSource && m_waterfallGraphs[5])
    {
        m_waterfallGraphs[5]->setDataSource(*newDataSource);
        m_waterfallGraphs[5]->draw();
        qDebug() << "Window 6 switched to series:" << seriesLabel;
    }
}

void SCWWindow::switchWindow7Series()
{
    SCW_SERIES_E seriesEList[] = {
        SCW_SERIES_E::EXTERNAL1,
        SCW_SERIES_E::EXTERNAL2,
        SCW_SERIES_E::EXTERNAL3,
        SCW_SERIES_E::EXTERNAL4,
        SCW_SERIES_E::EXTERNAL5,
    };
    
    // Cycle to next series
    m_currentSeriesEIndex = (m_currentSeriesEIndex + 1) % 5;
    SCW_SERIES_E newSeries = seriesEList[m_currentSeriesEIndex];
    QString seriesLabel = scwSeriesEToString(newSeries);
    
    // Update button text
    m_seriesButtons[6]->setText(seriesLabel);
    
    // Switch data source for the graph
    WaterfallData* newDataSource = getDataSourceE(newSeries);
    if (newDataSource && m_waterfallGraphs[6])
    {
        m_waterfallGraphs[6]->setDataSource(*newDataSource);
        m_waterfallGraphs[6]->draw();
        qDebug() << "Window 7 switched to series:" << seriesLabel;
    }
}

// Selection methods
void SCWWindow::selectWindow(int windowIndex)
{
    if (windowIndex < 0 || windowIndex >= 7)
    {
        qDebug() << "Invalid window index:" << windowIndex;
        return;
    }
    
    // Deselect previous window
    if (m_selectedWindowIndex >= 0 && m_selectedWindowIndex < 7)
    {
        m_seriesContainers[m_selectedWindowIndex]->setStyleSheet("QFrame { border: 2px solid transparent; }");
    }
    
    // Select new window
    m_selectedWindowIndex = windowIndex;
    m_seriesContainers[windowIndex]->setStyleSheet("QFrame { border: 3px solid yellow; }");
    
    // Get current series name and emit signal
    QString seriesName = getCurrentSeriesName(windowIndex);
    emit seriesSelected(seriesName);
    
    qDebug() << "Window" << (windowIndex + 1) << "selected, series:" << seriesName;
}

QString SCWWindow::getCurrentSeriesName(int windowIndex) const
{
    if (windowIndex < 0 || windowIndex >= 7)
    {
        return QString();
    }
    
    if (windowIndex < 4)
    {
        // Windows 1-4: Fixed RULER series
        SCW_SERIES_R rulerSeries[] = {
            SCW_SERIES_R::RULER_1,
            SCW_SERIES_R::RULER_2,
            SCW_SERIES_R::RULER_3,
            SCW_SERIES_R::RULER_4,
        };
        return scwSeriesRToString(rulerSeries[windowIndex]);
    }
    else if (windowIndex == 4)
    {
        // Window 5: Current SCW_SERIES_B
        SCW_SERIES_B seriesBList[] = {
            SCW_SERIES_B::BRAT,
            SCW_SERIES_B::BOT,
            SCW_SERIES_B::BFT,
            SCW_SERIES_B::BOPT,
            SCW_SERIES_B::BOTC,
        };
        return scwSeriesBToString(seriesBList[m_currentSeriesBIndex]);
    }
    else if (windowIndex == 5)
    {
        // Window 6: Current SCW_SERIES_A
        SCW_SERIES_A seriesAList[] = {
            SCW_SERIES_A::ATMA,
            SCW_SERIES_A::ATMAF,
        };
        return scwSeriesAToString(seriesAList[m_currentSeriesAIndex]);
    }
    else if (windowIndex == 6)
    {
        // Window 7: Current SCW_SERIES_E
        SCW_SERIES_E seriesEList[] = {
            SCW_SERIES_E::EXTERNAL1,
            SCW_SERIES_E::EXTERNAL2,
            SCW_SERIES_E::EXTERNAL3,
            SCW_SERIES_E::EXTERNAL4,
            SCW_SERIES_E::EXTERNAL5,
        };
        return scwSeriesEToString(seriesEList[m_currentSeriesEIndex]);
    }
    
    return QString();
}

bool SCWWindow::eventFilter(QObject *obj, QEvent *event)
{
    // Check if this is a mouse press event on a WaterfallGraph
    if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton)
        {
            // Find which WaterfallGraph was clicked
            for (int i = 0; i < 7; ++i)
            {
                if (m_waterfallGraphs[i] && obj == m_waterfallGraphs[i])
                {
                    selectWindow(i);
                    return true; // Event handled
                }
            }
        }
    }
    
    // Let other events pass through
    return QWidget::eventFilter(obj, event);
}

// Button click handlers for windows 1-4 (selection only)
void SCWWindow::onWindow1ButtonClicked()
{
    selectWindow(0);
}

void SCWWindow::onWindow2ButtonClicked()
{
    selectWindow(1);
}

void SCWWindow::onWindow3ButtonClicked()
{
    selectWindow(2);
}

void SCWWindow::onWindow4ButtonClicked()
{
    selectWindow(3);
}

// Button click handlers for windows 5-7 (cycle only, no selection)
void SCWWindow::onWindow5ButtonClicked()
{
    switchWindow5Series();
    // Note: Selection only happens when clicking on the graph, not the button
}

void SCWWindow::onWindow6ButtonClicked()
{
    switchWindow6Series();
    // Note: Selection only happens when clicking on the graph, not the button
}

void SCWWindow::onWindow7ButtonClicked()
{
    switchWindow7Series();
    // Note: Selection only happens when clicking on the graph, not the button
}

