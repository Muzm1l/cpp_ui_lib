#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#include <QDebug>
#include <QLabel>
#include <cmath>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), timer(new QTimer(this)), timeUpdateTimer(new QTimer(this))
{
    ui->setupUi(this);

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
    graphgrid->setGeometry(QRect(970, 70, 611, 651));

    // Create Simulator instance
    simulator = new Simulator(this, timeUpdateTimer, graphgrid);

    // inside MainWindow constructor
    std::srand(std::time(nullptr));

    this->simTick = 0;
    connect(timer, &QTimer::timeout, this, &MainWindow::updateSimulation);
    timer->start(1000); // 1000ms = 1 second

    // // Set bulk data for all graphs (simulation disabled)
    // setBulkDataForAllGraphs();

    // Set up timer for time updates (every 1 second) - this will be passed to GraphLayout
    timeUpdateTimer->setInterval(1000); // 1000ms = 1 second
    timeUpdateTimer->start();

    // Start the simulator
    simulator->start();

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
    graphgrid->setHardRangeLimits(GraphType::FDW, 8.0, 52.0);  // Frequency Domain Window
    // BDW: sim range 5.0-38.0, so limits 5.0-(5.0+2*(38.0-5.0)) = 5.0-71.0
    graphgrid->setHardRangeLimits(GraphType::BDW, 5.0, 71.0);  // Bandwidth Domain Window
    // BRW: sim range 8.0-30.0, so limits 8.0-52.0
    graphgrid->setHardRangeLimits(GraphType::BRW, 8.0, 52.0);  // Bit Rate Window
    // LTW: sim range 15.0-30.0, so limits 15.0-(15.0+2*(30.0-15.0)) = 15.0-45.0
    graphgrid->setHardRangeLimits(GraphType::LTW, 15.0, 45.0);  // Left Track Window
    // BTW: sim range 5.0-40.0, so limits 5.0-(5.0+2*(40.0-5.0)) = 5.0-75.0
    graphgrid->setHardRangeLimits(GraphType::BTW, 5.0, 75.0);   // Bottom Track Window
    // RTW: sim range 12.0-28.0, so limits 12.0-(12.0+2*(28.0-12.0)) = 12.0-44.0
    graphgrid->setHardRangeLimits(GraphType::RTW, 12.0, 44.0);  // Right Track Window
    // FTW: sim range 15.0-30.0, so limits 15.0-45.0
    graphgrid->setHardRangeLimits(GraphType::FTW, 15.0, 45.0);  // Frequency Time Window
    // graphgrid->setRangeLimits(GraphType::RTW, 36.0, 84.0);  // Right Track Window
    graphgrid->setHardRangeLimits(GraphType::FTW, 45.0, 90.0);  // Frequency Time Window

    ui->widget_2->setData(
        this->currentShipSpeed,
        this->currentOwnShipBearing,
        this->currentSensorBearing,
        this->currentAdoptedTrackRange,
        this->currentAdoptedTrackSpeed,
        this->currentAdoptedTrackBearing,
        this->currentSelectedTrackRange,
        this->currentSelectedTrackSpeed,
        this->currentSelectedTrackBearing,
        this->currentAdoptedTrackCourse,
        this->currentSelectedTrackCourse);

    // Configure layout selection combobox
    configureLayoutSelection();

    // Setup custom graphs tab
    setupCustomGraphsTab();

    // // Configure TimeSelectionVisualizer
    // configureTimeVisualizer();

    // // Configure TimelineView
    // configureTimelineView();

    // Configure Zoom Panel test functionality
    configureZoomPanel();


    // // Set up timer for current time updates (every second)
    // connect(timeUpdateTimer, &QTimer::timeout, this, &MainWindow::updateCurrentTime);
    // timeUpdateTimer->start(1000); // 1000ms = 1 second
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
    // qDebug() << "Sim Tick: " << this->simTick;

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

    // Push updated values into TacticalSolutionView
    ui->widget_2->setData(
        this->currentShipSpeed,
        this->currentOwnShipBearing,
        this->currentSensorBearing,
        this->currentAdoptedTrackRange,
        this->currentAdoptedTrackSpeed,
        this->currentAdoptedTrackBearing,
        this->currentSelectedTrackRange,
        this->currentSelectedTrackSpeed,
        this->currentSelectedTrackBearing,
        this->currentAdoptedTrackCourse,
        this->currentSelectedTrackCourse);

    // set the current time to the system time
    graphgrid->setCurrentTime(QTime::currentTime());

    // Set chevron labels to demonstrate functionality
    graphgrid->setChevronLabel1("Start");
    graphgrid->setChevronLabel2("Now");
    graphgrid->setChevronLabel3("End");
}

// void MainWindow::configureTimeVisualizer()
// {
//     // Set timeline length to 1 hour (60 minutes)
//     ui->timeVisualizer->setTimeLineLength(QTime(1, 0, 0));

//     // Set initial current time to system time
//     ui->timeVisualizer->setCurrentTime(QTime::currentTime());

//     // Add some sample time selections relative to current time
//     QTime currentTime = QTime::currentTime();

//     // Create selections that are within the visible range (last hour)
//     // Selection 1: 5-10 minutes ago (should be visible)
//     QTime start1 = currentTime.addSecs(-10 * 60); // 10 minutes ago
//     QTime end1 = currentTime.addSecs(-5 * 60);   // 5 minutes ago
//     TimeSelectionSpan span1(start1, end1);

//     // Selection 2: 20-25 minutes ago (should be visible)
//     QTime start2 = currentTime.addSecs(-25 * 60); // 25 minutes ago
//     QTime end2 = currentTime.addSecs(-20 * 60);   // 20 minutes ago
//     TimeSelectionSpan span2(start2, end2);

//     // Selection 3: 35-45 minutes ago (should be visible)
//     QTime start3 = currentTime.addSecs(-45 * 60); // 45 minutes ago
//     QTime end3 = currentTime.addSecs(-35 * 60);   // 35 minutes ago
//     TimeSelectionSpan span3(start3, end3);

//     // Selection 4: 2-3 minutes ago (should be visible)
//     QTime start4 = currentTime.addSecs(-3 * 60); // 3 minutes ago
//     QTime end4 = currentTime.addSecs(-2 * 60);   // 2 minutes ago
//     TimeSelectionSpan span4(start4, end4);

//     ui->timeVisualizer->addTimeSelection(span1);
//     ui->timeVisualizer->addTimeSelection(span2);
//     ui->timeVisualizer->addTimeSelection(span3);
//     ui->timeVisualizer->addTimeSelection(span4);

// }

// void MainWindow::configureTimelineView()
// {
//     ui->timelineView->setTimeLineLength(TimeInterval::FifteenMinutes); // 15 minutes
//     ui->timelineView->setCurrentTime(QTime::currentTime());
//     ui->timelineView->setNumberOfDivisions(15); // 15 segments

//     // Create a new timer that fires every minute
//     QTimer *timelineUpdateTimer = new QTimer(this);
//     connect(timelineUpdateTimer, &QTimer::timeout, this, &MainWindow::updateTimeline);
//     timelineUpdateTimer->start(60000); // 60000ms = 1 minute

// }

// void MainWindow::updateTimeline()
// {
//     ui->timelineView->setCurrentTime(QTime::currentTime());
// }

// void MainWindow::updateCurrentTime()
// {
//     // Update the current time to the system time
//     ui->timeVisualizer->setCurrentTime(QTime::currentTime());
//     ui->timelineView->setCurrentTime(QTime::currentTime());
// }

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

void MainWindow::setupCustomGraphsTab()
{
    qDebug() << "=== Setting up New Graph Components Tab ===";

    // Create a grid layout for the new graph components tab
    QGridLayout *gridLayout = new QGridLayout(ui->customGraphsTab);
    gridLayout->setSpacing(10);
    gridLayout->setContentsMargins(10, 10, 10, 10);

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
    fdwGraph->setMouseSelectionEnabled(true);
    fdwGraph->setDataSource(*fdwData); // Connect to data source
    
    // Set series colors for FDW graph
    fdwGraph->setSeriesColor("FDW-1", QColor(Qt::red));
    fdwGraph->setSeriesColor("FDW-2", QColor(Qt::green));
    fdwGraph->setSeriesColor("ADOPTED", QColor(Qt::yellow));
    qDebug() << "FDW Graph connected to data source and colors set";

    // BDW Graph - Bandwidth Domain Waterfall
    bdwGraph = new BDWGraph(ui->customGraphsTab, false, 8, TimeInterval::FifteenMinutes);
    bdwGraph->setObjectName("bdwGraph");
    bdwGraph->setMouseSelectionEnabled(true);
    bdwGraph->setDataSource(*bdwData); // Connect to data source
    
    // Set series colors for BDW graph
    bdwGraph->setSeriesColor("BDW-1", QColor(Qt::red));
    bdwGraph->setSeriesColor("BDW-2", QColor(Qt::green));
    bdwGraph->setSeriesColor("ADOPTED", QColor(Qt::yellow));
    qDebug() << "BDW Graph connected to data source and colors set";

    // BRW Graph - Bit Rate Waterfall
    brwGraph = new BRWGraph(ui->customGraphsTab, false, 8, TimeInterval::FifteenMinutes);
    brwGraph->setObjectName("brwGraph");
    brwGraph->setMouseSelectionEnabled(true);
    brwGraph->setDataSource(*brwData); // Connect to data source
    
    // Set series colors for BRW graph
    brwGraph->setSeriesColor("BRW-1", QColor(Qt::green));
    brwGraph->setSeriesColor("BRW-2", QColor(Qt::blue));
    brwGraph->setSeriesColor("ADOPTED", QColor(Qt::yellow));
    qDebug() << "BRW Graph connected to data source and colors set";

    // LTW Graph - Latency Time Waterfall
    ltwGraph = new LTWGraph(ui->customGraphsTab, false, 8, TimeInterval::FifteenMinutes);
    ltwGraph->setObjectName("ltwGraph");
    ltwGraph->setMouseSelectionEnabled(true);
    ltwGraph->setDataSource(*ltwData); // Connect to data source
    
    // Set series colors for LTW graph
    ltwGraph->setSeriesColor("LTW-1", QColor(Qt::red));
    ltwGraph->setSeriesColor("LTW-2", QColor(Qt::green));
    ltwGraph->setSeriesColor("ADOPTED", QColor(Qt::yellow));
    qDebug() << "LTW Graph connected to data source and colors set";

    // BTW Graph - Bit Time Waterfall
    btwGraph = new BTWGraph(ui->customGraphsTab, false, 8, TimeInterval::FifteenMinutes);
    btwGraph->setObjectName("btwGraph");
    btwGraph->setMouseSelectionEnabled(true);
    btwGraph->setDataSource(*btwData); // Connect to data source
    
    // Set series colors for BTW graph
    btwGraph->setSeriesColor("BTW-1", QColor(Qt::red));
    btwGraph->setSeriesColor("BTW-2", QColor(Qt::green));
    btwGraph->setSeriesColor("BTW-3", QColor(Qt::blue));
    btwGraph->setSeriesColor("ADOPTED", QColor(Qt::yellow));
    qDebug() << "BTW Graph connected to data source and colors set";

    // RTW Graph - Rate Time Waterfall
    rtwGraph = new RTWGraph(ui->customGraphsTab, false, 8, TimeInterval::FifteenMinutes);
    rtwGraph->setObjectName("rtwGraph");
    rtwGraph->setMouseSelectionEnabled(true);
    rtwGraph->setDataSource(*rtwData); // Connect to data source
    
    // Set series colors for RTW graph
    rtwGraph->setSeriesColor("RTW-1", QColor(Qt::red));
    rtwGraph->setSeriesColor("RTW-2", QColor(Qt::green));
    rtwGraph->setSeriesColor("ADOPTED", QColor(Qt::yellow));
    qDebug() << "RTW Graph connected to data source and colors set";

    // FTW Graph - Frequency Time Waterfall
    ftwGraph = new FTWGraph(ui->customGraphsTab, false, 8, TimeInterval::FifteenMinutes);
    ftwGraph->setObjectName("ftwGraph");
    ftwGraph->setMouseSelectionEnabled(true);
    ftwGraph->setDataSource(*ftwData); // Connect to data source
    
    // Set series colors for FTW graph
    ftwGraph->setSeriesColor("FTW-1", QColor(Qt::red));
    ftwGraph->setSeriesColor("FTW-2", QColor(Qt::green));
    ftwGraph->setSeriesColor("ADOPTED", QColor(Qt::yellow));
    qDebug() << "FTW Graph connected to data source and colors set";

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

    // Add graphs and labels to grid layout (6x3 - labels above graphs)
    // Row 0: Labels
    gridLayout->addWidget(fdwLabel, 0, 0);
    gridLayout->addWidget(bdwLabel, 0, 1);
    gridLayout->addWidget(brwLabel, 0, 2);
    
    // Row 1: Graphs
    gridLayout->addWidget(fdwGraph, 1, 0);
    gridLayout->addWidget(bdwGraph, 1, 1);
    gridLayout->addWidget(brwGraph, 1, 2);
    
    // Row 2: Labels
    gridLayout->addWidget(ltwLabel, 2, 0);
    gridLayout->addWidget(btwLabel, 2, 1);
    gridLayout->addWidget(rtwLabel, 2, 2);
    
    // Row 3: Graphs
    gridLayout->addWidget(ltwGraph, 3, 0);
    gridLayout->addWidget(btwGraph, 3, 1);
    gridLayout->addWidget(rtwGraph, 3, 2);
    
    // Row 4: Label
    gridLayout->addWidget(ftwLabel, 4, 0);
    
    // Row 5: Graph
    gridLayout->addWidget(ftwGraph, 5, 0);

    // Set equal column and row stretches
    gridLayout->setColumnStretch(0, 1);
    gridLayout->setColumnStretch(1, 1);
    gridLayout->setColumnStretch(2, 1);
    gridLayout->setRowStretch(0, 0);  // Label rows - minimal height
    gridLayout->setRowStretch(1, 1);  // Graph rows - full height
    gridLayout->setRowStretch(2, 0);  // Label rows - minimal height
    gridLayout->setRowStretch(3, 1);  // Graph rows - full height
    gridLayout->setRowStretch(4, 0);  // Label rows - minimal height
    gridLayout->setRowStretch(5, 1);  // Graph rows - full height

    // Create configuration map for data generation
    auto waterfallDataMap = std::map<WaterfallData*, SimulatorConfig>();

    waterfallDataMap[fdwData] = SimulatorConfig{8.0, 30.0, 19.0, 2.2};
    waterfallDataMap[bdwData] = SimulatorConfig{-30.0, 30.0, 0.0, 6.0};
    waterfallDataMap[brwData] = SimulatorConfig{8.0, 30.0, 19.0, 2.2};
    waterfallDataMap[ltwData] = SimulatorConfig{15.0, 30.0, 22.5, 1.5};
    waterfallDataMap[btwData] = SimulatorConfig{5.0, 40.0, 22.5, 3.5};
    waterfallDataMap[rtwData] = SimulatorConfig{12.0, 28.0, 20.0, 1.6};
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
    qDebug() << "All graphs redrawn";

    // Debug: Verify data was generated for each series
    qDebug() << "=== Verifying bulk data generation ===";
    qDebug() << "FDW Data series:" << fdwData->getDataSeriesLabels().size() << "series";
    if (!fdwData->getDataSeriesLabels().empty()) {
        QString firstSeries = fdwData->getDataSeriesLabels()[0];
        qDebug() << "FDW First series" << firstSeries << "has" << fdwData->getDataSeriesSize(firstSeries) << "points";
    }
    
    qDebug() << "BDW Data series:" << bdwData->getDataSeriesLabels().size() << "series";
    if (!bdwData->getDataSeriesLabels().empty()) {
        QString firstSeries = bdwData->getDataSeriesLabels()[0];
        qDebug() << "BDW First series" << firstSeries << "has" << bdwData->getDataSeriesSize(firstSeries) << "points";
    }
    
    qDebug() << "BRW Data series:" << brwData->getDataSeriesLabels().size() << "series";
    if (!brwData->getDataSeriesLabels().empty()) {
        QString firstSeries = brwData->getDataSeriesLabels()[0];
        qDebug() << "BRW First series" << firstSeries << "has" << brwData->getDataSeriesSize(firstSeries) << "points";
    }
    
    qDebug() << "LTW Data series:" << ltwData->getDataSeriesLabels().size() << "series";
    if (!ltwData->getDataSeriesLabels().empty()) {
        QString firstSeries = ltwData->getDataSeriesLabels()[0];
        qDebug() << "LTW First series" << firstSeries << "has" << ltwData->getDataSeriesSize(firstSeries) << "points";
    }
    
    qDebug() << "BTW Data series:" << btwData->getDataSeriesLabels().size() << "series";
    if (!btwData->getDataSeriesLabels().empty()) {
        QString firstSeries = btwData->getDataSeriesLabels()[0];
        qDebug() << "BTW First series" << firstSeries << "has" << btwData->getDataSeriesSize(firstSeries) << "points";
    }
    
    qDebug() << "RTW Data series:" << rtwData->getDataSeriesLabels().size() << "series";
    if (!rtwData->getDataSeriesLabels().empty()) {
        QString firstSeries = rtwData->getDataSeriesLabels()[0];
        qDebug() << "RTW First series" << firstSeries << "has" << rtwData->getDataSeriesSize(firstSeries) << "points";
    }
    
    qDebug() << "FTW Data series:" << ftwData->getDataSeriesLabels().size() << "series";
    if (!ftwData->getDataSeriesLabels().empty()) {
        QString firstSeries = ftwData->getDataSeriesLabels()[0];
        qDebug() << "FTW First series" << firstSeries << "has" << ftwData->getDataSeriesSize(firstSeries) << "points";
    }

    // Force all graphs to redraw with the new data
    qDebug() << "=== Forcing graph redraws ===";
    fdwGraph->update();
    bdwGraph->update();
    brwGraph->update();
    ltwGraph->update();
    btwGraph->update();
    rtwGraph->update();
    ftwGraph->update();
    qDebug() << "All graphs redrawn";

    qDebug() << "New graph components tab setup completed successfully";
}

void MainWindow::setupNewGraphData()
{
    // No initial data setup - graphs will be populated by simulation
    qDebug() << "Graph data will be populated by simulation";
}

void MainWindow::setBulkDataForAllGraphs()
{
    auto waterfallDataMap = std::map<WaterfallData* , SimulatorConfig>();

    WaterfallData fdwData("FDW", {"FDW-1", "FDW-2"});
    WaterfallData bdwData("BDW", {"BDW-1", "BDW-2"});
    WaterfallData brwData("BRW", {"BRW-1", "BRW-2"});
    WaterfallData ltwData("LTW", {"LTW-1", "LTW-2"});
    WaterfallData btwData("BTW", {"BTW-1", "BTW-2", "BTW-3"});
    WaterfallData rtwData("RTW", {"RTW-1", "RTW-2"});
    WaterfallData ftwData("FTW", {"FTW-1", "FTW-2"});

    waterfallDataMap[&fdwData] = SimulatorConfig{8.0, 30.0, 19.0, 2.2};
    waterfallDataMap[&bdwData] = SimulatorConfig{-30.0, 30.0, 0.0, 6.0};
    waterfallDataMap[&brwData] = SimulatorConfig{8.0, 30.0, 19.0, 2.2};
    waterfallDataMap[&ltwData] = SimulatorConfig{15.0, 30.0, 22.5, 1.5};
    waterfallDataMap[&btwData] = SimulatorConfig{5.0, 40.0, 22.5, 3.5};
    waterfallDataMap[&rtwData] = SimulatorConfig{12.0, 28.0, 20.0, 1.6};
    waterfallDataMap[&ftwData] = SimulatorConfig{15.0, 30.0, 22.5, 1.5};
    
    // Method moved to Simulator class
    simulator->generateBulkDataForWaterfallData(waterfallDataMap, 90);
}
