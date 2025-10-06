#include "simulator.h"
#include <QRandomGenerator>
#include <QDateTime>

Simulator::Simulator(QObject *parent, QTimer *timer, GraphLayout *graphLayout)
    : QObject(parent), m_timer(timer), m_graphLayout(graphLayout), m_running(false)
{
    qDebug() << "Simulator constructor called with timer:" << m_timer << "graphLayout:" << m_graphLayout;
    
    // Initialize configurations and current values
    initializeConfigurations();
    initializeCurrentValues();

    // Connect timer to our tick handler
    if (m_timer) {
        connect(m_timer, &QTimer::timeout, this, &Simulator::onTimerTick);
        qDebug() << "Simulator: Timer connected successfully";
    } else {
        qDebug() << "Simulator: No timer provided!";
    }
}

Simulator::~Simulator()
{
    stop();
}

void Simulator::start()
{
    if (m_timer && !m_running) {
        m_timer->start();
        m_running = true;
        qDebug() << "Simulator started successfully";
    } else {
        qDebug() << "Simulator start failed - timer:" << m_timer << "running:" << m_running;
    }
}

void Simulator::stop()
{
    if (m_timer && m_running) {
        m_timer->stop();
        m_running = false;
        qDebug() << "Simulator stopped";
    }
}

bool Simulator::isRunning() const
{
    return m_running;
}

qreal Simulator::generateRandomValue(qreal oldValue, qreal deltaValue)
{
    // Generate a random value between -deltaValue and +deltaValue
    qreal randomDelta = (QRandomGenerator::global()->generateDouble() - 0.5) * 2.0 * deltaValue;
    return oldValue + randomDelta;
}

void Simulator::updateValues()
{
    // Update all current values with random variations
    m_currentFDWValue = generateRandomValue(m_currentFDWValue, m_fdwConfig.deltaValue);
    m_currentBDWValue = generateRandomValue(m_currentBDWValue, m_bdwConfig.deltaValue);
    m_currentBRWValue = generateRandomValue(m_currentBRWValue, m_brwConfig.deltaValue);
    m_currentLTWValue = generateRandomValue(m_currentLTWValue, m_ltwConfig.deltaValue);
    m_currentBTWValue = generateRandomValue(m_currentBTWValue, m_btwConfig.deltaValue);
    m_currentRTWValue = generateRandomValue(m_currentRTWValue, m_rtwConfig.deltaValue);
    m_currentFTWValue = generateRandomValue(m_currentFTWValue, m_ftwConfig.deltaValue);

    // Ensure values stay within bounds
    m_currentFDWValue = qBound(m_fdwConfig.minValue, m_currentFDWValue, m_fdwConfig.maxValue);
    m_currentBDWValue = qBound(m_bdwConfig.minValue, m_currentBDWValue, m_bdwConfig.maxValue);
    m_currentBRWValue = qBound(m_brwConfig.minValue, m_currentBRWValue, m_brwConfig.maxValue);
    m_currentLTWValue = qBound(m_ltwConfig.minValue, m_currentLTWValue, m_ltwConfig.maxValue);
    m_currentBTWValue = qBound(m_btwConfig.minValue, m_currentBTWValue, m_btwConfig.maxValue);
    m_currentRTWValue = qBound(m_rtwConfig.minValue, m_currentRTWValue, m_rtwConfig.maxValue);
    m_currentFTWValue = qBound(m_ftwConfig.minValue, m_currentFTWValue, m_ftwConfig.maxValue);
}

void Simulator::addDataPoints()
{
    if (!m_graphLayout) {
        return;
    }

    QDateTime currentTime = QDateTime::currentDateTime();

    // Add new data points to each graph data source with precise timestamping
    m_graphLayout->addDataPointToDataSource(GraphType::FDW, "FDW-1", m_currentFDWValue, currentTime);
    m_graphLayout->addDataPointToDataSource(GraphType::FDW, "FDW-2", m_currentFDWValue+10, currentTime);
    m_graphLayout->addDataPointToDataSource(GraphType::BDW, "BDW-1", m_currentBDWValue, currentTime);
    m_graphLayout->addDataPointToDataSource(GraphType::BDW, "BDW-2", m_currentBDWValue+10, currentTime);
    m_graphLayout->addDataPointToDataSource(GraphType::BRW, "BRW-1", m_currentBRWValue, currentTime);
    m_graphLayout->addDataPointToDataSource(GraphType::BRW, "BRW-2", m_currentBRWValue+10, currentTime);
    m_graphLayout->addDataPointToDataSource(GraphType::LTW, "LTW-1", m_currentLTWValue, currentTime);
    m_graphLayout->addDataPointToDataSource(GraphType::LTW, "LTW-2", m_currentLTWValue+10, currentTime);
    m_graphLayout->addDataPointToDataSource(GraphType::BTW, "BTW-1", m_currentBTWValue, currentTime);
    m_graphLayout->addDataPointToDataSource(GraphType::BTW, "BTW-2", m_currentBTWValue+10, currentTime);
    m_graphLayout->addDataPointToDataSource(GraphType::BTW, "BTW-3", m_currentBTWValue+10, currentTime);
    m_graphLayout->addDataPointToDataSource(GraphType::RTW, "RTW-1", m_currentRTWValue, currentTime);
    m_graphLayout->addDataPointToDataSource(GraphType::RTW, "RTW-2", m_currentRTWValue+10, currentTime);
    m_graphLayout->addDataPointToDataSource(GraphType::FTW, "FTW-1", m_currentFTWValue, currentTime);
    m_graphLayout->addDataPointToDataSource(GraphType::FTW, "FTW-2", m_currentFTWValue+10, currentTime);

    qDebug() << "Added data points - FDW:" << m_currentFDWValue
             << "BDW:" << m_currentBDWValue
             << "BRW:" << m_currentBRWValue
             << "LTW:" << m_currentLTWValue
             << "BTW:" << m_currentBTWValue
             << "RTW:" << m_currentRTWValue
             << "FTW:" << m_currentFTWValue;
}

void Simulator::generateBulkData(WaterfallData* data, SimulatorConfig config, int numPoints)
{
    if (!data) {
        qDebug() << "Simulator: Null WaterfallData pointer provided";
        return;
    }

    QDateTime currentTime = QDateTime::currentDateTime();
    std::vector<QDateTime> timestamps;
    std::vector<qreal> dataSeries;

    // Generate timestamps and data for each point
    // Generate data going backwards in time to fill the waterfall display
    // Use smaller intervals to fit more data within the 15-minute window
    for (int i = 0; i < numPoints; ++i) {
        QDateTime timestamp = currentTime.addSecs(-i * 60); // Go backwards in time, 10 second intervals
        timestamps.push_back(timestamp);

        double timeFactor = static_cast<double>(i) / numPoints;

        // Generate sine wave pattern
        qreal value = config.startValue + (config.maxValue - config.minValue) * 0.5 * std::sin(timeFactor * 2 * M_PI) + (std::rand() % 100 - 50) / 100.0;
        
        // Ensure value stays within bounds
        value = qBound(config.minValue, value, config.maxValue);
        dataSeries.push_back(value);
    }

    // Get the first series label from the data source
    std::vector<QString> seriesLabels = data->getDataSeriesLabels();
    if (seriesLabels.empty()) {
        qDebug() << "Simulator: No series labels found in WaterfallData";
        return;
    }

    QString firstSeriesLabel = seriesLabels[0];
    qDebug() << "Simulator: Adding bulk data to series:" << firstSeriesLabel;

    // Add the data to the first series
    data->addDataPointsToSeries(firstSeriesLabel, dataSeries, timestamps);

    qDebug() << "Generated data series range:" << *std::min_element(dataSeries.begin(), dataSeries.end()) 
             << "to" << *std::max_element(dataSeries.begin(), dataSeries.end())
             << "for series:" << firstSeriesLabel;
}

void Simulator::generateBulkDataForWaterfallData(
    std::map<WaterfallData* , SimulatorConfig> &waterfallDataMap,
    int numPoints)
{
    qDebug() << "Simulator: Generating bulk data for" << numPoints << "points using static method";
    
    // Go through each of the waterfallDataMap
    for (const auto& pair : waterfallDataMap) {
        WaterfallData* data = pair.first;
        SimulatorConfig config = pair.second;

        qDebug() << "Simulator: Processing WaterfallData with title:" << data->getDataTitle();
        
        // Generate the points based on the config
        generateBulkData(data, config, numPoints);
        
        // Verify data was added
        std::vector<QString> seriesLabels = data->getDataSeriesLabels();
        if (!seriesLabels.empty()) {
            QString firstSeries = seriesLabels[0];
            qDebug() << "Simulator: Verified data added to series" << firstSeries 
                     << "with" << data->getDataSeriesSize(firstSeries) << "points";
        }
    }
    
    qDebug() << "Simulator: Static bulk data generation completed";
}

void Simulator::onTimerTick()
{
    qDebug() << "Simulator::onTimerTick() called";
    updateValues();
    addDataPoints();
}

void Simulator::initializeConfigurations()
{
    // Initialize configuration for each graph type (Start, End, Start, Delta)
    m_fdwConfig = SimulatorConfig{8.0, 30.0, 19.0, 2.2};  // Frequency Domain Window: 10% of 22.0 range
    m_bdwConfig = SimulatorConfig{5.0, 38.0, 21.5, 3.3};  // Bandwidth Domain Window: 10% of 33.0 range
    m_brwConfig = SimulatorConfig{8.0, 30.0, 19.0, 2.2};  // Bit Rate Window: 10% of 22.0 range
    m_ltwConfig = SimulatorConfig{15.0, 30.0, 22.5, 1.5}; // Left Track Window: 10% of 15.0 range
    m_btwConfig = SimulatorConfig{5.0, 40.0, 22.5, 3.5};  // Bottom Track Window: 10% of 35.0 range
    m_rtwConfig = SimulatorConfig{12.0, 28.0, 20.0, 1.6}; // Right Track Window: 10% of 16.0 range
    m_ftwConfig = SimulatorConfig{15.0, 30.0, 22.5, 1.5}; // Frequency Time Window: 10% of 15.0 range
}

void Simulator::initializeCurrentValues()
{
    // Initialize current values to middle of their respective ranges
    m_currentFDWValue = m_fdwConfig.startValue; // Middle of 8.0-30.0 range
    m_currentBDWValue = m_bdwConfig.startValue; // Middle of 5.0-38.0 range
    m_currentBRWValue = m_brwConfig.startValue; // Middle of 8.0-30.0 range
    m_currentLTWValue = m_ltwConfig.startValue; // Middle of 15.0-30.0 range
    m_currentBTWValue = m_btwConfig.startValue; // Middle of 5.0-40.0 range
    m_currentRTWValue = m_rtwConfig.startValue; // Middle of 12.0-28.0 range
    m_currentFTWValue = m_ftwConfig.startValue; // Middle of 15.0-30.0 range
}
