#include "scwwindow.h"
#include <QDebug>
#include <QString>

// Helper function implementations
QString scwSeriesToString(SCW_SERIES series)
{
    switch (series)
    {
        case SCW_SERIES::BRAT:
            return "BRAT";
        case SCW_SERIES::BOTV:
            return "BOTV";
        case SCW_SERIES::ATMA:
            return "ATMA";
        case SCW_SERIES::SERIES_4:
            return "SERIES_4";
        case SCW_SERIES::SERIES_5:
            return "SERIES_5";
        case SCW_SERIES::SERIES_6:
            return "SERIES_6";
        case SCW_SERIES::SERIES_7:
            return "SERIES_7";
        default:
            return "UNKNOWN";
    }
}

SCW_SERIES stringToScwSeries(const QString& str)
{
    if (str == "BRAT")
        return SCW_SERIES::BRAT;
    else if (str == "BOTV")
        return SCW_SERIES::BOTV;
    else if (str == "ATMA")
        return SCW_SERIES::ATMA;
    else if (str == "SERIES_4")
        return SCW_SERIES::SERIES_4;
    else if (str == "SERIES_5")
        return SCW_SERIES::SERIES_5;
    else if (str == "SERIES_6")
        return SCW_SERIES::SERIES_6;
    else if (str == "SERIES_7")
        return SCW_SERIES::SERIES_7;
    else
        return SCW_SERIES::BRAT; // Default
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
    // Clean up WaterfallData objects
    for (auto it = m_dataSources.begin(); it != m_dataSources.end(); ++it)
    {
        delete it.value();
    }
    m_dataSources.clear();
    
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
    // Create 7 WaterfallData data sources, one for each SCW_SERIES
    // The key in the dictionary is the SCW_SERIES enum value
    
    // Create all 7 series
    SCW_SERIES seriesList[] = {
        SCW_SERIES::BRAT,
        SCW_SERIES::BOTV,
        SCW_SERIES::ATMA,
        SCW_SERIES::SERIES_4,
        SCW_SERIES::SERIES_5,
        SCW_SERIES::SERIES_6,
        SCW_SERIES::SERIES_7
    };
    
    for (int i = 0; i < 7; ++i)
    {
        SCW_SERIES series = seriesList[i];
        QString seriesLabel = scwSeriesToString(series);
        WaterfallData* dataSource = new WaterfallData(seriesLabel);
        m_dataSources[series] = dataSource;
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
    m_timelineView = new TimelineView(this, m_timer);
    m_timelineView->setObjectName("scwTimelineView");
    
    // Set size policy for TimelineView - fixed width, expanding height
    m_timelineView->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    
    // Disable history slider by finding the visualizer widget and disabling mouse interaction
    // The visualizer widget is a child widget that handles slider interaction
    // We need to find widgets that are NOT buttons (buttons need to remain functional)
    QList<QWidget*> allChildren = m_timelineView->findChildren<QWidget*>();
    for (QWidget* child : allChildren)
    {
        // Only disable mouse interaction on non-button widgets (the visualizer widget)
        // Buttons are QPushButton, so we skip those
        if (qobject_cast<QPushButton*>(child) == nullptr)
        {
            // Disable mouse tracking and make transparent to mouse events
            // This prevents the slider from responding to mouse events
            child->setMouseTracking(false);
            child->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        }
    }
    
    // Add TimelineView to main layout
    m_mainLayout->addWidget(m_timelineView);
    
    // Create 7 vertical layouts, each with a button and waterfallgraph
    SCW_SERIES seriesList[] = {
        SCW_SERIES::BRAT,
        SCW_SERIES::BOTV,
        SCW_SERIES::ATMA,
        SCW_SERIES::SERIES_4,
        SCW_SERIES::SERIES_5,
        SCW_SERIES::SERIES_6,
        SCW_SERIES::SERIES_7
    };
    
    for (int i = 0; i < 7; ++i)
    {
        SCW_SERIES series = seriesList[i];
        QString seriesLabel = scwSeriesToString(series);
        
        // Create vertical layout for this series
        m_seriesLayouts[i] = new QVBoxLayout();
        m_seriesLayouts[i]->setContentsMargins(0, 0, 0, 0);
        m_seriesLayouts[i]->setSpacing(2);
        
        // Create button for this series
        m_seriesButtons[i] = new QPushButton(seriesLabel, this);
        m_seriesButtons[i]->setFixedHeight(30);
        // Set size policy for button - fixed height, expanding width
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
        
        // Add button to vertical layout
        m_seriesLayouts[i]->addWidget(m_seriesButtons[i]);
        
        // WaterfallGraph will be added in setupWaterfallGraphs()
        // For now, we'll add a placeholder that will be replaced
        
        // Add vertical layout to main horizontal layout
        m_mainLayout->addLayout(m_seriesLayouts[i], 1); // Stretch factor of 1
    }
    
    // Set the layout
    setLayout(m_mainLayout);
    
    qDebug() << "SCWWindow layout setup completed";
}

void SCWWindow::setupWaterfallGraphs()
{
    // Create 7 WaterfallGraph widgets and connect them to their data sources
    SCW_SERIES seriesList[] = {
        SCW_SERIES::BRAT,
        SCW_SERIES::BOTV,
        SCW_SERIES::ATMA,
        SCW_SERIES::SERIES_4,
        SCW_SERIES::SERIES_5,
        SCW_SERIES::SERIES_6,
        SCW_SERIES::SERIES_7
    };
    
    for (int i = 0; i < 7; ++i)
    {
        SCW_SERIES series = seriesList[i];
        QString seriesLabel = scwSeriesToString(series);
        
        // Get the data source for this series
        WaterfallData* dataSource = getDataSource(series);
        if (!dataSource)
        {
            qDebug() << "Error: No data source found for series:" << seriesLabel;
            continue;
        }
        
        // Create WaterfallGraph for this series
        m_waterfallGraphs[i] = new WaterfallGraph(this, false, 8, TimeInterval::FifteenMinutes);
        m_waterfallGraphs[i]->setObjectName(QString("scwWaterfallGraph_%1").arg(seriesLabel));
        
        // Set size policy to allow the graph to expand and shrink with available space
        m_waterfallGraphs[i]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        
        // Disable cursor/crosshair in WaterfallGraph
        m_waterfallGraphs[i]->setCrosshairEnabled(false);
        m_waterfallGraphs[i]->setCursorLayerEnabled(false);
        
        // Connect to data source
        m_waterfallGraphs[i]->setDataSource(*dataSource);
        
        // Add WaterfallGraph to the corresponding vertical layout with stretch factor
        m_seriesLayouts[i]->addWidget(m_waterfallGraphs[i], 1); // Stretch factor of 1
        
        qDebug() << "Created and connected WaterfallGraph for series:" << seriesLabel;
    }
    
    qDebug() << "SCWWindow waterfall graphs setup completed";
}

WaterfallData* SCWWindow::getDataSource(SCW_SERIES series) const
{
    auto it = m_dataSources.find(series);
    if (it != m_dataSources.end())
    {
        return it.value();
    }
    return nullptr;
}

void SCWWindow::setDataPoints(SCW_SERIES series, const std::vector<qreal>& yData, const std::vector<QDateTime>& timestamps)
{
    WaterfallData* dataSource = getDataSource(series);
    if (!dataSource)
    {
        qDebug() << "Error: No data source found for series:" << scwSeriesToString(series);
        return;
    }
    
    QString seriesLabel = scwSeriesToString(series);
    
    // Find the corresponding WaterfallGraph and use its setData method
    // This will handle both the data source update and the redraw
    SCW_SERIES seriesList[] = {
        SCW_SERIES::BRAT,
        SCW_SERIES::BOTV,
        SCW_SERIES::ATMA,
        SCW_SERIES::SERIES_4,
        SCW_SERIES::SERIES_5,
        SCW_SERIES::SERIES_6,
        SCW_SERIES::SERIES_7
    };
    
    for (int i = 0; i < 7; ++i)
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

void SCWWindow::addDataPoints(SCW_SERIES series, const std::vector<qreal>& yData, const std::vector<QDateTime>& timestamps)
{
    WaterfallData* dataSource = getDataSource(series);
    if (!dataSource)
    {
        qDebug() << "Error: No data source found for series:" << scwSeriesToString(series);
        return;
    }
    
    QString seriesLabel = scwSeriesToString(series);
    
    // Find the corresponding WaterfallGraph and use its addDataPoints method
    // This will handle both the data source update and the incremental redraw
    SCW_SERIES seriesList[] = {
        SCW_SERIES::BRAT,
        SCW_SERIES::BOTV,
        SCW_SERIES::ATMA,
        SCW_SERIES::SERIES_4,
        SCW_SERIES::SERIES_5,
        SCW_SERIES::SERIES_6,
        SCW_SERIES::SERIES_7
    };
    
    for (int i = 0; i < 7; ++i)
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

