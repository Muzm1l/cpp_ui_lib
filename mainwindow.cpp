#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), timer(new QTimer(this)), timeUpdateTimer(new QTimer(this))
{
    ui->setupUi(this);

    // Create GraphLayout programmatically with default graph types and timer
    graphgrid = new GraphLayout(ui->originalTab, LayoutType::GPW4W, timeUpdateTimer);
    graphgrid->setObjectName("graphgrid");
    graphgrid->setGeometry(QRect(970, 70, 611, 651));

    // inside MainWindow constructor
    std::srand(std::time(nullptr));

    this->simTick = 0;
    // Set up timer for simulation updates (every 1 second)
    connect(timer, &QTimer::timeout, this, &MainWindow::updateSimulation);
    timer->start(1000); // 1000ms = 1 second

    // Set up timer for time updates (every 1 second) - this will be passed to GraphLayout
    timeUpdateTimer->setInterval(1000); // 1000ms = 1 second
    timeUpdateTimer->start();

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

    // Initialize current values for each graph type (starting at middle of ranges)
    this->currentFDWValue = 19.0; // Middle of 8.0-30.0 range
    this->currentBDWValue = 21.5; // Middle of 5.0-38.0 range
    this->currentBRWValue = 19.0; // Middle of 8.0-30.0 range
    this->currentLTWValue = 22.5; // Middle of 15.0-30.0 range
    this->currentBTWValue = 22.5; // Middle of 5.0-40.0 range
    this->currentRTWValue = 20.0; // Middle of 12.0-28.0 range
    this->currentFTWValue = 22.5; // Middle of 15.0-30.0 range

    // Initialize graph configurations with bounds, start values, and delta values
    this->fdwConfig = {8.0, 30.0, 19.0, 2.2};  // Frequency Domain Window: 10% of 22.0 range
    this->bdwConfig = {5.0, 38.0, 21.5, 3.3};  // Bandwidth Domain Window: 10% of 33.0 range
    this->brwConfig = {8.0, 30.0, 19.0, 2.2};  // Bit Rate Window: 10% of 22.0 range
    this->ltwConfig = {15.0, 30.0, 22.5, 1.5}; // Left Track Window: 10% of 15.0 range
    this->btwConfig = {5.0, 40.0, 22.5, 3.5};  // Bottom Track Window: 10% of 35.0 range
    this->rtwConfig = {12.0, 28.0, 20.0, 1.6}; // Right Track Window: 10% of 16.0 range
    this->ftwConfig = {15.0, 30.0, 22.5, 1.5}; // Frequency Time Window: 10% of 15.0 range

    // Set hard limits for all graph types using GraphLayout range limit methods
    graphgrid->setRangeLimits(GraphType::FDW, 24.0, 90.0);  // Frequency Domain Window
    graphgrid->setRangeLimits(GraphType::BDW, 15.0, 114.0); // Bandwidth Domain Window
    graphgrid->setRangeLimits(GraphType::BRW, 24.0, 90.0);  // Bit Rate Window
    graphgrid->setRangeLimits(GraphType::LTW, 45.0, 90.0);  // Left Track Window
    graphgrid->setRangeLimits(GraphType::BTW, 15.0, 120.0); // Bottom Track Window
    graphgrid->setRangeLimits(GraphType::RTW, 36.0, 84.0);  // Right Track Window
    graphgrid->setRangeLimits(GraphType::FTW, 45.0, 90.0);  // Frequency Time Window

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

    // Update graph values with random generation (oldValue + random(-1,1) * delta)
    QDateTime currentTime = QDateTime::currentDateTime();

    // Generate new values using oldValue + random(-1,1) * delta pattern
    this->currentFDWValue = generateRandomValue(this->currentFDWValue, this->fdwConfig.deltaValue);
    this->currentBDWValue = generateRandomValue(this->currentBDWValue, this->bdwConfig.deltaValue);
    this->currentBRWValue = generateRandomValue(this->currentBRWValue, this->brwConfig.deltaValue);
    this->currentLTWValue = generateRandomValue(this->currentLTWValue, this->ltwConfig.deltaValue);
    this->currentBTWValue = generateRandomValue(this->currentBTWValue, this->btwConfig.deltaValue);
    this->currentRTWValue = generateRandomValue(this->currentRTWValue, this->rtwConfig.deltaValue);
    this->currentFTWValue = generateRandomValue(this->currentFTWValue, this->ftwConfig.deltaValue);

    // Clamp values to their respective ranges using configuration bounds
    this->currentFDWValue = qBound(this->fdwConfig.minValue, this->currentFDWValue, this->fdwConfig.maxValue);
    this->currentBDWValue = qBound(this->bdwConfig.minValue, this->currentBDWValue, this->bdwConfig.maxValue);
    this->currentBRWValue = qBound(this->brwConfig.minValue, this->currentBRWValue, this->brwConfig.maxValue);
    this->currentLTWValue = qBound(this->ltwConfig.minValue, this->currentLTWValue, this->ltwConfig.maxValue);
    this->currentBTWValue = qBound(this->btwConfig.minValue, this->currentBTWValue, this->btwConfig.maxValue);
    this->currentRTWValue = qBound(this->rtwConfig.minValue, this->currentRTWValue, this->rtwConfig.maxValue);
    this->currentFTWValue = qBound(this->ftwConfig.minValue, this->currentFTWValue, this->ftwConfig.maxValue);

    // Add new data points to each graph data source with precise timestamping
    graphgrid->addDataPointToDataSource(GraphType::FDW, this->currentFDWValue, currentTime);
    graphgrid->addDataPointToDataSource(GraphType::BDW, this->currentBDWValue, currentTime);
    graphgrid->addDataPointToDataSource(GraphType::BRW, this->currentBRWValue, currentTime);
    graphgrid->addDataPointToDataSource(GraphType::LTW, this->currentLTWValue, currentTime);
    graphgrid->addDataPointToDataSource(GraphType::BTW, this->currentBTWValue, currentTime);
    graphgrid->addDataPointToDataSource(GraphType::RTW, this->currentRTWValue, currentTime);
    graphgrid->addDataPointToDataSource(GraphType::FTW, this->currentFTWValue, currentTime);

    qDebug() << "Added data points - FDW:" << this->currentFDWValue
             << "BDW:" << this->currentBDWValue
             << "BRW:" << this->currentBRWValue
             << "LTW:" << this->currentLTWValue
             << "BTW:" << this->currentBTWValue
             << "RTW:" << this->currentRTWValue
             << "FTW:" << this->currentFTWValue;

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

qreal MainWindow::generateRandomValue(qreal oldValue, qreal deltaValue)
{
    // Generate random value between -1 and 1
    qreal randomFactor = (static_cast<qreal>(std::rand()) / RAND_MAX) * 2.0 - 1.0;
    return oldValue + randomFactor * deltaValue;
}

void MainWindow::setupCustomGraphsTab()
{
    qDebug() << "=== Setting up New Graph Components Tab ===";

    // Create a grid layout for the new graph components tab
    QGridLayout *gridLayout = new QGridLayout(ui->customGraphsTab);
    gridLayout->setSpacing(10);
    gridLayout->setContentsMargins(10, 10, 10, 10);

    // Create all 7 new graph components

    // FDW Graph - Frequency Domain Waterfall
    fdwGraph = new FDWGraph(ui->customGraphsTab, true, 8, TimeInterval::FifteenMinutes);
    fdwGraph->setObjectName("fdwGraph");
    fdwGraph->setMouseSelectionEnabled(true);

    // BDW Graph - Bandwidth Domain Waterfall
    bdwGraph = new BDWGraph(ui->customGraphsTab, true, 8, TimeInterval::FifteenMinutes);
    bdwGraph->setObjectName("bdwGraph");
    bdwGraph->setMouseSelectionEnabled(true);

    // BRW Graph - Bit Rate Waterfall
    brwGraph = new BRWGraph(ui->customGraphsTab, true, 8, TimeInterval::FifteenMinutes);
    brwGraph->setObjectName("brwGraph");
    brwGraph->setMouseSelectionEnabled(true);

    // LTW Graph - Latency Time Waterfall
    ltwGraph = new LTWGraph(ui->customGraphsTab, true, 8, TimeInterval::FifteenMinutes);
    ltwGraph->setObjectName("ltwGraph");
    ltwGraph->setMouseSelectionEnabled(true);

    // BTW Graph - Bit Time Waterfall
    btwGraph = new BTWGraph(ui->customGraphsTab, true, 8, TimeInterval::FifteenMinutes);
    btwGraph->setObjectName("btwGraph");
    btwGraph->setMouseSelectionEnabled(true);

    // RTW Graph - Rate Time Waterfall
    rtwGraph = new RTWGraph(ui->customGraphsTab, true, 8, TimeInterval::FifteenMinutes);
    rtwGraph->setObjectName("rtwGraph");
    rtwGraph->setMouseSelectionEnabled(true);

    // FTW Graph - Frequency Time Waterfall
    ftwGraph = new FTWGraph(ui->customGraphsTab, true, 8, TimeInterval::FifteenMinutes);
    ftwGraph->setObjectName("ftwGraph");
    ftwGraph->setMouseSelectionEnabled(true);

    // Add graphs to grid layout (3x3 with 2 empty spaces)
    gridLayout->addWidget(fdwGraph, 0, 0);
    gridLayout->addWidget(bdwGraph, 0, 1);
    gridLayout->addWidget(brwGraph, 0, 2);
    gridLayout->addWidget(ltwGraph, 1, 0);
    gridLayout->addWidget(btwGraph, 1, 1);
    gridLayout->addWidget(rtwGraph, 1, 2);
    gridLayout->addWidget(ftwGraph, 2, 0);

    // Set equal column and row stretches
    gridLayout->setColumnStretch(0, 1);
    gridLayout->setColumnStretch(1, 1);
    gridLayout->setColumnStretch(2, 1);
    gridLayout->setRowStretch(0, 1);
    gridLayout->setRowStretch(1, 1);
    gridLayout->setRowStretch(2, 1);

    qDebug() << "New graph components tab setup completed successfully";
}

void MainWindow::setupNewGraphData()
{
    // No initial data setup - graphs will be populated by simulation
    qDebug() << "Graph data will be populated by simulation";
}
