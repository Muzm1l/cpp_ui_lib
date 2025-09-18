#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), timer(new QTimer(this)), timeUpdateTimer(new QTimer(this))
{
    ui->setupUi(this);

    

    // inside MainWindow constructor
    std::srand(std::time(nullptr));

    this->simTick = 0;
    // Set up timer for simulation updates (every 2 seconds)
    connect(timer, &QTimer::timeout, this, &MainWindow::updateSimulation);
    timer->start(2000); // 2000ms = 2 seconds
    
    // Set up timer for current time updates (every second)
    connect(timeUpdateTimer, &QTimer::timeout, this, &MainWindow::updateCurrentTime);
    timeUpdateTimer->start(1000); // 1000ms = 1 second

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

    // Configure TimeSelectionVisualizer
    configureTimeVisualizer();
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
    qDebug() << "Sim Tick: " << this->simTick;

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

void MainWindow::configureTimeVisualizer()
{
    // Set timeline length to 1 hour (60 minutes)
    ui->timeVisualizer->setTimeLineLength(QTime(1, 0, 0));
    
    // Set initial current time to system time
    ui->timeVisualizer->setCurrentTime(QTime::currentTime());
    
    // Add some sample time selections relative to current time
    QTime currentTime = QTime::currentTime();
    
    // Create selections that are within the visible range (last hour)
    // Selection 1: 5-10 minutes ago (should be visible)
    QTime start1 = currentTime.addSecs(-10 * 60); // 10 minutes ago
    QTime end1 = currentTime.addSecs(-5 * 60);   // 5 minutes ago
    TimeSelectionSpan span1(start1, end1);
    
    // Selection 2: 20-25 minutes ago (should be visible)
    QTime start2 = currentTime.addSecs(-25 * 60); // 25 minutes ago
    QTime end2 = currentTime.addSecs(-20 * 60);   // 20 minutes ago
    TimeSelectionSpan span2(start2, end2);
    
    // Selection 3: 35-45 minutes ago (should be visible)
    QTime start3 = currentTime.addSecs(-45 * 60); // 45 minutes ago
    QTime end3 = currentTime.addSecs(-35 * 60);   // 35 minutes ago
    TimeSelectionSpan span3(start3, end3);
    
    // Selection 4: 2-3 minutes ago (should be visible)
    QTime start4 = currentTime.addSecs(-3 * 60); // 3 minutes ago
    QTime end4 = currentTime.addSecs(-2 * 60);   // 2 minutes ago
    TimeSelectionSpan span4(start4, end4);
    
    
    ui->timeVisualizer->addTimeSelection(span1);
    ui->timeVisualizer->addTimeSelection(span2);
    ui->timeVisualizer->addTimeSelection(span3);
    ui->timeVisualizer->addTimeSelection(span4);
}

void MainWindow::updateCurrentTime()
{
    // Update the current time to the system time
    ui->timeVisualizer->setCurrentTime(QTime::currentTime());
}

