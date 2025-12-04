#ifndef SCWSIMULATOR_H
#define SCWSIMULATOR_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QDebug>
#include "scwwindow.h"
#include <cmath>
#include <random>

/**
 * @brief The SCWSimulator class handles the simulation of data for SCWWindow graphs
 * 
 * This class manages the generation of random data points for all SCW series types
 * and updates the SCWWindow with new data every second.
 */
class SCWSimulator : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Construct a new SCWSimulator object
     * 
     * @param parent Parent QObject
     * @param timer Timer instance for data updates (should fire every second)
     * @param scwWindow SCWWindow instance to update with data
     */
    explicit SCWSimulator(QObject *parent, QTimer *timer, SCWWindow *scwWindow);

    /**
     * @brief Destroy the SCWSimulator object
     */
    ~SCWSimulator();

    /**
     * @brief Start the simulation
     */
    void start();

    /**
     * @brief Stop the simulation
     */
    void stop();

    /**
     * @brief Check if simulation is running
     * 
     * @return true if simulation is running
     */
    bool isRunning() const;

    /**
     * @brief Generate a random value based on previous value and delta
     * 
     * @param oldValue Previous value
     * @param deltaValue Maximum change allowed
     * @return New random value
     */
    qreal generateRandomValue(qreal oldValue, qreal deltaValue);

private slots:
    /**
     * @brief Timer tick handler - called when timer times out
     */
    void onTimerTick();

private:
    QTimer *m_timer;                    ///< Timer for data updates
    SCWWindow *m_scwWindow;             ///< SCWWindow to update with data
    bool m_running;                     ///< Whether simulation is running

    // Current values for each series type
    // SCW_SERIES_R (RULER series)
    qreal m_currentRuler1Value;
    qreal m_currentRuler2Value;
    qreal m_currentRuler3Value;
    qreal m_currentRuler4Value;

    // SCW_SERIES_B
    qreal m_currentBRATValue;
    qreal m_currentBOTValue;
    qreal m_currentBFTValue;
    qreal m_currentBOPTValue;
    qreal m_currentBOTCValue;

    // SCW_SERIES_A
    qreal m_currentATMAValue;
    qreal m_currentATMAFValue;

    // SCW_SERIES_E
    qreal m_currentEXTERNAL1Value;
    qreal m_currentEXTERNAL2Value;
    qreal m_currentEXTERNAL3Value;
    qreal m_currentEXTERNAL4Value;
    qreal m_currentEXTERNAL5Value;

    /**
     * @brief Initialize current values for all series types
     */
    void initializeCurrentValues();

    /**
     * @brief Update all current values with new random data
     */
    void updateValues();

    /**
     * @brief Add current data points to the SCWWindow
     */
    void addDataPoints();
};

#endif // SCWSIMULATOR_H

