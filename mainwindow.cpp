#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), timer(new QTimer(this)), timeUpdateTimer(new QTimer(this))
{
    ui->setupUi(this);

    // Create GraphLayout programmatically
    std::vector<QString> dataSourceLabels = {"LTW", "BTW", "RTW"};
    graphgrid = new GraphLayout(ui->centralwidget, LayoutType::GPW4W, dataSourceLabels);
    graphgrid->setObjectName("graphgrid");
    graphgrid->setGeometry(QRect(970, 70, 611, 651));

    

    // inside MainWindow constructor
    std::srand(std::time(nullptr));

    this->simTick = 0;
    // Set up timer for simulation updates (every 2 seconds)
    connect(timer, &QTimer::timeout, this, &MainWindow::updateSimulation);
    timer->start(2000); // 2000ms = 2 seconds
    

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
    
    // Demonstrate data point methods
    demonstrateDataPointMethods();

    // // Configure TimeSelectionVisualizer 
    // configureTimeVisualizer();

    // // Configure TimelineView
    // configureTimelineView();
    
    // // Configure Zoom Panel test functionality
    // configureZoomPanel();

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
    this->currentAdoptedTrackCourse += (std::rand() % 7) - 3;  // jitter ±3°

    // --- Selected track (large variations >30%) ---
    this->currentSelectedTrackSpeed *= (0.7 + randPercent(60));   // 0.7–1.29 → ±30%
    this->currentSelectedTrackRange *= (0.7 + randPercent(60));   // ±30%
    this->currentSelectedTrackBearing += (std::rand() % 91) - 45; // swing ±45°
    this->currentSelectedTrackCourse += (std::rand() % 91) - 45; // swing ±45°

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

// void MainWindow::configureZoomPanel()
// {
//     // Connect test buttons to zoom panel indicator
//     connect(ui->testButton0, &QPushButton::clicked, [this]() {
//         ui->zoomPanel->setIndicatorValue(0.0);
//     });
    
//     connect(ui->testButton50, &QPushButton::clicked, [this]() {
//         ui->zoomPanel->setIndicatorValue(0.5);
//     });
    
//     connect(ui->testButton100, &QPushButton::clicked, [this]() {
//         ui->zoomPanel->setIndicatorValue(1.0);
//     });
        
//     // Initialize zoom panel with a default value
//     ui->zoomPanel->setIndicatorValue(0.3);
    
//     // Initialize label values
//     ui->zoomPanel->setLeftLabelValue(0.0);  // Left reference value
//     ui->zoomPanel->setCenterLabelValue(0.5); // Center value
//     ui->zoomPanel->setRightLabelValue(1.0);  // Range for upper bound
    
//     // Initialize the indicator value label
//     ui->indicatorValueLabel->setText("Bounds: [0.35, 0.80]");
    
//     // Connect to zoom panel value changed signal
//     connect(ui->zoomPanel, &ZoomPanel::valueChanged, [this](ZoomBounds bounds) {
//         // Update the indicator value label on main window with bounds
//         ui->indicatorValueLabel->setText(QString("Bounds: [%1, %2]").arg(bounds.lowerbound, 0, 'f', 2).arg(bounds.upperbound, 0, 'f', 2));
        
//         // You can add additional logic here to respond to value changes
//         // For example, update other UI elements or trigger other actions
//     });
// }

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

void MainWindow::demonstrateDataPointMethods()
{
    qDebug() << "=== Data Source Demonstration ===";
    
    // Demonstrate the new data source methods
    QDateTime baseTime = QDateTime::currentDateTime();
    
    // Add data points to specific data sources
    graphgrid->addDataPointToDataSource("LTW", 25.0, baseTime.addMSecs(1000));
    graphgrid->addDataPointToDataSource("BTW", 35.0, baseTime.addMSecs(1000));
    graphgrid->addDataPointToDataSource("RTW", 45.0, baseTime.addMSecs(1000));
    
    // Add multiple data points to a specific data source
    std::vector<qreal> additionalYData = {30.0, 40.0, 50.0};
    std::vector<QDateTime> additionalTimestamps = {
        baseTime.addMSecs(1500),
        baseTime.addMSecs(2000),
        baseTime.addMSecs(2500)
    };
    graphgrid->addDataPointsToDataSource("LTW", additionalYData, additionalTimestamps);
    
    // Set data for a specific data source
    std::vector<qreal> newYData = {10.0, 20.0, 30.0, 40.0, 50.0};
    std::vector<QDateTime> newTimestamps;
    for (int i = 0; i < 5; ++i) {
        newTimestamps.push_back(baseTime.addMSecs(i * 1000));
    }
    graphgrid->setDataToDataSource("BTW", newYData, newTimestamps);
    
    // Demonstrate data source management
    qDebug() << "Available data sources:" << graphgrid->getDataSourceLabels().size();
    qDebug() << "Has LTW data source:" << graphgrid->hasDataSource("LTW");
    qDebug() << "Has INVALID data source:" << graphgrid->hasDataSource("INVALID");
    
    // Get a data source and check its data
    WaterfallData* ltwData = graphgrid->getDataSource("LTW");
    if (ltwData) {
        qDebug() << "LTW data size:" << ltwData->getDataSize();
        qDebug() << "LTW Y range:" << ltwData->getMinY() << "to" << ltwData->getMaxY();
    }
    
    qDebug() << "Data source methods demonstrated successfully";
}

