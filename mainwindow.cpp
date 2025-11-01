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
    graphgrid->setGeometry(QRect(100, 100, 900, 900));

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
    // RTW: sim range 0.0-25.0, so limits 0.0-(0.0+2*(25.0-0.0)) = 0.0-50.0
    graphgrid->setHardRangeLimits(GraphType::RTW, 0.0, 50.0);  // Right Track Window
    // FTW: sim range 15.0-30.0, so limits 15.0-45.0
    graphgrid->setHardRangeLimits(GraphType::FTW, 15.0, 45.0);  // Frequency Time Window
    // graphgrid->setRangeLimits(GraphType::RTW, 36.0, 84.0);  // Right Track Window
    graphgrid->setHardRangeLimits(GraphType::FTW, 45.0, 90.0);  // Frequency Time Window

    ui->tsv->setData(
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
    
    // Setup test WaterfallGraph in controls tab for crosshair testing
    setupTestWaterfallGraph();

    // Setup TimelineView in controls tab for slider testing
    setupTimelineView();

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
    ui->tsv->setData(
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

/**
 * @brief Setup test WaterfallGraph in controls tab for crosshair testing
 */
void MainWindow::setupTestWaterfallGraph()
{
    qDebug() << "=== Setting up Test WaterfallGraph in Controls Tab ===";
    
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
        testWaterfallGraph->addDataPoint("TEST-1", value, timestamp);
        
        qreal adoptedValue = 0.5 + 0.2 * std::cos(i * 0.3);
        testWaterfallGraph->addDataPoint("ADOPTED", adoptedValue, timestamp);
    }
    
    qDebug() << "Test WaterfallGraph created in controls tab with" 
             << testWaterfallData->getDataSeriesSize("TEST-1") << "test data points";
    qDebug() << "Crosshair enabled:" << testWaterfallGraph->isCrosshairEnabled();
    qDebug() << "Test WaterfallGraph geometry:" << testWaterfallGraph->geometry();
    qDebug() << "Test WaterfallGraph visible:" << testWaterfallGraph->isVisible();
}

/**
 * @brief Setup TimelineView in a dedicated timelineview tab
 */
void MainWindow::setupTimelineView()
{
    qDebug() << "=== Setting up TimelineView Tab ===";
    
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
                    // Update start time label
                    timespanStartLabel->setText(selection.startTime.toString("HH:mm:ss"));
                    
                    // Update end time label
                    timespanEndLabel->setText(selection.endTime.toString("HH:mm:ss"));
                    
                    // Calculate and display duration using msecsTo which handles rollovers
                    int durationMs = selection.startTime.msecsTo(selection.endTime);
                    if (durationMs < 0) {
                        durationMs += 24 * 3600 * 1000; // Add 24 hours if negative
                    }
                    int durationSeconds = durationMs / 1000;
                    
                    QTime duration = QTime(0, 0, 0).addSecs(durationSeconds);
                    timespanDurationLabel->setText(duration.toString("HH:mm:ss"));
                    
                    qDebug() << "TimeScopeChanged - Start:" << selection.startTime.toString("HH:mm:ss")
                             << "End:" << selection.endTime.toString("HH:mm:ss")
                             << "Duration:" << duration.toString("HH:mm:ss");
                }
            });
    
    qDebug() << "TimelineView created in dedicated Timeline View tab";
    qDebug() << "TimelineView geometry:" << testTimelineView->geometry();
    qDebug() << "TimelineView visible:" << testTimelineView->isVisible();
}

void MainWindow::setupCustomGraphsTab()
{
    qDebug() << "=== Setting up Custom Graph Components Tab ===";

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
    qDebug() << "FDW Graph connected to data source and colors set";

    // BDW Graph - Bandwidth Domain Waterfall
    bdwGraph = new BDWGraph(ui->customGraphsTab, false, 8, TimeInterval::FifteenMinutes);
    bdwGraph->setObjectName("bdwGraph");
    bdwGraph->setDataSource(*bdwData); // Connect to data source
    
    // Set series colors for BDW graph
    bdwGraph->setSeriesColor("BDW-1", QColor(Qt::red));
    bdwGraph->setSeriesColor("BDW-2", QColor(Qt::green));
    bdwGraph->setSeriesColor("ADOPTED", QColor(Qt::yellow));
    qDebug() << "BDW Graph connected to data source and colors set";

    // BRW Graph - Bit Rate Waterfall
    brwGraph = new BRWGraph(ui->customGraphsTab, false, 8, TimeInterval::FifteenMinutes);
    brwGraph->setObjectName("brwGraph");
    brwGraph->setDataSource(*brwData); // Connect to data source
    
    // Set series colors for BRW graph
    brwGraph->setSeriesColor("BRW-1", QColor(Qt::green));
    brwGraph->setSeriesColor("BRW-2", QColor(Qt::blue));
    brwGraph->setSeriesColor("ADOPTED", QColor(Qt::yellow));
    qDebug() << "BRW Graph connected to data source and colors set";

    // LTW Graph - Latency Time Waterfall
    ltwGraph = new LTWGraph(ui->customGraphsTab, false, 8, TimeInterval::FifteenMinutes);
    ltwGraph->setObjectName("ltwGraph");
    ltwGraph->setDataSource(*ltwData); // Connect to data source
    
    // Set series colors for LTW graph
    ltwGraph->setSeriesColor("LTW-1", QColor(Qt::red));
    ltwGraph->setSeriesColor("LTW-2", QColor(Qt::green));
    ltwGraph->setSeriesColor("ADOPTED", QColor(Qt::yellow));
    qDebug() << "LTW Graph connected to data source and colors set";

    // BTW Graph - Bit Time Waterfall
    btwGraph = new BTWGraph(ui->customGraphsTab, false, 8, TimeInterval::FifteenMinutes);
    btwGraph->setObjectName("btwGraph");
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
    rtwGraph->setDataSource(*rtwData); // Connect to data source
    
    // Set series colors for RTW graph
    rtwGraph->setSeriesColor("RTW-1", QColor(Qt::red));
    rtwGraph->setSeriesColor("RTW-2", QColor(Qt::green));
    rtwGraph->setSeriesColor("ADOPTED", QColor(Qt::yellow));
    qDebug() << "RTW Graph connected to data source and colors set";

    // FTW Graph - Frequency Time Waterfall
    ftwGraph = new FTWGraph(ui->customGraphsTab, false, 8, TimeInterval::FifteenMinutes);
    ftwGraph->setObjectName("ftwGraph");
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
    waterfallDataMap[&rtwData] = SimulatorConfig{0.0, 25.0, 12.5, 2.5};
    waterfallDataMap[&ftwData] = SimulatorConfig{15.0, 30.0, 22.5, 1.5};
    
    // Method moved to Simulator class
    simulator->generateBulkDataForWaterfallData(waterfallDataMap, 90);
}
