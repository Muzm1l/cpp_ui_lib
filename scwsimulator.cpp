#include "scwsimulator.h"
#include <QRandomGenerator>
#include <QDateTime>

SCWSimulator::SCWSimulator(QObject *parent, QTimer *timer, SCWWindow *scwWindow)
    : QObject(parent), m_timer(timer), m_scwWindow(scwWindow), m_running(false)
{
    qDebug() << "SCWSimulator constructor called with timer:" << m_timer << "scwWindow:" << m_scwWindow;

    // Initialize current values
    initializeCurrentValues();

    // Connect timer to our tick handler
    if (m_timer)
    {
        connect(m_timer, &QTimer::timeout, this, &SCWSimulator::onTimerTick);
        connect(m_timer, &QObject::destroyed, this, [this]()
        {
            qDebug() << "SCWSimulator: Connected timer destroyed, clearing pointer";
            m_timer = nullptr;
            m_running = false; 
        });
        qDebug() << "SCWSimulator: Timer connected successfully";
    }
    else
    {
        qDebug() << "SCWSimulator: No timer provided!";
    }
}

SCWSimulator::~SCWSimulator()
{
    // Disconnect from timer first to prevent signal emission during destruction
    if (m_timer)
    {
        disconnect(m_timer, &QTimer::timeout, this, &SCWSimulator::onTimerTick);
    }
    stop();
}

void SCWSimulator::start()
{
    if (m_timer && !m_running)
    {
        // Check if the timer is still valid before calling start()
        try
        {
            if (m_timer->parent() != nullptr)
            {
                m_timer->start();
                m_running = true;
                qDebug() << "SCWSimulator started successfully";
            }
            else
            {
                qDebug() << "SCWSimulator start failed - timer parent is null";
            }
        }
        catch (...)
        {
            qDebug() << "SCWSimulator start failed - timer is invalid or being destroyed";
        }
    }
    else
    {
        qDebug() << "SCWSimulator start failed - timer:" << m_timer << "running:" << m_running;
    }
}

void SCWSimulator::stop()
{
    if (m_timer && m_running)
    {
        try
        {
            if (m_timer->parent() != nullptr)
            {
                m_timer->stop();
            }
        }
        catch (...)
        {
            // Timer was already deleted, just continue
        }
        m_running = false;
        qDebug() << "SCWSimulator stopped";
    }
    else if (m_running)
    {
        m_running = false;
        qDebug() << "SCWSimulator stopped (timer was null)";
    }
}

bool SCWSimulator::isRunning() const
{
    return m_running;
}

qreal SCWSimulator::generateRandomValue(qreal oldValue, qreal deltaValue)
{
    // Generate a random value between -deltaValue and +deltaValue
    qreal randomDelta = (QRandomGenerator::global()->generateDouble() - 0.5) * 2.0 * deltaValue;
    return oldValue + randomDelta;
}

void SCWSimulator::initializeCurrentValues()
{
    // Initialize current values to reasonable starting points
    // RULER series: 0-100 range
    m_currentRuler1Value = 25.0;
    m_currentRuler2Value = 50.0;
    m_currentRuler3Value = 75.0;
    m_currentRuler4Value = 100.0;

    // SCW_SERIES_B: 0-100 range
    m_currentBRATValue = 20.0;
    m_currentBOTValue = 40.0;
    m_currentBFTValue = 60.0;
    m_currentBOPTValue = 80.0;
    m_currentBOTCValue = 100.0;

    // SCW_SERIES_A: 0-50 range
    m_currentATMAValue = 15.0;
    m_currentATMAFValue = 35.0;

    // SCW_SERIES_E: 0-100 range
    m_currentEXTERNAL1Value = 10.0;
    m_currentEXTERNAL2Value = 30.0;
    m_currentEXTERNAL3Value = 50.0;
    m_currentEXTERNAL4Value = 70.0;
    m_currentEXTERNAL5Value = 90.0;
}

void SCWSimulator::updateValues()
{
    // Update all current values with random variations
    // RULER series: delta of 5.0
    m_currentRuler1Value = generateRandomValue(m_currentRuler1Value, 5.0);
    m_currentRuler2Value = generateRandomValue(m_currentRuler2Value, 5.0);
    m_currentRuler3Value = generateRandomValue(m_currentRuler3Value, 5.0);
    m_currentRuler4Value = generateRandomValue(m_currentRuler4Value, 5.0);

    // SCW_SERIES_B: delta of 5.0
    m_currentBRATValue = generateRandomValue(m_currentBRATValue, 5.0);
    m_currentBOTValue = generateRandomValue(m_currentBOTValue, 5.0);
    m_currentBFTValue = generateRandomValue(m_currentBFTValue, 5.0);
    m_currentBOPTValue = generateRandomValue(m_currentBOPTValue, 5.0);
    m_currentBOTCValue = generateRandomValue(m_currentBOTCValue, 5.0);

    // SCW_SERIES_A: delta of 3.0
    m_currentATMAValue = generateRandomValue(m_currentATMAValue, 3.0);
    m_currentATMAFValue = generateRandomValue(m_currentATMAFValue, 3.0);

    // SCW_SERIES_E: delta of 5.0
    m_currentEXTERNAL1Value = generateRandomValue(m_currentEXTERNAL1Value, 5.0);
    m_currentEXTERNAL2Value = generateRandomValue(m_currentEXTERNAL2Value, 5.0);
    m_currentEXTERNAL3Value = generateRandomValue(m_currentEXTERNAL3Value, 5.0);
    m_currentEXTERNAL4Value = generateRandomValue(m_currentEXTERNAL4Value, 5.0);
    m_currentEXTERNAL5Value = generateRandomValue(m_currentEXTERNAL5Value, 5.0);

    // Ensure values stay within reasonable bounds
    // RULER series: 0-100
    m_currentRuler1Value = qBound(0.0, m_currentRuler1Value, 100.0);
    m_currentRuler2Value = qBound(0.0, m_currentRuler2Value, 100.0);
    m_currentRuler3Value = qBound(0.0, m_currentRuler3Value, 100.0);
    m_currentRuler4Value = qBound(0.0, m_currentRuler4Value, 100.0);

    // SCW_SERIES_B: 0-100
    m_currentBRATValue = qBound(0.0, m_currentBRATValue, 100.0);
    m_currentBOTValue = qBound(0.0, m_currentBOTValue, 100.0);
    m_currentBFTValue = qBound(0.0, m_currentBFTValue, 100.0);
    m_currentBOPTValue = qBound(0.0, m_currentBOPTValue, 100.0);
    m_currentBOTCValue = qBound(0.0, m_currentBOTCValue, 100.0);

    // SCW_SERIES_A: 0-50
    m_currentATMAValue = qBound(0.0, m_currentATMAValue, 50.0);
    m_currentATMAFValue = qBound(0.0, m_currentATMAFValue, 50.0);

    // SCW_SERIES_E: 0-100
    m_currentEXTERNAL1Value = qBound(0.0, m_currentEXTERNAL1Value, 100.0);
    m_currentEXTERNAL2Value = qBound(0.0, m_currentEXTERNAL2Value, 100.0);
    m_currentEXTERNAL3Value = qBound(0.0, m_currentEXTERNAL3Value, 100.0);
    m_currentEXTERNAL4Value = qBound(0.0, m_currentEXTERNAL4Value, 100.0);
    m_currentEXTERNAL5Value = qBound(0.0, m_currentEXTERNAL5Value, 100.0);
}

void SCWSimulator::addDataPoints()
{
    if (!m_scwWindow)
    {
        qDebug() << "SCWSimulator: No SCWWindow provided";
        return;
    }

    QDateTime currentTime = QDateTime::currentDateTime();
    std::vector<QDateTime> timestamps = {currentTime};
    
    // Add data points for all RULER series (windows 1-4)
    std::vector<qreal> ruler1Data = {m_currentRuler1Value};
    std::vector<qreal> ruler2Data = {m_currentRuler2Value};
    std::vector<qreal> ruler3Data = {m_currentRuler3Value};
    std::vector<qreal> ruler4Data = {m_currentRuler4Value};
    
    m_scwWindow->addDataPoints(SCW_SERIES_R::RULER_1, ruler1Data, timestamps);
    m_scwWindow->addDataPoints(SCW_SERIES_R::RULER_2, ruler2Data, timestamps);
    m_scwWindow->addDataPoints(SCW_SERIES_R::RULER_3, ruler3Data, timestamps);
    m_scwWindow->addDataPoints(SCW_SERIES_R::RULER_4, ruler4Data, timestamps);

    // Add data points for all SCW_SERIES_B (window 5 cycles through these)
    std::vector<qreal> bratData = {m_currentBRATValue};
    std::vector<qreal> botData = {m_currentBOTValue};
    std::vector<qreal> bftData = {m_currentBFTValue};
    std::vector<qreal> boptData = {m_currentBOPTValue};
    std::vector<qreal> botcData = {m_currentBOTCValue};
    
    m_scwWindow->addDataPoints(SCW_SERIES_B::BRAT, bratData, timestamps);
    m_scwWindow->addDataPoints(SCW_SERIES_B::BOT, botData, timestamps);
    m_scwWindow->addDataPoints(SCW_SERIES_B::BFT, bftData, timestamps);
    m_scwWindow->addDataPoints(SCW_SERIES_B::BOPT, boptData, timestamps);
    m_scwWindow->addDataPoints(SCW_SERIES_B::BOTC, botcData, timestamps);

    // Add data points for all SCW_SERIES_A (window 6 cycles through these)
    std::vector<qreal> atmaData = {m_currentATMAValue};
    std::vector<qreal> atmafData = {m_currentATMAFValue};
    
    m_scwWindow->addDataPoints(SCW_SERIES_A::ATMA, atmaData, timestamps);
    m_scwWindow->addDataPoints(SCW_SERIES_A::ATMAF, atmafData, timestamps);

    // Add data points for all SCW_SERIES_E (window 7 cycles through these)
    std::vector<qreal> ext1Data = {m_currentEXTERNAL1Value};
    std::vector<qreal> ext2Data = {m_currentEXTERNAL2Value};
    std::vector<qreal> ext3Data = {m_currentEXTERNAL3Value};
    std::vector<qreal> ext4Data = {m_currentEXTERNAL4Value};
    std::vector<qreal> ext5Data = {m_currentEXTERNAL5Value};
    
    m_scwWindow->addDataPoints(SCW_SERIES_E::EXTERNAL1, ext1Data, timestamps);
    m_scwWindow->addDataPoints(SCW_SERIES_E::EXTERNAL2, ext2Data, timestamps);
    m_scwWindow->addDataPoints(SCW_SERIES_E::EXTERNAL3, ext3Data, timestamps);
    m_scwWindow->addDataPoints(SCW_SERIES_E::EXTERNAL4, ext4Data, timestamps);
    m_scwWindow->addDataPoints(SCW_SERIES_E::EXTERNAL5, ext5Data, timestamps);

    qDebug() << "SCWSimulator: Added data points - RULER1:" << m_currentRuler1Value
             << "RULER2:" << m_currentRuler2Value
             << "RULER3:" << m_currentRuler3Value
             << "RULER4:" << m_currentRuler4Value
             << "BRAT:" << m_currentBRATValue
             << "BOT:" << m_currentBOTValue
             << "BFT:" << m_currentBFTValue
             << "BOPT:" << m_currentBOPTValue
             << "BOTC:" << m_currentBOTCValue
             << "ATMA:" << m_currentATMAValue
             << "ATMAF:" << m_currentATMAFValue
             << "EXTERNAL1:" << m_currentEXTERNAL1Value
             << "EXTERNAL2:" << m_currentEXTERNAL2Value
             << "EXTERNAL3:" << m_currentEXTERNAL3Value
             << "EXTERNAL4:" << m_currentEXTERNAL4Value
             << "EXTERNAL5:" << m_currentEXTERNAL5Value;
}

void SCWSimulator::onTimerTick()
{
    qDebug() << "SCWSimulator::onTimerTick() called";
    updateValues();
    addDataPoints();
}

