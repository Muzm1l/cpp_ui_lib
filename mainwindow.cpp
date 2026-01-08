#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "timelineutils.h"
#include "debugutils.h"
#include <QDateTime>
#include <QDebug>
#include <QLabel>
#include <QList>
#include <QStringList>
#include <QPainter>
#include <QFont>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTimer>
#include <cmath>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), timer(new QTimer(this)), timeUpdateTimer(new QTimer(this)),
      m_currentBTWLineMode(BTWGraph::HorizontalLineMode::Normal)
{
    ui->setupUi(this);
    
    // Hide TSW (Tactical Solution View) tab and widget - DISABLED
    int tacticalTabIndex = -1;
    for (int i = 0; i < ui->tabWidget->count(); ++i) {
        if (ui->tabWidget->tabText(i) == "Tactical View") {
            tacticalTabIndex = i;
            break;
        }
    }
    if (tacticalTabIndex >= 0) {
        ui->tabWidget->setTabVisible(tacticalTabIndex, false);
    }
    
    // Hide and disable the TacticalSolutionView widget itself
    if (ui->tsv) {
        ui->tsv->setVisible(false);
        ui->tsv->setEnabled(false);
    }

    // Initialize series labels map
    std::map<GraphType, std::vector<QPair<QString, QColor>>> seriesLabelsMap;
    
    // Initialize series labels map
    seriesLabelsMap[GraphType::BDW] = {{"BDW-1", QColor(Qt::red)}, {"ADOPTED", QColor(Qt::yellow)}};
    seriesLabelsMap[GraphType::BRW] = {{"BRW-1", QColor(Qt::green)}, {"BRW-2", QColor(Qt::blue)}, {"ADOPTED", QColor(Qt::yellow)}};
    seriesLabelsMap[GraphType::BTW] = {{"BTW-1", QColor(Qt::red)}, {"BTW-2", QColor(Qt::green)}, {"BTW-3", QColor(Qt::blue)}, {"ADOPTED", QColor(Qt::yellow)}};
    seriesLabelsMap[GraphType::FDW] = {{"FDW-1", QColor(Qt::red)}, {"FDW-2", QColor(Qt::green)}, {"ADOPTED", QColor(Qt::yellow)}};
    seriesLabelsMap[GraphType::FTW] = {{"FTW-1", QColor(Qt::red)}, {"FTW-2", QColor(Qt::green)}, {"ADOPTED", QColor(Qt::yellow)}};
    seriesLabelsMap[GraphType::LTW] = {{"LTW-1", QColor(Qt::red)}, {"ADOPTED", QColor(Qt::yellow)}};
    seriesLabelsMap[GraphType::RTW] = {{"RTW-1", QColor(Qt::red)}, {"ADOPTED", QColor(Qt::yellow)}};

    // Create GraphLayout programmatically with default graph types and timer
    graphgrid = new GraphLayout(ui->originalTab, LayoutType::GPW4W, timeUpdateTimer, seriesLabelsMap);
    graphgrid->setObjectName("graphgrid");
    graphgrid->setGeometry(QRect(100, 100, 900, 900));
    
    // Create container widget for controls below the layout combo box
    QWidget* controlsWidget = new QWidget(ui->originalTab);
    controlsWidget->setObjectName("controlsWidget");
    // Position below the layout combo box (combo box is at y=30, height=32, so start at y=70)
    controlsWidget->setGeometry(QRect(1100, 70, 300, 400));
    
    // Create vertical layout for controls (one by one below combo box)
    QVBoxLayout* controlsLayout = new QVBoxLayout(controlsWidget);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setSpacing(10);
    
    // Create label to display marker timestamp in first tab
    markerTimestampLabel = new QLabel("Marker Timestamp: --", controlsWidget);
    markerTimestampLabel->setObjectName("markerTimestampLabel");
    markerTimestampLabel->setStyleSheet("QLabel { color: white; font-size: 14px; font-weight: bold; background-color: rgba(0, 0, 0, 200); padding: 6px; border: 2px solid yellow; border-radius: 4px; }");
    markerTimestampLabel->setMinimumWidth(280);
    
    // Add label to layout
    controlsLayout->addWidget(markerTimestampLabel);
    
    // Connect marker timestamp signal from graphgrid to update label
    connect(graphgrid, &GraphLayout::markerTimestampValueChanged,
            [this](const QDateTime &timestamp, qreal value) {
                if (markerTimestampLabel && timestamp.isValid()) {
                    QString timestampStr = timestamp.toString("yyyy-MM-dd HH:mm:ss.zzz");
                    markerTimestampLabel->setText(QString("Marker Timestamp: %1 | Value: %2").arg(timestampStr).arg(value, 0, 'f', 2));
                    DEBUG_OUT() << "Marker timestamp updated:" << timestampStr << "Value:" << value;
                }
            });

    // Create label to display RTW symbol timestamp when clicked
    rtwSymbolTimestampLabel = new QLabel("RTW Symbol: --", controlsWidget);
    rtwSymbolTimestampLabel->setObjectName("rtwSymbolTimestampLabel");
    rtwSymbolTimestampLabel->setStyleSheet("QLabel { color: white; font-size: 14px; font-weight: bold; background-color: rgba(0, 0, 0, 200); padding: 6px; border: 2px solid cyan; border-radius: 4px; }");
    rtwSymbolTimestampLabel->setMinimumWidth(280);
    
    // Add label to layout
    controlsLayout->addWidget(rtwSymbolTimestampLabel);
    
    // Set debug output state programmatically (disabled by default)
    DebugUtils::setDebugEnabled(false);
    
    // Connect RTW symbol timestamp signal from graphgrid to update label
    connect(graphgrid, &GraphLayout::RTWSymbolTimestampCaptured,
            [this](const QDateTime &timestamp, const QPointF &position, const QString &symbolName) {
                if (rtwSymbolTimestampLabel && timestamp.isValid()) {
                    QString timestampStr = timestamp.toString("yyyy-MM-dd HH:mm:ss.zzz");
                    rtwSymbolTimestampLabel->setText(QString("RTW Symbol: %1 | Timestamp: %2 | Pos: (%3, %4)")
                        .arg(symbolName)
                        .arg(timestampStr)
                        .arg(position.x(), 0, 'f', 1)
                        .arg(position.y(), 0, 'f', 1));
                    DEBUG_OUT() << "RTW Symbol clicked:" << symbolName << "Timestamp:" << timestampStr << "Position:" << position;
                }
            });

    // Create label to display RTW R marker timestamp when clicked
    rtwRMarkerTimestampLabel = new QLabel("RTW R Marker: --", controlsWidget);
    rtwRMarkerTimestampLabel->setObjectName("rtwRMarkerTimestampLabel");
    rtwRMarkerTimestampLabel->setStyleSheet("QLabel { color: white; font-size: 14px; font-weight: bold; background-color: rgba(0, 0, 0, 200); padding: 6px; border: 2px solid yellow; border-radius: 4px; }");
    rtwRMarkerTimestampLabel->setMinimumWidth(280);
    
    // Add label to layout
    controlsLayout->addWidget(rtwRMarkerTimestampLabel);
    
    // Connect RTW R marker timestamp signal from graphgrid to update label
    connect(graphgrid, &GraphLayout::RTWRMarkerTimestampCaptured,
            [this](const QDateTime &timestamp, const QPointF &position) {
                if (rtwRMarkerTimestampLabel && timestamp.isValid()) {
                    QString timestampStr = timestamp.toString("yyyy-MM-dd HH:mm:ss.zzz");
                    rtwRMarkerTimestampLabel->setText(QString("RTW R Marker | Timestamp: %1 | Pos: (%2, %3)")
                        .arg(timestampStr)
                        .arg(position.x(), 0, 'f', 1)
                        .arg(position.y(), 0, 'f', 1));
                    DEBUG_OUT() << "RTW R Marker clicked - Timestamp:" << timestampStr << "Position:" << position;
                }
            });

    // Create button to test clearAllGraphs API
    clearAllGraphsButton = new QPushButton("Clear All Graphs", controlsWidget);
    clearAllGraphsButton->setObjectName("clearAllGraphsButton");
    clearAllGraphsButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #d32f2f;"
        "    border: 2px solid #b71c1c;"
        "    color: white;"
        "    font-size: 12px;"
        "    font-weight: bold;"
        "    padding: 6px 12px;"
        "    border-radius: 4px;"
        "    min-width: 120px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #c62828;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #b71c1c;"
        "}");
    
    // Add button to layout
    controlsLayout->addWidget(clearAllGraphsButton);
    
    // Connect button click to clearAllGraphs API
    connect(clearAllGraphsButton, &QPushButton::clicked, this, [this]() {
        if (graphgrid) {
            DEBUG_OUT() << "MainWindow: Clear All Graphs button clicked - calling clearAllGraphs()";
            graphgrid->clearAllGraphs();
            DEBUG_OUT() << "MainWindow: clearAllGraphs() completed";
        } else {
            DEBUG_OUT() << "MainWindow: Cannot clear graphs - graphgrid is null";
        }
    });

    // Create button to manually trigger redraw
    redrawGraphsButton = new QPushButton("Redraw All Graphs", controlsWidget);
    redrawGraphsButton->setObjectName("redrawGraphsButton");
    redrawGraphsButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #1976d2;"
        "    border: 2px solid #1565c0;"
        "    color: white;"
        "    font-size: 12px;"
        "    font-weight: bold;"
        "    padding: 6px 12px;"
        "    border-radius: 4px;"
        "    min-width: 120px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #1565c0;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #0d47a1;"
        "}");
    
    // Add button to layout
    controlsLayout->addWidget(redrawGraphsButton);
    
    // Connect button click to redrawAllGraphs API
    connect(redrawGraphsButton, &QPushButton::clicked, this, [this]() {
        if (graphgrid) {
            DEBUG_OUT() << "MainWindow: Redraw All Graphs button clicked - calling redrawAllGraphs()";
            graphgrid->redrawAllGraphs();
            DEBUG_OUT() << "MainWindow: redrawAllGraphs() completed";
        } else {
            DEBUG_OUT() << "MainWindow: Cannot redraw graphs - graphgrid is null";
        }
    });

    // Create Simulator instance
    simulator = new Simulator(this, timeUpdateTimer, graphgrid);

    // inside MainWindow constructor
    std::srand(std::time(nullptr));

    this->simTick = 0;
    connect(timer, &QTimer::timeout, this, &MainWindow::updateSimulation);
    timer->start(1000); // 1000ms = 1 second

    // Set up timer for time updates (every 1 second) - this will be passed to GraphLayout
    timeUpdateTimer->setInterval(1000); // 1000ms = 1 second
    timeUpdateTimer->start();

    // Start the simulator
    simulator->start();

    // Test shaded region API in first tab
    // Wait a bit for the graph to initialize, then add a test shaded region
    QTimer::singleShot(2000, this, [this]() {
        if (graphgrid) {
            // Find the first GraphContainer
            QList<GraphContainer*> containers = graphgrid->findChildren<GraphContainer*>();
            if (!containers.isEmpty()) {
                GraphContainer* firstContainer = containers.first();
                if (firstContainer) {
                    // Get the BTW graph from the container
                    WaterfallGraph* btwGraphBase = firstContainer->getWaterfallGraph(GraphType::BTW);
                    if (btwGraphBase) {
                        BTWGraph* btwGraph = qobject_cast<BTWGraph*>(btwGraphBase);
                        if (btwGraph) {
                            // Get current time for the test
                            QDateTime currentTime = QDateTime::currentDateTime();
                            // Add a test vertical shaded region with X range 30.0 to 40.0
                            // This will create a vertical band spanning from top to bottom
                            int regionId = btwGraph->addShadedRegion(30.0, 40.0, currentTime);
                            DEBUG_OUT() << "MainWindow: Test - Added vertical shaded region" << regionId 
                                     << "X range: 30.0 to 40.0, Y=" << currentTime.toString("yyyy-MM-dd hh:mm:ss.zzz");
                            
                            // Add another test region with different X range
                            QTimer::singleShot(1000, this, [btwGraph, currentTime]() {
                                int regionId2 = btwGraph->addShadedRegion(50.0, 60.0, currentTime);
                                DEBUG_OUT() << "MainWindow: Test - Added second vertical shaded region" << regionId2 
                                         << "X range: 50.0 to 60.0, Y=" << currentTime.toString("yyyy-MM-dd hh:mm:ss.zzz");
                            });
                        } else {
                            DEBUG_OUT() << "MainWindow: Test - Could not cast to BTWGraph";
                        }
                    } else {
                        DEBUG_OUT() << "MainWindow: Test - Could not get BTW graph from container";
                    }
                } else {
                    DEBUG_OUT() << "MainWindow: Test - First container is null";
                }
            } else {
                DEBUG_OUT() << "MainWindow: Test - No GraphContainers found";
            }
        }
    });

    // Test BTW blue markers - add a marker every 3 minutes
    // Wait for graph to initialize, then start adding markers
    QTimer::singleShot(3000, this, [this]() {
        if (graphgrid) {
            // Add an immediate test marker first
            QDateTime currentTime = QDateTime::currentDateTime();
            graphgrid->addBTWMarker(GraphType::BTW, currentTime, 40.0, 2.5);
            DEBUG_OUT() << "MainWindow: Test - Added initial BTW blue marker at" 
                     << currentTime.toString("yyyy-MM-dd hh:mm:ss.zzz")
                     << "range: 40.0, delta: 2.5";
            
            // Create a timer to add BTW markers every 3 minutes (180 seconds = 180000 ms)
            QTimer* btwMarkerTimer = new QTimer(this);
            connect(btwMarkerTimer, &QTimer::timeout, this, [this]() {
                if (graphgrid) {
                    QDateTime currentTime = QDateTime::currentDateTime();
                    // Add BTW marker with varying range values for testing
                    // Use a simple pattern: range alternates between 20.0 and 60.0
                    static bool useFirstRange = true;
                    qreal range = useFirstRange ? 20.0 : 60.0;
                    useFirstRange = !useFirstRange;
                    
                    // Delta value for the marker (affects the angle of the line)
                    // Alternate between positive and negative for visual variety
                    static bool usePositiveDelta = true;
                    qreal delta = usePositiveDelta ? 2.5 : -2.5;
                    usePositiveDelta = !usePositiveDelta;
                    
                    // Add the blue automatic marker
                    graphgrid->addBTWMarker(GraphType::BTW, currentTime, range, delta);
                    DEBUG_OUT() << "MainWindow: Test - Added BTW blue marker at" 
                             << currentTime.toString("yyyy-MM-dd hh:mm:ss.zzz")
                             << "range:" << range << "delta:" << delta;
                }
            });
            
            // Start the timer - first marker after 3 minutes, then every 3 minutes
            btwMarkerTimer->start(180000); // 180000 ms = 3 minutes
            
            DEBUG_OUT() << "MainWindow: Test - Started BTW blue marker timer (every 3 minutes)";
        }
    });

    // Test RTW R markers - add a marker every 3 minutes
    // Wait for graph to initialize, then start adding markers
    QTimer::singleShot(4000, this, [this]() {
        if (graphgrid) {
            // Add an immediate test R marker first
            QDateTime currentTime = QDateTime::currentDateTime();
            graphgrid->addRTWRMarker(GraphType::RTW, currentTime, 45.0);
            DEBUG_OUT() << "MainWindow: Test - Added initial RTW R marker at" 
                     << currentTime.toString("yyyy-MM-dd hh:mm:ss.zzz")
                     << "range: 45.0";
            
            // Create a timer to add RTW R markers every 3 minutes (180 seconds = 180000 ms)
            QTimer* rtwMarkerTimer = new QTimer(this);
            connect(rtwMarkerTimer, &QTimer::timeout, this, [this]() {
                if (graphgrid) {
                    QDateTime currentTime = QDateTime::currentDateTime();
                    // Add RTW R marker with varying range values for testing
                    // Use a simple pattern: range alternates between 30.0 and 60.0
                    static bool useFirstRange = true;
                    qreal range = useFirstRange ? 30.0 : 60.0;
                    useFirstRange = !useFirstRange;
                    
                    // Add the R marker
                    graphgrid->addRTWRMarker(GraphType::RTW, currentTime, range);
                    DEBUG_OUT() << "MainWindow: Test - Added RTW R marker at" 
                             << currentTime.toString("yyyy-MM-dd hh:mm:ss.zzz")
                             << "range:" << range;
                }
            });
            
            // Start the timer - first marker after 3 minutes, then every 3 minutes
            rtwMarkerTimer->start(180000); // 180000 ms = 3 minutes
            
            DEBUG_OUT() << "MainWindow: Test - Started RTW R marker timer (every 3 minutes)";
        }
    });

    // Initialize some sample data for the graph
    std::vector<double> x_data = {0.0, 1.0, 2.0, 3.0, 4.0};
    std::vector<double> y1_data = {0.0, 2.0, 4.0, 6.0, 8.0};  // Linear growth
    std::vector<double> y2_data = {0.0, 1.0, 4.0, 9.0, 16.0}; // Quadratic growth

    // Set the data to the graph widget
    ui->widget->setData(x_data, y1_data, y2_data);
    ui->widget->setAxesLabels("Time (s)", "Speed (m/s)", "Distance (m)");

    // Our ship
    this->currentShipSpeed = 30;
    this->currentOwnShipBearing = 90; //  Nautical degrees

    this->currentSensorBearing = 250;

    // selected track
    this->currentSelectedTrackSpeed = 30;
    this->currentSelectedTrackRange = 9;
    this->currentSelectedTrackBearing = 200;
    this->currentSelectedTrackCourse = 180;

    // adopted track
    this->currentAdoptedTrackSpeed = 30;
    this->currentAdoptedTrackRange = 10;
    this->currentAdoptedTrackBearing = 300;
    this->currentAdoptedTrackCourse = 270;

    // Set hard limits for all graph types using GraphLayout range limit methods
    // Range limits are set to be 2x larger than the simulation ranges
    // FDW: sim range 8.0-30.0, so limits 8.0-(8.0+2*(30.0-8.0)) = 8.0-52.0
    graphgrid->setHardRangeLimits(GraphType::FDW, -35.0, 35.0);  // Frequency Domain Window4
    // BDW: sim range 5.0-38.0, so limits 5.0-(5.0+2*(38.0-5.0)) = 5.0-71.0
    graphgrid->setHardRangeLimits(GraphType::BDW, -35.0, 35.0);  // Bandwidth Domain Window
    // BRW: sim range 8.0-30.0, so limits 8.0-52.0
    graphgrid->setHardRangeLimits(GraphType::BRW, -35.0, 35.0);  // Bit Rate Window
    // LTW: sim range 15.0-30.0, so limits 15.0-(15.0+2*(30.0-15.0)) = 15.0-45.0
    graphgrid->setHardRangeLimits(GraphType::LTW, 15.0, 45.0);  // Left Track Window
    // BTW: sim range 5.0-40.0, so limits 5.0-(5.0+2*(40.0-5.0)) = 5.0-75.0
    graphgrid->setHardRangeLimits(GraphType::BTW, 5.0, 75.0);   // Bottom Track Window
    // RTW: sim range 0.0-25.0, so limits 0.0-(0.0+2*(25.0-0.0)) = 0.0-50.0
    graphgrid->setHardRangeLimits(GraphType::RTW, 0.0, 50.0);  // Right Track Window
    // FTW: sim range 15.0-30.0, so limits 15.0-45.0
    // graphgrid->setHardRangeLimits(GraphType::FTW, 15.0, 45.0);  // Frequency Time Window
    graphgrid->setHardRangeLimits(GraphType::FTW, -40.0, 40.0);  // Frequency Time Window

    // TSW (Tactical Solution View) disabled
    // ui->tsv->setData(
    //     this->currentShipSpeed,
    //     this->currentOwnShipBearing,
    //     this->currentSensorBearing,
    //     this->currentAdoptedTrackRange,
    //     this->currentAdoptedTrackSpeed,
    //     this->currentAdoptedTrackBearing,
    //     this->currentSelectedTrackRange,
    //     this->currentSelectedTrackSpeed,
    //     this->currentSelectedTrackBearing,
    //     this->currentAdoptedTrackCourse,
    //     this->currentSelectedTrackCourse);

    // Configure layout selection combobox
    configureLayoutSelection();

    // Setup custom graphs tab
    setupCustomGraphsTab();
    
    // Setup test WaterfallGraph in controls tab for crosshair testing
    setupTestWaterfallGraph();

    // Setup TimelineView in controls tab for slider testing
    setupTimelineView();

    // Setup RTW Symbols test
    setupRTWSymbolsTest();

    // Setup SCWWindow in a new tab
    setupSCWWindow();
    
    // Hide all tabs except first tab (Original View) and SCW Window for memory leak testing
    int scwTabIndex = -1;
    for (int i = 0; i < ui->tabWidget->count(); ++i) {
        QString tabText = ui->tabWidget->tabText(i);
        if (tabText == "SCW Window") {
            scwTabIndex = i;
            // Keep SCW tab visible
            // ui->tabWidget->setTabVisible(i, true);
        } else if (i == 0) {
            // Keep first tab (Original View) visible
            // ui->tabWidget->setTabVisible(i, true);
        } else {
            // Hide all other tabs
            // ui->tabWidget->setTabVisible(i, false);
        }
    }
    
    // Connect BTW symbol signal to SCWWindow (if it exists)
    // This will add magenta circles to SCW graphs whenever BTW markers are placed
    connect(graphgrid, &GraphLayout::BTWSymbolAddedToAllGraphs,
            [this](const QDateTime &timestamp) {
                if (scwWindow && timestamp.isValid()) {
                    scwWindow->addBTWSymbolToAllGraphs(timestamp);
                }
            });


    // Configure Zoom Panel test functionality
    configureZoomPanel();
    
    // Setup time selection history storage
    setupTimeSelectionHistory();

    // Setup manoeuvre button
    setupManoeuvreButton();

    // Add a test RTW symbol to the graph layout
    QDateTime testSymbolTime = QDateTime::currentDateTime().addSecs(-120); // 2 minutes ago
    graphgrid->addRTWSymbol(GraphType::RTW, "TM", testSymbolTime, 15.0);
    DEBUG_OUT() << "MainWindow: Added test RTW symbol 'TM' at timestamp:" << testSymbolTime.toString("yyyy-MM-dd hh:mm:ss");
}

void MainWindow::setupTimeSelectionHistory()
{
    // Connect time selection signal to store timestamps
    connect(graphgrid, &GraphLayout::TimeSelectionCreated,
            this, &MainWindow::onTimeSelectionCreated);
    
    DEBUG_OUT() << "Time selection history storage initialized (max 5 selections)";
}

void MainWindow::setupManoeuvreButton()
{
    // Find the controls widget and its layout (below the layout combo box)
    QWidget* controlsWidget = ui->originalTab->findChild<QWidget*>("controlsWidget");
    if (!controlsWidget) {
        qWarning() << "Controls widget not found, creating new one";
        controlsWidget = new QWidget(ui->originalTab);
        controlsWidget->setObjectName("controlsWidget");
        controlsWidget->setGeometry(QRect(1100, 70, 300, 400));
    }
    
    // Get or create the vertical layout
    QVBoxLayout* controlsLayout = qobject_cast<QVBoxLayout*>(controlsWidget->layout());
    if (!controlsLayout) {
        controlsLayout = new QVBoxLayout(controlsWidget);
        controlsLayout->setContentsMargins(0, 0, 0, 0);
        controlsLayout->setSpacing(10);
    }
    
    // Create button to add manoeuvres
    addManoeuvreButton = new QPushButton("Add Manoeuvre", controlsWidget);
    addManoeuvreButton->setObjectName("addManoeuvreButton");
    addManoeuvreButton->setFixedSize(150, 30);
    
    // Connect button click to slot
    connect(addManoeuvreButton, &QPushButton::clicked, this, &MainWindow::onAddManoeuvreButtonClicked);
    
    // Create button to clear manoeuvres
    clearManoeuvresButton = new QPushButton("Clear Manoeuvres", controlsWidget);
    clearManoeuvresButton->setObjectName("clearManoeuvresButton");
    clearManoeuvresButton->setFixedSize(150, 30);
    
    // Connect button click to slot
    connect(clearManoeuvresButton, &QPushButton::clicked, this, &MainWindow::onClearManoeuvresButtonClicked);
    
    // Create button to start manoeuvre drawing (new API)
    startManoeuvreButton = new QPushButton("Start Manoeuvre", controlsWidget);
    startManoeuvreButton->setObjectName("startManoeuvreButton");
    startManoeuvreButton->setFixedSize(150, 30);
    startManoeuvreButton->setStyleSheet("QPushButton { background-color: #28a745; color: white; font-weight: bold; }");
    
    // Connect button click to slot
    connect(startManoeuvreButton, &QPushButton::clicked, this, &MainWindow::onStartManoeuvreButtonClicked);
    
    // Create button to end manoeuvre drawing (new API)
    endManoeuvreButton = new QPushButton("End Manoeuvre", controlsWidget);
    endManoeuvreButton->setObjectName("endManoeuvreButton");
    endManoeuvreButton->setFixedSize(150, 30);
    endManoeuvreButton->setStyleSheet("QPushButton { background-color: #dc3545; color: white; font-weight: bold; }");
    
    // Connect button click to slot
    connect(endManoeuvreButton, &QPushButton::clicked, this, &MainWindow::onEndManoeuvreButtonClicked);
    
    // Create button to toggle BTW horizontal line mode
    btwLineModeButton = new QPushButton("BTW Mode: Normal", controlsWidget);
    btwLineModeButton->setObjectName("btwLineModeButton");
    btwLineModeButton->setFixedSize(200, 30);
    updateBTWLineModeButton();
    
    // Connect button click to slot
    connect(btwLineModeButton, &QPushButton::clicked, this, &MainWindow::onBTWLineModeButtonClicked);
    
    // Add buttons to the layout (vertically, one by one)
    controlsLayout->addWidget(addManoeuvreButton);
    controlsLayout->addWidget(clearManoeuvresButton);
    controlsLayout->addWidget(startManoeuvreButton);
    controlsLayout->addWidget(endManoeuvreButton);
    controlsLayout->addWidget(btwLineModeButton);
    controlsLayout->addStretch(); // Add stretch to push everything to the top
    
    DEBUG_OUT() << "Manoeuvre buttons created and connected in vertical layout below combo box";
}

void MainWindow::onAddManoeuvreButtonClicked()
{
    // Create a sample manoeuvre with dummy data
    QDateTime now = QDateTime::currentDateTime();
    QDateTime startTime = now.addSecs(-180); // 3 minutes ago
    QDateTime endTime = now;                  // Current time
    
    // Generate dummy data: bearing (0-360), speed (10-30), depth (50-200)
    int bearing = (std::rand() % 360);      // Random bearing 0-359
    int speed = 10 + (std::rand() % 21);     // Random speed 10-30
    int depth = 50 + (std::rand() % 151);    // Random depth 50-200
    
    Manoeuvre manoeuvre(startTime, endTime, bearing, speed, depth);
    
    // Add manoeuvre to graph layout
    graphgrid->addManoeuvre(manoeuvre);
    
    DEBUG_OUT() << "MainWindow: Added manoeuvre - startTime:" << startTime.toString("yyyy-MM-dd hh:mm:ss")
             << "endTime:" << endTime.toString("yyyy-MM-dd hh:mm:ss")
             << "bearing:" << manoeuvre.bearing
             << "speed:" << manoeuvre.speed
             << "depth:" << manoeuvre.depth;
}

void MainWindow::onClearManoeuvresButtonClicked()
{
    // Clear all manoeuvres from graph layout
    graphgrid->clearManoeuvres();
    
    DEBUG_OUT() << "MainWindow: Cleared all manoeuvres";
}

void MainWindow::onStartManoeuvreButtonClicked()
{
    // Start a manoeuvre drawing session with current time and random parameters
    QDateTime startTime = QDateTime::currentDateTime();
    
    // Generate dummy data: bearing (0-360), speed (10-30), depth (50-200)
    int bearing = (std::rand() % 360);      // Random bearing 0-359
    int speed = 10 + (std::rand() % 21);     // Random speed 10-30
    int depth = 50 + (std::rand() % 151);    // Random depth 50-200
    
    // Call the new API to start manoeuvre drawing
    graphgrid->startManoeuvreDrawing(startTime, bearing, speed, depth);
    
    DEBUG_OUT() << "MainWindow: Started manoeuvre drawing - startTime:" << startTime.toString("yyyy-MM-dd hh:mm:ss")
             << "bearing:" << bearing
             << "speed:" << speed
             << "depth:" << depth;
}

void MainWindow::onEndManoeuvreButtonClicked()
{
    // End the current manoeuvre drawing session with current time
    QDateTime endTime = QDateTime::currentDateTime();
    
    // Call the new API to end manoeuvre drawing
    graphgrid->endManoeuvreDrawing(endTime);
    
    DEBUG_OUT() << "MainWindow: Ended manoeuvre drawing - endTime:" << endTime.toString("yyyy-MM-dd hh:mm:ss");
    
    // Show current manoeuvre count
    auto manoeuvres = graphgrid->getManoeuvres();
    DEBUG_OUT() << "MainWindow: Total manoeuvres:" << manoeuvres.size();
}

void MainWindow::onTimeSelectionCreated(const TimeSelectionSpan &selection)
{
    DEBUG_OUT() << "MainWindow: Time selection created - start:" << selection.startTime.toString("yyyy-MM-dd hh:mm:ss.zzz")
             << "end:" << selection.endTime.toString("yyyy-MM-dd hh:mm:ss.zzz");
    
    // Store the selection timestamps (both start and end)
    // If we already have 5, remove the oldest (FIFO)
    if (timeSelectionHistory.size() >= 5)
    {
        timeSelectionHistory.erase(timeSelectionHistory.begin());
        DEBUG_OUT() << "Time selection history full, removed oldest entry";
    }
    
    // Add the new selection to the end
    timeSelectionHistory.push_back(selection);
    
    DEBUG_OUT() << "Time selection stored. History size:" << timeSelectionHistory.size();
    DEBUG_OUT() << "All stored selections:";
    for (size_t i = 0; i < timeSelectionHistory.size(); ++i)
    {
        DEBUG_OUT() << "  [" << i << "] Start:" << timeSelectionHistory[i].startTime.toString("yyyy-MM-dd hh:mm:ss.zzz")
                 << "End:" << timeSelectionHistory[i].endTime.toString("yyyy-MM-dd hh:mm:ss.zzz");
    }
}

void MainWindow::onBTWLineModeButtonClicked()
{
    // Cycle through the 3 modes: Normal -> DrawLine -> DeleteLine -> Normal
    switch (m_currentBTWLineMode)
    {
        case BTWGraph::HorizontalLineMode::Normal:
            m_currentBTWLineMode = BTWGraph::HorizontalLineMode::DrawLine;
            break;
        case BTWGraph::HorizontalLineMode::DrawLine:
            m_currentBTWLineMode = BTWGraph::HorizontalLineMode::DeleteLine;
            break;
        case BTWGraph::HorizontalLineMode::DeleteLine:
            m_currentBTWLineMode = BTWGraph::HorizontalLineMode::Normal;
            break;
    }
    
    // Apply the mode to GraphLayout
    if (graphgrid)
    {
        graphgrid->setBTWHorizontalLineMode(GraphType::BTW, m_currentBTWLineMode);
        DEBUG_OUT() << "MainWindow: BTW line mode changed to" << static_cast<int>(m_currentBTWLineMode);
    }
    
    // Update button text and style
    updateBTWLineModeButton();
}

void MainWindow::updateBTWLineModeButton()
{
    if (!btwLineModeButton)
        return;
    
    QString buttonText;
    QString buttonStyle;
    
    switch (m_currentBTWLineMode)
    {
        case BTWGraph::HorizontalLineMode::Normal:
            buttonText = "BTW Mode: Normal";
            buttonStyle = "QPushButton { background-color: #6c757d; color: white; font-weight: bold; }";
            break;
        case BTWGraph::HorizontalLineMode::DrawLine:
            buttonText = "BTW Mode: Draw Line";
            buttonStyle = "QPushButton { background-color: #007bff; color: white; font-weight: bold; }";
            break;
        case BTWGraph::HorizontalLineMode::DeleteLine:
            buttonText = "BTW Mode: Delete Line";
            buttonStyle = "QPushButton { background-color: #dc3545; color: white; font-weight: bold; }";
            break;
    }
    
    btwLineModeButton->setText(buttonText);
    btwLineModeButton->setStyleSheet(buttonStyle);
}

MainWindow::~MainWindow()
{
    // Clean up WaterfallData objects
    delete fdwData;
    delete bdwData;
    delete brwData;
    delete ltwData;
    delete btwData;
    delete rtwData;
    delete ftwData;
    delete testWaterfallData;
    
    delete ui;
}

/**
 * @brief Simulation update slot - called every timer interval
 *
 * Updates target position, recalculates bearing/range/rate,
 * and triggers widget repaint. This is the main simulation loop.
 */
void MainWindow::updateSimulation()
{
    this->simTick++;

    auto randPercent = [](int max)
    {
        return static_cast<double>(std::rand() % max) / 100.0; // 0.00 .. (max-1)/100
    };

    // --- Own ship (small variations <10%) ---
    this->currentShipSpeed *= (0.95 + randPercent(10));   // ±5%
    this->currentOwnShipBearing += (std::rand() % 7) - 3; // jitter ±3°

    // --- Adopted track (small variations <10%) ---
    this->currentAdoptedTrackSpeed *= (0.95 + randPercent(10)); // ±5%
    this->currentAdoptedTrackRange *= (0.95 + randPercent(10)); // ±5%
    this->currentAdoptedTrackBearing += (std::rand() % 9) - 4;  // jitter ±4°
    this->currentAdoptedTrackCourse += (std::rand() % 7) - 3;   // jitter ±3°

    // --- Selected track (large variations >30%) ---
    this->currentSelectedTrackSpeed *= (0.7 + randPercent(60));   // 0.7–1.29 → ±30%
    this->currentSelectedTrackRange *= (0.7 + randPercent(60));   // ±30%
    this->currentSelectedTrackBearing += (std::rand() % 91) - 45; // swing ±45°
    this->currentSelectedTrackCourse += (std::rand() % 91) - 45;  // swing ±45°

    // Normalize bearings and courses to [0, 360)
    this->currentOwnShipBearing = fmod(this->currentOwnShipBearing + 360.0, 360.0);
    this->currentAdoptedTrackBearing = fmod(this->currentAdoptedTrackBearing + 360.0, 360.0);
    this->currentSelectedTrackBearing = fmod(this->currentSelectedTrackBearing + 360.0, 360.0);
    this->currentAdoptedTrackCourse = fmod(this->currentAdoptedTrackCourse + 360.0, 360.0);
    this->currentSelectedTrackCourse = fmod(this->currentSelectedTrackCourse + 360.0, 360.0);

    // Push updated values into TacticalSolutionView (DISABLED)
    // ui->tsv->setData(
    //     this->currentShipSpeed,
    //     this->currentOwnShipBearing,
    //     this->currentSensorBearing,
    //     this->currentAdoptedTrackRange,
    //     this->currentAdoptedTrackSpeed,
    //     this->currentAdoptedTrackBearing,
    //     this->currentSelectedTrackRange,
    //     this->currentSelectedTrackSpeed,
    //     this->currentSelectedTrackBearing,
    //     this->currentAdoptedTrackCourse,
    //     this->currentSelectedTrackCourse);

    // set the current time to the system time
    graphgrid->setCurrentTime(QTime::currentTime());

    // Set chevron labels to demonstrate functionality
    graphgrid->setChevronLabel1("Start");
    graphgrid->setChevronLabel2("Now");
    graphgrid->setChevronLabel3("End");
}

void MainWindow::configureZoomPanel()
{
    // Initialize label values
    ui->zoomPanel->setLeftLabelValue(0.0);   // Left reference value
    ui->zoomPanel->setCenterLabelValue(50.0); // Center value
    ui->zoomPanel->setRightLabelValue(100.0);  // Range for upper bound

    // Initialize the indicator value label
    ui->indicatorValueLabel->setText("Bounds: [0.35, 0.80]");

    // Connect to zoom panel value changed signal
    connect(ui->zoomPanel, &ZoomPanel::valueChanged, [this](ZoomBounds bounds)
            {
                // Update the indicator value label on main window with bounds
                ui->indicatorValueLabel->setText(QString("Bounds: [%1, %2]").arg(bounds.lowerbound, 0, 'f', 2).arg(bounds.upperbound, 0, 'f', 2));

                // You can add additional logic here to respond to value changes
                // For example, update other UI elements or trigger other actions
            });
}

void MainWindow::configureLayoutSelection()
{
    // Populate combobox with layout type options
    ui->layoutSelectionComboBox->addItem("1 Window", static_cast<int>(LayoutType::GPW1W));
    ui->layoutSelectionComboBox->addItem("4 Windows (2x2)", static_cast<int>(LayoutType::GPW4W));
    ui->layoutSelectionComboBox->addItem("2 Windows Vertical", static_cast<int>(LayoutType::GPW2WV));
    ui->layoutSelectionComboBox->addItem("2 Windows Horizontal", static_cast<int>(LayoutType::GPW2WH));
    ui->layoutSelectionComboBox->addItem("4 Windows Horizontal", static_cast<int>(LayoutType::GPW4WH));
    ui->layoutSelectionComboBox->addItem("2 Windows Horizontal (no GPW)", static_cast<int>(LayoutType::NOGPW2WH));
    ui->layoutSelectionComboBox->addItem("Hidden", static_cast<int>(LayoutType::HIDDEN));

    // Set default selection to 1 Window
    ui->layoutSelectionComboBox->setCurrentIndex(5);

    // Connect combobox to slot
    connect(ui->layoutSelectionComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onLayoutTypeChanged);
}

void MainWindow::onLayoutTypeChanged(int index)
{
    LayoutType layoutType = static_cast<LayoutType>(ui->layoutSelectionComboBox->itemData(index).toInt());
    graphgrid->setLayoutType(layoutType);
}

/**
 * @brief Setup test WaterfallGraph in controls tab for crosshair testing
 */
void MainWindow::setupTestWaterfallGraph()
{
    DEBUG_OUT() << "=== Setting up Test WaterfallGraph in Controls Tab ===";
    
    // Create WaterfallData for test
    testWaterfallData = new WaterfallData("TEST", {"TEST-1", "ADOPTED"});
    
    // Create test WaterfallGraph in the controls tab
    testWaterfallGraph = new WaterfallGraph(ui->controlsTab, true, 8, TimeInterval::FifteenMinutes);
    testWaterfallGraph->setObjectName("testWaterfallGraph");
    testWaterfallGraph->setGeometry(QRect(500, 10, 400, 500));
    testWaterfallGraph->setDataSource(*testWaterfallData);
    
    // Set series colors
    testWaterfallGraph->setSeriesColor("TEST-1", QColor(Qt::red));
    testWaterfallGraph->setSeriesColor("ADOPTED", QColor(Qt::yellow));
    
    // Enable crosshair (already enabled by default, but explicitly set)
    testWaterfallGraph->setCrosshairEnabled(true);
    
    // Add some test data
    QDateTime baseTime = QDateTime::currentDateTime();
    for (int i = 0; i < 20; i++)
    {
        QDateTime timestamp = baseTime.addSecs(-i * 10); // 10 seconds apart
        qreal value = 0.3 + 0.4 * (i / 20.0) + 0.1 * std::sin(i * 0.5); // Varying values
        testWaterfallGraph->addDataPoint("TEST-1", static_cast<float>(value), timestamp);
        
        qreal adoptedValue = 0.5 + 0.2 * std::cos(i * 0.3);
        testWaterfallGraph->addDataPoint("ADOPTED", static_cast<float>(adoptedValue), timestamp);
    }
    
    DEBUG_OUT() << "Test WaterfallGraph created in controls tab with" 
             << testWaterfallData->getDataSeriesSize("TEST-1") << "test data points";
    DEBUG_OUT() << "Crosshair enabled:" << testWaterfallGraph->isCrosshairEnabled();
    DEBUG_OUT() << "Test WaterfallGraph geometry:" << testWaterfallGraph->geometry();
    DEBUG_OUT() << "Test WaterfallGraph visible:" << testWaterfallGraph->isVisible();
}

/**
 * @brief Setup TimelineView in a dedicated timelineview tab
 */
void MainWindow::setupTimelineView()
{
    DEBUG_OUT() << "=== Setting up TimelineView Tab ===";
    
    // Create a new tab for TimelineView
    QWidget* timelineViewTab = new QWidget();
    timelineViewTab->setObjectName("timelineViewTab");
    ui->tabWidget->addTab(timelineViewTab, "Timeline View");
    
    // Create TimelineView in the new tab
    testTimelineView = new TimelineView(timelineViewTab, timeUpdateTimer);
    testTimelineView->setObjectName("testTimelineView");
    testTimelineView->setGeometry(QRect(50, 50, 80, 600));
    
    // Create labels for monitoring timespan changes
    QLabel* titleLabel = new QLabel("Timeline Slider Control", timelineViewTab);
    titleLabel->setGeometry(QRect(150, 50, 300, 30));
    titleLabel->setStyleSheet("QLabel { color: white; font-size: 16px; font-weight: bold; background-color: rgba(0, 0, 0, 150); padding: 6px; border-radius: 4px; }");
    
    QLabel* startLabel = new QLabel("Start Time:", timelineViewTab);
    startLabel->setGeometry(QRect(150, 100, 120, 25));
    startLabel->setStyleSheet("QLabel { color: white; font-size: 13px; font-weight: bold; }");
    
    timespanStartLabel = new QLabel("--:--:--", timelineViewTab);
    timespanStartLabel->setGeometry(QRect(280, 100, 200, 25));
    timespanStartLabel->setStyleSheet("QLabel { color: yellow; font-size: 13px; font-weight: bold; background-color: rgba(0, 0, 0, 200); padding: 4px; border: 1px solid gray; border-radius: 3px; }");
    
    QLabel* endLabel = new QLabel("End Time:", timelineViewTab);
    endLabel->setGeometry(QRect(150, 135, 120, 25));
    endLabel->setStyleSheet("QLabel { color: white; font-size: 13px; font-weight: bold; }");
    
    timespanEndLabel = new QLabel("--:--:--", timelineViewTab);
    timespanEndLabel->setGeometry(QRect(280, 135, 200, 25));
    timespanEndLabel->setStyleSheet("QLabel { color: yellow; font-size: 13px; font-weight: bold; background-color: rgba(0, 0, 0, 200); padding: 4px; border: 1px solid gray; border-radius: 3px; }");
    
    QLabel* durationLabel = new QLabel("Duration:", timelineViewTab);
    durationLabel->setGeometry(QRect(150, 170, 120, 25));
    durationLabel->setStyleSheet("QLabel { color: white; font-size: 13px; font-weight: bold; }");
    
    timespanDurationLabel = new QLabel("--:--:--", timelineViewTab);
    timespanDurationLabel->setGeometry(QRect(280, 170, 200, 25));
    timespanDurationLabel->setStyleSheet("QLabel { color: cyan; font-size: 13px; font-weight: bold; background-color: rgba(0, 0, 0, 200); padding: 4px; border: 1px solid gray; border-radius: 3px; }");
    
    QLabel* modeTitleLabel = new QLabel("Mode:", timelineViewTab);
    modeTitleLabel->setGeometry(QRect(150, 205, 120, 25));
    modeTitleLabel->setStyleSheet("QLabel { color: white; font-size: 13px; font-weight: bold; }");
    
    timelineModeLabel = new QLabel("FOLLOW_MODE", timelineViewTab);
    timelineModeLabel->setGeometry(QRect(280, 205, 200, 25));
    timelineModeLabel->setStyleSheet("QLabel { color: lime; font-size: 13px; font-weight: bold; background-color: rgba(0, 0, 0, 200); padding: 4px; border: 2px solid lime; border-radius: 3px; }");
    
    // Connect mode change signal to update mode label
    connect(testTimelineView, &TimelineView::GraphContainerInFollowModeChanged,
            [this](bool isInFollowMode) {
                if (timelineModeLabel) {
                    if (isInFollowMode) {
                        timelineModeLabel->setText("FOLLOW_MODE");
                        timelineModeLabel->setStyleSheet("QLabel { color: lime; font-size: 13px; font-weight: bold; background-color: rgba(0, 0, 0, 200); padding: 4px; border: 2px solid lime; border-radius: 3px; }");
                    } else {
                        timelineModeLabel->setText("FROZEN_MODE");
                        timelineModeLabel->setStyleSheet("QLabel { color: red; font-size: 13px; font-weight: bold; background-color: rgba(0, 0, 0, 200); padding: 4px; border: 2px solid red; border-radius: 3px; }");
                    }
                }
            });
    
    // Add instructions label
    QLabel* instructionsLabel = new QLabel(
        "Instructions:\n"
        "• Drag the white rectangle in the slider to change the visible time window\n"
        "• The slider represents the last 12 hours\n"
        "• The white rectangle size is proportional to the selected time interval\n"
        "• Use the interval button (dt:) to change the time interval",
        timelineViewTab);
    instructionsLabel->setGeometry(QRect(150, 220, 500, 150));
    instructionsLabel->setStyleSheet("QLabel { color: lightgray; font-size: 12px; background-color: rgba(0, 0, 0, 100); padding: 10px; border: 1px solid gray; border-radius: 4px; }");
    instructionsLabel->setWordWrap(true);
    
    // Connect TimeScopeChanged signal to update labels
    connect(testTimelineView, &TimelineView::TimeScopeChanged, 
            [this](const TimeSelectionSpan& selection) {
                if (selection.startTime.isValid() && selection.endTime.isValid()) {
                    // Update start time label (QDateTime formatting)
                    timespanStartLabel->setText(selection.startTime.toString("HH:mm:ss"));
                    
                    // Update end time label (QDateTime formatting)
                    timespanEndLabel->setText(selection.endTime.toString("HH:mm:ss"));
                    
                    // Calculate and display duration using msecsTo
                    int durationMs = selection.startTime.msecsTo(selection.endTime);
                    if (durationMs < 0) {
                        durationMs = -durationMs; // Use absolute value
                    }
                    int durationSeconds = durationMs / 1000;
                    
                    QTime duration = QTime(0, 0, 0).addSecs(durationSeconds);
                    timespanDurationLabel->setText(duration.toString("HH:mm:ss"));
                    
                    DEBUG_OUT() << "TimeScopeChanged - Start:" << selection.startTime.toString("HH:mm:ss")
                             << "End:" << selection.endTime.toString("HH:mm:ss")
                             << "Duration:" << duration.toString("HH:mm:ss");
                }
            });
    
    DEBUG_OUT() << "TimelineView created in dedicated Timeline View tab";
    DEBUG_OUT() << "TimelineView geometry:" << testTimelineView->geometry();
    DEBUG_OUT() << "TimelineView visible:" << testTimelineView->isVisible();
}

/**
 * @brief Setup SCWWindow in a new tab
 */
void MainWindow::setupSCWWindow()
{
    DEBUG_OUT() << "=== Setting up SCWWindow Tab ===";
    
    // Create a new tab for SCWWindow
    QWidget* scwTab = new QWidget();
    scwTab->setObjectName("scwTab");
    
    // Create a layout for the tab to ensure SCWWindow fills the entire tab
    QVBoxLayout* tabLayout = new QVBoxLayout(scwTab);
    tabLayout->setContentsMargins(0, 0, 0, 0);
    tabLayout->setSpacing(0);
    
    // Get sync state from graphlayout for timeline synchronization
    GraphContainerSyncState *syncState = graphgrid ? graphgrid->getSyncState() : nullptr;
    
    // Create SCWWindow in the new tab with sync state
    scwWindow = new SCWWindow(scwTab, timeUpdateTimer, syncState);
    scwWindow->setObjectName("scwWindow");
    
    // Set size policy to allow SCWWindow to expand and fill available space
    scwWindow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Add SCWWindow to the tab layout
    tabLayout->addWidget(scwWindow);
    
    // Set the layout on the tab
    scwTab->setLayout(tabLayout);
    
    // Add the tab to the tab widget
    ui->tabWidget->addTab(scwTab, "SCW Window");
    
    // Sync SCW timeline view with GraphLayout timeline views
    if (graphgrid && scwWindow->getTimelineView())
    {
        graphgrid->syncExternalTimelineView(scwWindow->getTimelineView());
    }
    
    // Create SCWSimulator to populate graphs every second
    // Use the same timeUpdateTimer which fires every second
    scwSimulator = new SCWSimulator(this, timeUpdateTimer, scwWindow);
    scwSimulator->start();
    
    // Connect BTW symbol signal to SCWWindow for magenta circles
    if (graphgrid && scwWindow)
    {
        connect(graphgrid, &GraphLayout::BTWSymbolAddedToAllGraphs,
                scwWindow, &SCWWindow::addBTWSymbolToAllGraphs, Qt::UniqueConnection);
        DEBUG_OUT() << "MainWindow: Connected BTWSymbolAddedToAllGraphs signal to SCWWindow";
    }
    
    DEBUG_OUT() << "SCWWindow created in SCW Window tab";
    DEBUG_OUT() << "SCWWindow size policy:" << scwWindow->sizePolicy();
    DEBUG_OUT() << "SCWSimulator created and started";
}

void MainWindow::setupCustomGraphsTab()
{
    DEBUG_OUT() << "=== Setting up Custom Graph Components Tab ===";

    // Create a horizontal layout for the new graph components tab
    QHBoxLayout *horizontalLayout = new QHBoxLayout(ui->customGraphsTab);
    horizontalLayout->setSpacing(5);
    horizontalLayout->setContentsMargins(10, 10, 10, 10);

    // Create WaterfallData objects as member variables
    fdwData = new WaterfallData("FDW", {"FDW-1", "FDW-2", "ADOPTED"});
    bdwData = new WaterfallData("BDW", {"BDW-1", "BDW-2", "ADOPTED"});
    brwData = new WaterfallData("BRW", {"BRW-1", "BRW-2", "ADOPTED"});
    ltwData = new WaterfallData("LTW", {"LTW-1", "LTW-2", "ADOPTED"});
    btwData = new WaterfallData("BTW", {"BTW-1", "BTW-2", "BTW-3", "ADOPTED"});
    rtwData = new WaterfallData("RTW", {"RTW-1", "RTW-2", "ADOPTED"});
    ftwData = new WaterfallData("FTW", {"FTW-1", "FTW-2", "ADOPTED"});

    // Create all 7 new graph components

    // FDW Graph - Frequency Domain Waterfall
    fdwGraph = new FDWGraph(ui->customGraphsTab, false, 8, TimeInterval::FifteenMinutes);
    fdwGraph->setObjectName("fdwGraph");
    fdwGraph->setDataSource(*fdwData); // Connect to data source
    
    // Set series colors for FDW graph
    fdwGraph->setSeriesColor("FDW-1", QColor(Qt::red));
    fdwGraph->setSeriesColor("FDW-2", QColor(Qt::green));
    fdwGraph->setSeriesColor("ADOPTED", QColor(Qt::yellow));
    DEBUG_OUT() << "FDW Graph connected to data source and colors set";

    // BDW Graph - Bandwidth Domain Waterfall
    bdwGraph = new BDWGraph(ui->customGraphsTab, false, 8, TimeInterval::FifteenMinutes);
    bdwGraph->setObjectName("bdwGraph");
    bdwGraph->setDataSource(*bdwData); // Connect to data source
    
    // Set series colors for BDW graph
    bdwGraph->setSeriesColor("BDW-1", QColor(Qt::red));
    bdwGraph->setSeriesColor("BDW-2", QColor(Qt::green));
    bdwGraph->setSeriesColor("ADOPTED", QColor(Qt::yellow));
    DEBUG_OUT() << "BDW Graph connected to data source and colors set";

    // BRW Graph - Bit Rate Waterfall
    brwGraph = new BRWGraph(ui->customGraphsTab, false, 8, TimeInterval::FifteenMinutes);
    brwGraph->setObjectName("brwGraph");
    brwGraph->setDataSource(*brwData); // Connect to data source
    
    // Set series colors for BRW graph
    brwGraph->setSeriesColor("BRW-1", QColor(Qt::green));
    brwGraph->setSeriesColor("BRW-2", QColor(Qt::blue));
    brwGraph->setSeriesColor("ADOPTED", QColor(Qt::yellow));
    DEBUG_OUT() << "BRW Graph connected to data source and colors set";

    // LTW Graph - Latency Time Waterfall
    ltwGraph = new LTWGraph(ui->customGraphsTab, false, 8, TimeInterval::FifteenMinutes);
    ltwGraph->setObjectName("ltwGraph");
    ltwGraph->setDataSource(*ltwData); // Connect to data source
    
    // Set series colors for LTW graph
    ltwGraph->setSeriesColor("LTW-1", QColor(Qt::red));
    ltwGraph->setSeriesColor("LTW-2", QColor(Qt::green));
    ltwGraph->setSeriesColor("ADOPTED", QColor(Qt::yellow));
    DEBUG_OUT() << "LTW Graph connected to data source and colors set";

    // BTW Graph - Bit Time Waterfall
    btwGraph = new BTWGraph(ui->customGraphsTab, false, 8, TimeInterval::FifteenMinutes);
    btwGraph->setObjectName("btwGraph");
    btwGraph->setDataSource(*btwData); // Connect to data source
    
    // Set series colors for BTW graph
    btwGraph->setSeriesColor("BTW-1", QColor(Qt::red));
    btwGraph->setSeriesColor("BTW-2", QColor(Qt::green));
    btwGraph->setSeriesColor("BTW-3", QColor(Qt::blue));
    btwGraph->setSeriesColor("ADOPTED", QColor(Qt::yellow));
    DEBUG_OUT() << "BTW Graph connected to data source and colors set";

    // RTW Graph - Rate Time Waterfall
    rtwGraph = new RTWGraph(ui->customGraphsTab, false, 8, TimeInterval::FifteenMinutes);
    rtwGraph->setObjectName("rtwGraph");
    rtwGraph->setDataSource(*rtwData); // Connect to data source
    
    // Set series colors for RTW graph
    rtwGraph->setSeriesColor("RTW-1", QColor(Qt::red));
    rtwGraph->setSeriesColor("RTW-2", QColor(Qt::green));
    rtwGraph->setSeriesColor("ADOPTED", QColor(Qt::yellow));
    DEBUG_OUT() << "RTW Graph connected to data source and colors set";
    
    // Test: Add RTW symbols to the overview tab (GraphLayout) RTW graph
    // The overview tab uses GraphLayout's data sources, not the standalone rtwData
    // Strategy: Temporarily set RTW as current in one container, get the graph, add symbols, then restore
    QTimer::singleShot(3000, this, [this]() {
        DEBUG_OUT() << "RTW: ===== Timer callback fired - attempting to add test symbols to overview tab =====";
        
        if (!graphgrid) {
            DEBUG_OUT() << "RTW: ERROR - graphgrid (GraphLayout) is null!";
            return;
        }
        
        // Get RTW data source from GraphLayout (this is what the overview tab uses)
        WaterfallData* overviewRTWData = graphgrid->getDataSource(GraphType::RTW);
        if (!overviewRTWData) {
            DEBUG_OUT() << "RTW: ERROR - GraphLayout RTW data source is null!";
            return;
        }
        
        DEBUG_OUT() << "RTW: GraphLayout RTW data source pointer:" << overviewRTWData;
        DEBUG_OUT() << "RTW: Current symbols in GraphLayout RTW data:" << overviewRTWData->getRTWSymbolsCount();
        
        // Find RTW graphs in the GraphLayout widget tree using Qt's findChildren
        // This allows us to get RTW graph instances without accessing private members
        QList<RTWGraph*> rtwGraphs = graphgrid->findChildren<RTWGraph*>();
        DEBUG_OUT() << "RTW: Found" << rtwGraphs.size() << "RTW graph(s) in GraphLayout";
        
        RTWGraph* rtwGraphToUse = nullptr;
        
        // Find an RTW graph that uses the GraphLayout's RTW data source
        for (RTWGraph* rtwGraph : rtwGraphs) {
            if (!rtwGraph) continue;
            
            WaterfallData* graphDataSource = rtwGraph->getDataSource();
            if (graphDataSource == overviewRTWData) {
                rtwGraphToUse = rtwGraph;
                DEBUG_OUT() << "RTW: Found RTW graph using GraphLayout RTW data source";
                break;
            }
        }
        
        // Get the current time range from the RTW graph to add symbols within visible range
        QDateTime symbolTimeMin, symbolTimeMax;
        bool timeRangeValid = false;
        
        if (rtwGraphToUse) {
            auto timeRange = rtwGraphToUse->getTimeRange();
            symbolTimeMin = timeRange.first;
            symbolTimeMax = timeRange.second;
            timeRangeValid = symbolTimeMin.isValid() && symbolTimeMax.isValid() && symbolTimeMin < symbolTimeMax;
            DEBUG_OUT() << "RTW: Current graph time range:" << symbolTimeMin.toString() << "to" << symbolTimeMax.toString() << "- Valid:" << timeRangeValid;
        }
        
        // If graph time range is invalid, try data source time range
        if (!timeRangeValid) {
            if (!overviewRTWData->isEmpty()) {
                auto timeRange = overviewRTWData->getCombinedTimeRange();
                symbolTimeMin = timeRange.first;
                symbolTimeMax = timeRange.second;
                timeRangeValid = symbolTimeMin.isValid() && symbolTimeMax.isValid() && symbolTimeMin < symbolTimeMax;
                DEBUG_OUT() << "RTW: Using data source time range:" << symbolTimeMin.toString() << "to" << symbolTimeMax.toString() << "- Valid:" << timeRangeValid;
            }
        }
        
        // If still invalid, use current time with a window
        if (!timeRangeValid) {
            symbolTimeMax = QDateTime::currentDateTime();
            symbolTimeMin = symbolTimeMax.addSecs(-150); // 2.5 minutes window
            DEBUG_OUT() << "RTW: No valid time range available, using default:" << symbolTimeMin.toString() << "to" << symbolTimeMax.toString();
        }
        
        // Calculate symbol timestamps with 10 second intervals
        // Start from symbolTimeMin and add 0, 10, 20, 30, 40 seconds
        QDateTime symbol1Time = symbolTimeMin.addSecs(0);
        QDateTime symbol2Time = symbolTimeMin.addSecs(250);
        QDateTime symbol3Time = symbolTimeMin.addSecs(100);
        QDateTime symbol4Time = symbolTimeMin.addSecs(150);
        QDateTime symbol5Time = symbolTimeMin.addSecs(200);
        
        DEBUG_OUT() << "RTW: Symbol timestamps:";
        DEBUG_OUT() << "  Symbol1 (TM):" << symbol1Time.toString();
        DEBUG_OUT() << "  Symbol2 (DP):" << symbol2Time.toString();
        DEBUG_OUT() << "  Symbol3 (LY):" << symbol3Time.toString();
        DEBUG_OUT() << "  Symbol4 (CircleI):" << symbol4Time.toString();
        DEBUG_OUT() << "  Symbol5 (Triangle):" << symbol5Time.toString();
        DEBUG_OUT() << "RTW: Time range for filtering:" << symbolTimeMin.toString() << "to" << symbolTimeMax.toString();
        
        if (rtwGraphToUse) {
            // Use rtwGraphToUse->addRTWSymbol() which adds to dataSource AND calls draw() automatically
            DEBUG_OUT() << "RTW: Adding test symbols via RTW graph->addRTWSymbol() within time range";
            rtwGraphToUse->addRTWSymbol("TM", symbol1Time, 10.0);
            rtwGraphToUse->addRTWSymbol("DP", symbol2Time, 15.0);
            rtwGraphToUse->addRTWSymbol("LY", symbol3Time, 20.0);
            rtwGraphToUse->addRTWSymbol("CircleI", symbol4Time, 8.0);
            rtwGraphToUse->addRTWSymbol("Triangle", symbol5Time, 12.0);
        } else {
            // Fallback: add directly to data source (symbols will appear on next redraw)
            DEBUG_OUT() << "RTW: WARNING - No RTW graph found using GraphLayout RTW data source";
            DEBUG_OUT() << "RTW: Adding symbols directly to data source (will appear on next redraw)";
            overviewRTWData->addRTWSymbol("TM", symbol1Time, 10.0);
            overviewRTWData->addRTWSymbol("DP", symbol2Time, 15.0);
            overviewRTWData->addRTWSymbol("LY", symbol3Time, 20.0);
            overviewRTWData->addRTWSymbol("CircleI", symbol4Time, 8.0);
            overviewRTWData->addRTWSymbol("Triangle", symbol5Time, 12.0);
        }
        
        // Verify symbols were added
        DEBUG_OUT() << "RTW: After adding - symbols in GraphLayout RTW data:" << overviewRTWData->getRTWSymbolsCount();
        DEBUG_OUT() << "RTW: ===== Finished adding 5 test symbols to overview tab RTW graph =====";
    });

    // FTW Graph - Frequency Time Waterfall
    ftwGraph = new FTWGraph(ui->customGraphsTab, false, 8, TimeInterval::FifteenMinutes);
    ftwGraph->setObjectName("ftwGraph");
    ftwGraph->setDataSource(*ftwData); // Connect to data source
    
    // Set series colors for FTW graph
    ftwGraph->setSeriesColor("FTW-1", QColor(Qt::red));
    ftwGraph->setSeriesColor("FTW-2", QColor(Qt::green));
    ftwGraph->setSeriesColor("ADOPTED", QColor(Qt::yellow));
    DEBUG_OUT() << "FTW Graph connected to data source and colors set";

    // Create labels for each graph
    QLabel *fdwLabel = new QLabel("FDW", ui->customGraphsTab);
    QLabel *bdwLabel = new QLabel("BDW", ui->customGraphsTab);
    QLabel *brwLabel = new QLabel("BRW", ui->customGraphsTab);
    QLabel *ltwLabel = new QLabel("LTW", ui->customGraphsTab);
    QLabel *btwLabel = new QLabel("BTW", ui->customGraphsTab);
    QLabel *rtwLabel = new QLabel("RTW", ui->customGraphsTab);
    QLabel *ftwLabel = new QLabel("FTW", ui->customGraphsTab);

    // Style the labels
    QString labelStyle = "QLabel { color: white; font-size: 12px; font-weight: bold; background-color: rgba(0, 0, 0, 150); padding: 4px; border-radius: 4px; }";
    fdwLabel->setStyleSheet(labelStyle);
    bdwLabel->setStyleSheet(labelStyle);
    brwLabel->setStyleSheet(labelStyle);
    ltwLabel->setStyleSheet(labelStyle);
    btwLabel->setStyleSheet(labelStyle);
    rtwLabel->setStyleSheet(labelStyle);
    ftwLabel->setStyleSheet(labelStyle);

    // Set label alignment
    fdwLabel->setAlignment(Qt::AlignCenter);
    bdwLabel->setAlignment(Qt::AlignCenter);
    brwLabel->setAlignment(Qt::AlignCenter);
    ltwLabel->setAlignment(Qt::AlignCenter);
    btwLabel->setAlignment(Qt::AlignCenter);
    rtwLabel->setAlignment(Qt::AlignCenter);
    ftwLabel->setAlignment(Qt::AlignCenter);

    // Add graphs and labels horizontally stacked
    // Each graph gets its own vertical container with label above and graph below
    QVBoxLayout *fdwContainer = new QVBoxLayout();
    fdwContainer->addWidget(fdwLabel);
    fdwContainer->addWidget(fdwGraph);
    fdwContainer->setSpacing(2);
    horizontalLayout->addLayout(fdwContainer, 1);

    QVBoxLayout *bdwContainer = new QVBoxLayout();
    bdwContainer->addWidget(bdwLabel);
    bdwContainer->addWidget(bdwGraph);
    bdwContainer->setSpacing(2);
    horizontalLayout->addLayout(bdwContainer, 1);

    QVBoxLayout *brwContainer = new QVBoxLayout();
    brwContainer->addWidget(brwLabel);
    brwContainer->addWidget(brwGraph);
    brwContainer->setSpacing(2);
    horizontalLayout->addLayout(brwContainer, 1);

    QVBoxLayout *ltwContainer = new QVBoxLayout();
    ltwContainer->addWidget(ltwLabel);
    ltwContainer->addWidget(ltwGraph);
    ltwContainer->setSpacing(2);
    horizontalLayout->addLayout(ltwContainer, 1);

    QVBoxLayout *btwContainer = new QVBoxLayout();
    btwContainer->addWidget(btwLabel);
    btwContainer->addWidget(btwGraph);
    btwContainer->setSpacing(2);
    horizontalLayout->addLayout(btwContainer, 1);

    QVBoxLayout *rtwContainer = new QVBoxLayout();
    rtwContainer->addWidget(rtwLabel);
    rtwContainer->addWidget(rtwGraph);
    rtwContainer->setSpacing(2);
    horizontalLayout->addLayout(rtwContainer, 1);

    QVBoxLayout *ftwContainer = new QVBoxLayout();
    ftwContainer->addWidget(ftwLabel);
    ftwContainer->addWidget(ftwGraph);
    ftwContainer->setSpacing(2);
    horizontalLayout->addLayout(ftwContainer, 1);

    // Create configuration map for data generation
    auto waterfallDataMap = std::map<WaterfallData*, SimulatorConfig>();

    waterfallDataMap[fdwData] = SimulatorConfig{8.0, 30.0, 19.0, 2.2};
    waterfallDataMap[bdwData] = SimulatorConfig{-30.0, 30.0, 0.0, 6.0};
    waterfallDataMap[brwData] = SimulatorConfig{8.0, 30.0, 19.0, 2.2};
    waterfallDataMap[ltwData] = SimulatorConfig{15.0, 30.0, 22.5, 1.5};
    waterfallDataMap[btwData] = SimulatorConfig{5.0, 40.0, 22.5, 3.5};
    waterfallDataMap[rtwData] = SimulatorConfig{0.0, 25.0, 12.5, 2.5};
    waterfallDataMap[ftwData] = SimulatorConfig{15.0, 30.0, 22.5, 1.5};

    // Generate the data for the new graph components
    Simulator::generateBulkDataForWaterfallData(waterfallDataMap, 90);

    // Trigger the graphs to redraw
    fdwGraph->update();
    bdwGraph->update();
    brwGraph->update();
    ltwGraph->update();
    btwGraph->update();
    rtwGraph->update();
    ftwGraph->update();
    DEBUG_OUT() << "All graphs redrawn";

    // Debug: Verify data was generated for each series
    DEBUG_OUT() << "=== Verifying bulk data generation ===";
    DEBUG_OUT() << "FDW Data series:" << fdwData->getDataSeriesLabels().size() << "series";
    if (!fdwData->getDataSeriesLabels().empty()) {
        QString firstSeries = fdwData->getDataSeriesLabels()[0];
        DEBUG_OUT() << "FDW First series" << firstSeries << "has" << fdwData->getDataSeriesSize(firstSeries) << "points";
    }
    
    DEBUG_OUT() << "BDW Data series:" << bdwData->getDataSeriesLabels().size() << "series";
    if (!bdwData->getDataSeriesLabels().empty()) {
        QString firstSeries = bdwData->getDataSeriesLabels()[0];
        DEBUG_OUT() << "BDW First series" << firstSeries << "has" << bdwData->getDataSeriesSize(firstSeries) << "points";
    }
    
    DEBUG_OUT() << "BRW Data series:" << brwData->getDataSeriesLabels().size() << "series";
    if (!brwData->getDataSeriesLabels().empty()) {
        QString firstSeries = brwData->getDataSeriesLabels()[0];
        DEBUG_OUT() << "BRW First series" << firstSeries << "has" << brwData->getDataSeriesSize(firstSeries) << "points";
    }
    
    DEBUG_OUT() << "LTW Data series:" << ltwData->getDataSeriesLabels().size() << "series";
    if (!ltwData->getDataSeriesLabels().empty()) {
        QString firstSeries = ltwData->getDataSeriesLabels()[0];
        DEBUG_OUT() << "LTW First series" << firstSeries << "has" << ltwData->getDataSeriesSize(firstSeries) << "points";
    }
    
    DEBUG_OUT() << "BTW Data series:" << btwData->getDataSeriesLabels().size() << "series";
    if (!btwData->getDataSeriesLabels().empty()) {
        QString firstSeries = btwData->getDataSeriesLabels()[0];
        DEBUG_OUT() << "BTW First series" << firstSeries << "has" << btwData->getDataSeriesSize(firstSeries) << "points";
    }
    
    DEBUG_OUT() << "RTW Data series:" << rtwData->getDataSeriesLabels().size() << "series";
    if (!rtwData->getDataSeriesLabels().empty()) {
        QString firstSeries = rtwData->getDataSeriesLabels()[0];
        DEBUG_OUT() << "RTW First series" << firstSeries << "has" << rtwData->getDataSeriesSize(firstSeries) << "points";
    }
    
    DEBUG_OUT() << "FTW Data series:" << ftwData->getDataSeriesLabels().size() << "series";
    if (!ftwData->getDataSeriesLabels().empty()) {
        QString firstSeries = ftwData->getDataSeriesLabels()[0];
        DEBUG_OUT() << "FTW First series" << firstSeries << "has" << ftwData->getDataSeriesSize(firstSeries) << "points";
    }

    // Force all graphs to redraw with the new data
    DEBUG_OUT() << "=== Forcing graph redraws ===";
    fdwGraph->update();
    bdwGraph->update();
    brwGraph->update();
    ltwGraph->update();
    btwGraph->update();
    rtwGraph->update();
    ftwGraph->update();
    DEBUG_OUT() << "All graphs redrawn";

    DEBUG_OUT() << "New graph components tab setup completed successfully";
}

void MainWindow::setupNewGraphData()
{
    // No initial data setup - graphs will be populated by simulation
    DEBUG_OUT() << "Graph data will be populated by simulation";
}

void MainWindow::setBulkDataForAllGraphs()
{
    // Skip if simulator is disabled
    if (!simulator) {
        DEBUG_OUT() << "setBulkDataForAllGraphs: Simulator is disabled, skipping";
        return;
    }

    auto waterfallDataMap = std::map<WaterfallData* , SimulatorConfig>();

    WaterfallData fdwData("FDW", {"FDW-1", "FDW-2"});
    WaterfallData bdwData("BDW", {"BDW-1", "BDW-2"});
    WaterfallData brwData("BRW", {"BRW-1", "BRW-2"});
    WaterfallData ltwData("LTW", {"LTW-1", "LTW-2"});
    WaterfallData btwData("BTW", {"BTW-1", "BTW-2", "BTW-3"});
    WaterfallData rtwData("RTW", {"RTW-1", "RTW-2"});
    WaterfallData ftwData("FTW", {"FTW-1", "FTW-2"});

    waterfallDataMap[&fdwData] = SimulatorConfig{-30.0, 30.0, 0.0, 6.0};
    waterfallDataMap[&bdwData] = SimulatorConfig{-30.0, 30.0, 0.0, 6.0};
    waterfallDataMap[&brwData] = SimulatorConfig{-30.0, 30.0, 0.0, 6.0};
    waterfallDataMap[&ltwData] = SimulatorConfig{15.0, 30.0, 22.5, 1.5};
    waterfallDataMap[&btwData] = SimulatorConfig{5.0, 40.0, 22.5, 3.5};
    waterfallDataMap[&rtwData] = SimulatorConfig{0.0, 25.0, 12.5, 2.5};
    waterfallDataMap[&ftwData] = SimulatorConfig{15.0, 30.0, 22.5, 1.5};
    
    // Method moved to Simulator class
    simulator->generateBulkDataForWaterfallData(waterfallDataMap, 90);
}

/**
 * @brief Simple widget class to display RTW symbols for testing
 */
class RTWSymbolsTestWidget : public QWidget
{
public:
    RTWSymbolsTestWidget(QWidget* parent = nullptr) : QWidget(parent), symbols(40)
    {
        setMinimumSize(1200, 800);
        // Set black background
        QPalette pal = palette();
        pal.setColor(QPalette::Window, Qt::black);
        setPalette(pal);
        setAutoFillBackground(true);
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        int symbolSize = 60;
        int spacing = 120;
        int startX = 50;
        int startY = 80;
        int currentX = startX;
        int currentY = startY;

        // Draw title
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 16, QFont::Bold));
        painter.drawText(QRect(0, 10, width(), 30), Qt::AlignCenter, "RTW Symbols Test");

        // Draw all symbol types
        QList<RTWSymbolDrawing::SymbolType> symbolTypes = {
            RTWSymbolDrawing::SymbolType::TM,
            RTWSymbolDrawing::SymbolType::DP,
            RTWSymbolDrawing::SymbolType::LY,
            RTWSymbolDrawing::SymbolType::CircleI,
            RTWSymbolDrawing::SymbolType::Triangle,
            RTWSymbolDrawing::SymbolType::RectR,
            RTWSymbolDrawing::SymbolType::EllipsePP,
            RTWSymbolDrawing::SymbolType::RectX,
            RTWSymbolDrawing::SymbolType::RectA,
            RTWSymbolDrawing::SymbolType::RectAPurple,
            RTWSymbolDrawing::SymbolType::RectK,
            RTWSymbolDrawing::SymbolType::CircleRYellow,
            RTWSymbolDrawing::SymbolType::DoubleBarYellow,
            RTWSymbolDrawing::SymbolType::R,
            RTWSymbolDrawing::SymbolType::L,
            RTWSymbolDrawing::SymbolType::BOT,
            RTWSymbolDrawing::SymbolType::BOTC,
            RTWSymbolDrawing::SymbolType::BOTF,
            RTWSymbolDrawing::SymbolType::BOTD,
            RTWSymbolDrawing::SymbolType::YellowCircle1,
            RTWSymbolDrawing::SymbolType::YellowCircle2,
            RTWSymbolDrawing::SymbolType::YellowCircle3,
            RTWSymbolDrawing::SymbolType::YellowCircle4,
            RTWSymbolDrawing::SymbolType::MaxSymbol,
            RTWSymbolDrawing::SymbolType::MinSymbol
            
        };

        QStringList symbolNames = {
            "TTM Range",
            "DOPPLER Range",
            "LLOYD Range",
            "SONAR Range",
            "INTERCEPTION SONAR",
            "RADAR Range",
            "RULER PIVOT Range",
            "EXTERNAL Range",
            "REAL TIME ADOPTION",
            "PAST TIME ADOPTION",
            "EKELUND Range",
            "LATERAL Range",
            "MIN/MAX Range",
            "ATMA-ATMAF",
            "BOPT",
            "BOT",
            "BOTC",
            "BFT",
            "BRAT",
            "Wavy Circle (Green)",
            "Scallop Ellipse (Green)",
            "Yellow Circle 1",
            "Yellow Circle 2",
            "Yellow Circle 3",
            "Yellow Circle 4",
            "MAX Symbol",
            "MIN Symbol"
        };

        for (int i = 0; i < symbolTypes.size(); ++i)
        {
            // Draw symbol label (with space above symbol)
            painter.setPen(Qt::white);
            painter.setFont(QFont("Arial", 10));
            painter.drawText(QRect(currentX - symbolSize/2, currentY - 45, symbolSize, 20), 
                           Qt::AlignCenter, symbolNames[i]);

            // Draw the symbol (with space below label)
            symbols.draw(&painter, QPointF(currentX, currentY), symbolTypes[i]);

            // Move to next position
            currentX += spacing;
            if (currentX + spacing > width() - startX)
            {
                currentX = startX;
                currentY += spacing + 20; // Extra spacing between rows
            }
        }
    }

private:
    RTWSymbolDrawing symbols;
};

/**
 * @brief Setup RTW Symbols test widget in a new tab
 */
void MainWindow::setupRTWSymbolsTest()
{
    DEBUG_OUT() << "=== Setting up RTW Symbols Test ===";
    
    // Create a new tab for RTW symbols test
    QWidget* rtwSymbolsTab = new QWidget();
    rtwSymbolsTab->setObjectName("rtwSymbolsTab");
    ui->tabWidget->addTab(rtwSymbolsTab, "RTW Symbols Test");
    
    // Create the RTW symbols test widget
    rtwSymbolsTestWidget = new RTWSymbolsTestWidget(rtwSymbolsTab);
    rtwSymbolsTestWidget->setObjectName("rtwSymbolsTestWidget");
    rtwSymbolsTestWidget->setGeometry(QRect(10, 10, 1200, 800));
    
    // Add instructions label
    QLabel* instructionsLabel = new QLabel(
        "RTW Symbols Test\n"
        "This widget displays all available RTW symbol types:\n\n"
        "Range Types:\n"
        "• TTM Range - TM\n"
        "• DOPPLER Range - DP\n"
        "• LLOYD Range - LY\n"
        "• SONAR Range - CircleI\n"
        "• RADAR Range - RectR\n"
        "• RULER PIVOT Range - EllipsePP\n"
        "• EXTERNAL Range - RectX\n"
        "• EKELUND Range - RectK\n"
        "• LATERAL Range - CircleRYellow\n"
        "• MIN/MAX Range - DoubleBarYellow\n"
        "• MAX Symbol - Yellow line with cyan dots behind\n"
        "• MIN Symbol - Yellow line with cyan dots in front\n\n"
        "Adoption Types:\n"
        "• REAL TIME ADOPTION - RectA (Red)\n"
        "• PAST TIME ADOPTION - RectAPurple\n\n"
        "Methodology Types:\n"
        "• ATMA-ATMAF - R (Orange)\n"
        "• BOPT - L in Circle (Green)\n"
        "• BOT - L in Rectangle (Green)\n"
        "• BOTC - C (Green)\n"
        "• BFT - F (Green)\n"
        "• BRAT - D (Green)\n\n"
        "Other:\n"
        "• INTERCEPTION SONAR - Triangle",
        rtwSymbolsTab);
    instructionsLabel->setGeometry(QRect(1220, 10, 350, 750));
    instructionsLabel->setStyleSheet("QLabel { color: white; font-size: 12px; background-color: rgba(0, 0, 0, 150); padding: 10px; border: 1px solid gray; border-radius: 4px; }");
    instructionsLabel->setWordWrap(true);
    
    DEBUG_OUT() << "RTW Symbols test widget created in new tab";
    DEBUG_OUT() << "RTW Symbols test widget geometry:" << rtwSymbolsTestWidget->geometry();
    DEBUG_OUT() << "RTW Symbols test widget visible:" << rtwSymbolsTestWidget->isVisible();
}

/**
 * @brief Setup RTW Symbol Test Tab with GraphContainer
 * 
 * Creates a new tab with a GraphContainer containing an RTW graph.
 * Adds test data and symbols to verify symbol persistence through
 * track changes and zoom customization.
 */