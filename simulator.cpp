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

void Simulator::generateBulkData(int numPoints)
{
    if (!m_graphLayout) {
        return;
    }

    QDateTime currentTime = QDateTime::currentDateTime();
    std::vector<QDateTime> timestamps;
    std::vector<qreal> fdwData, bdwData, brwData, ltwData, btwData, rtwData, ftwData;

    // Generate timestamps and data for each point
    for (int i = 0; i < numPoints; ++i) {
        QDateTime timestamp = currentTime.addSecs(i * 60); // 1 minute intervals
        timestamps.push_back(timestamp);

        double timeFactor = static_cast<double>(i) / numPoints;

        // FDW: Frequency Domain Window - sine wave pattern
        fdwData.push_back(19.0 + 11.0 * std::sin(timeFactor * 2 * M_PI) + (std::rand() % 100 - 50) / 100.0);

        // BDW: Bandwidth Domain Window - cosine wave pattern
        bdwData.push_back(21.5 + 16.5 * std::cos(timeFactor * 2 * M_PI) + (std::rand() % 100 - 50) / 100.0);

        // BRW: Bit Rate Window - sawtooth pattern
        brwData.push_back(19.0 + 11.0 * (timeFactor - std::floor(timeFactor)) + (std::rand() % 100 - 50) / 100.0);

        // LTW: Left Track Window - linear pattern
        ltwData.push_back(22.5 + 7.5 * (timeFactor - 0.5) + (std::rand() % 100 - 50) / 100.0);

        // BTW: Bottom Track Window - exponential pattern
        btwData.push_back(22.5 + 17.5 * std::exp(-timeFactor * 2) + (std::rand() % 100 - 50) / 100.0);

        // RTW: Right Track Window - logarithmic pattern
        rtwData.push_back(20.0 + 8.0 * std::log(timeFactor * 10 + 1) + (std::rand() % 100 - 50) / 100.0);

        // FTW: Frequency Time Window - quadratic pattern
        ftwData.push_back(22.5 + 2.0 * (timeFactor - 0.5) * (timeFactor - 0.5) * 8 + (std::rand() % 100 - 50) / 100.0);
    }

    // Set hard range limits for each graph
    m_graphLayout->setHardRangeLimits(GraphType::FDW, 2, 102);
    m_graphLayout->setHardRangeLimits(GraphType::BDW, 2, 102);
    m_graphLayout->setHardRangeLimits(GraphType::BRW, 2, 102);
    m_graphLayout->setHardRangeLimits(GraphType::LTW, 2, 102);
    m_graphLayout->setHardRangeLimits(GraphType::BTW, 2, 102);
    m_graphLayout->setHardRangeLimits(GraphType::RTW, 2, 102);
    m_graphLayout->setHardRangeLimits(GraphType::FTW, 2, 102);

    // Add bulk data to each graph data source
    m_graphLayout->addDataPointsToDataSource(GraphType::FDW, "FDW-1", fdwData, timestamps);
    m_graphLayout->addDataPointsToDataSource(GraphType::BDW, "BDW-1", bdwData, timestamps);
    m_graphLayout->addDataPointsToDataSource(GraphType::BRW, "BRW-1", brwData, timestamps);
    m_graphLayout->addDataPointsToDataSource(GraphType::LTW, "LTW-1", ltwData, timestamps);
    m_graphLayout->addDataPointsToDataSource(GraphType::BTW, "BTW-1", btwData, timestamps);
    m_graphLayout->addDataPointsToDataSource(GraphType::RTW, "RTW-1", rtwData, timestamps);
    m_graphLayout->addDataPointsToDataSource(GraphType::FTW, "FTW-1", ftwData, timestamps);

    qDebug() << "FDW range:" << *std::min_element(fdwData.begin(), fdwData.end()) << "to" << *std::max_element(fdwData.begin(), fdwData.end());
    qDebug() << "BDW range:" << *std::min_element(bdwData.begin(), bdwData.end()) << "to" << *std::max_element(bdwData.begin(), bdwData.end());
    qDebug() << "BRW range:" << *std::min_element(brwData.begin(), brwData.end()) << "to" << *std::max_element(brwData.begin(), brwData.end());
    qDebug() << "LTW range:" << *std::min_element(ltwData.begin(), ltwData.end()) << "to" << *std::max_element(ltwData.begin(), ltwData.end());
    qDebug() << "BTW range:" << *std::min_element(btwData.begin(), btwData.end()) << "to" << *std::max_element(btwData.begin(), btwData.end());
    qDebug() << "RTW range:" << *std::min_element(rtwData.begin(), rtwData.end()) << "to" << *std::max_element(rtwData.begin(), rtwData.end());
    qDebug() << "FTW range:" << *std::min_element(ftwData.begin(), ftwData.end()) << "to" << *std::max_element(ftwData.begin(), ftwData.end());
}

void Simulator::onTimerTick()
{
    qDebug() << "Simulator::onTimerTick() called";
    updateValues();
    addDataPoints();
}

void Simulator::initializeConfigurations()
{
    // Initialize configuration for each graph type
    m_fdwConfig = {8.0, 30.0, 19.0, 2.2};  // Frequency Domain Window: 10% of 22.0 range
    m_bdwConfig = {5.0, 38.0, 21.5, 3.3};  // Bandwidth Domain Window: 10% of 33.0 range
    m_brwConfig = {8.0, 30.0, 19.0, 2.2};  // Bit Rate Window: 10% of 22.0 range
    m_ltwConfig = {15.0, 30.0, 22.5, 1.5}; // Left Track Window: 10% of 15.0 range
    m_btwConfig = {5.0, 40.0, 22.5, 3.5};  // Bottom Track Window: 10% of 35.0 range
    m_rtwConfig = {12.0, 28.0, 20.0, 1.6}; // Right Track Window: 10% of 16.0 range
    m_ftwConfig = {15.0, 30.0, 22.5, 1.5}; // Frequency Time Window: 10% of 15.0 range
}

void Simulator::initializeCurrentValues()
{
    // Initialize current values to middle of their respective ranges
    m_currentFDWValue = 19.0; // Middle of 8.0-30.0 range
    m_currentBDWValue = 21.5; // Middle of 5.0-38.0 range
    m_currentBRWValue = 19.0; // Middle of 8.0-30.0 range
    m_currentLTWValue = 22.5; // Middle of 15.0-30.0 range
    m_currentBTWValue = 22.5; // Middle of 5.0-40.0 range
    m_currentRTWValue = 20.0; // Middle of 12.0-28.0 range
    m_currentFTWValue = 22.5; // Middle of 15.0-30.0 range
}
