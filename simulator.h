#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QDebug>
#include "graphlayout.h"
#include "graphtype.h"
#include "waterfalldata.h"
#include <map>
#include <vector>
#include <cmath>
#include <algorithm>
#include <random>


/**
 * @brief Simulator configuration
 * 
 */
struct SimulatorConfig
{
    qreal minValue;           ///< Minimum value
    qreal maxValue;           ///< Maximum value
    qreal startValue;         ///< Start value
    qreal deltaValue;         ///< Delta value
};


/**
 * @brief The Simulator class handles the simulation of data for all graph types
 * 
 * This class manages the generation of random data points for different graph types
 * and updates the GraphLayout with new data at regular intervals.
 */
class Simulator : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Construct a new Simulator object
     * 
     * @param parent Parent QObject
     * @param timer Timer instance for data updates
     * @param graphLayout GraphLayout instance to update with data
     */
    explicit Simulator(QObject *parent, QTimer *timer, GraphLayout *graphLayout);

    /**
     * @brief Destroy the Simulator object
     */
    ~Simulator();

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

    /**
     * @brief Update all current values with new random data
     */
    void updateValues();

    /**
     * @brief Add current data points to the graph layout
     */
    void addDataPoints();

    /**
     * @brief Generate bulk data for all graph types
     * 
     * @param numPoints Number of data points to generate
     */
    static void generateBulkData(WaterfallData* data, SimulatorConfig config, int numPoints = 100);

    /**
     * @brief Static method to generate bulk data for WaterfallData instances
     * 
     * @param waterfallDataMap Map of GraphType to WaterfallData instances
     * @param configs Vector of SimulatorConfig instances (each contains GraphType)
     * @param numPoints Number of data points to generate
     */
    static void generateBulkDataForWaterfallData(
        std::map<WaterfallData* , SimulatorConfig> &waterfallDataMap,
        int numPoints = 100);

private slots:
    /**
     * @brief Timer tick handler - called when timer times out
     */
    void onTimerTick();

private:
    QTimer *m_timer;                    ///< Timer for data updates
    GraphLayout *m_graphLayout;         ///< GraphLayout to update with data
    bool m_running;                     ///< Whether simulation is running

    // Current values for each graph type
    qreal m_currentFDWValue;            ///< Current FDW value
    qreal m_currentBDWValue;            ///< Current BDW value
    qreal m_currentBRWValue;            ///< Current BRW value
    qreal m_currentLTWValue;            ///< Current LTW value
    qreal m_currentBTWValue;            ///< Current BTW value
    qreal m_currentRTWValue;            ///< Current RTW value
    qreal m_currentFTWValue;            ///< Current FTW value

    // Configuration for each graph type
    SimulatorConfig m_fdwConfig;            ///< FDW configuration
    SimulatorConfig m_bdwConfig;            ///< BDW configuration
    SimulatorConfig m_brwConfig;            ///< BRW configuration
    SimulatorConfig m_ltwConfig;            ///< LTW configuration
    SimulatorConfig m_btwConfig;            ///< BTW configuration
    SimulatorConfig m_rtwConfig;            ///< RTW configuration
    SimulatorConfig m_ftwConfig;            ///< FTW configuration

    /**
     * @brief Initialize default configurations for all graph types
     */
    void initializeConfigurations();

    /**
     * @brief Initialize current values for all graph types
     */
    void initializeCurrentValues();
};

#endif // SIMULATOR_H
