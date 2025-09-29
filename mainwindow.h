#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <cstdlib>
#include <ctime>
#include "timeselectionvisualizer.h"
#include "timelineview.h"
#include "zoompanel.h"
#include "graphlayout.h"
#include "waterfallgraph.h"
#include "waterfalldata.h"
#include "customwaterfallgraph.h"
#include "fdwgraph.h"
#include "bdwgraph.h"
#include "brwgraph.h"
#include "ltwgraph.h"
#include "btwgraph.h"
#include "rtwgraph.h"
#include "ftwgraph.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    QTimer *timer; ///< Timer for simulation updates
    QTimer *timeUpdateTimer; ///< Timer for updating current time
    
    GraphLayout *graphgrid; ///< Graph layout widget
    
    // New graph components for the second tab
    FDWGraph *fdwGraph; ///< FDW Graph component
    BDWGraph *bdwGraph; ///< BDW Graph component
    BRWGraph *brwGraph; ///< BRW Graph component
    LTWGraph *ltwGraph; ///< LTW Graph component
    BTWGraph *btwGraph; ///< BTW Graph component
    RTWGraph *rtwGraph; ///< RTW Graph component
    FTWGraph *ftwGraph; ///< FTW Graph component
    
    // void configureTimeVisualizer();
    // void configureTimelineView();
    // void configureZoomPanel();
    // void updateTimeline();
    void configureLayoutSelection();
    void demonstrateDataPointMethods();
    void setupCustomGraphsTab();
    void setupNewGraphData();
    qreal generateRandomValue(qreal oldValue, qreal deltaValue);

    long simTick;

    // Sensor Bearing
    qreal currentSensorBearing;
    qreal prevSensorBearing;

    // Own Ship Info
    qreal currentOwnShipBearing;
    qreal currentShipSpeed;
    qreal prevOwnShipBearing;
    qreal prevShipSpeed;

    // Selected Track Info
    qreal currentSelectedTrackRange;
    qreal currentSelectedTrackBearing;
    qreal currentSelectedTrackSpeed;
    qreal currentSelectedTrackCourse;

    qreal prevSelectedTrackRange;
    qreal prevSelectedTrackBearing;
    qreal prevSelectedTrackSpeed;
    qreal prevSelectedTrackCourse;

    // Adopted Track Info
    qreal currentAdoptedTrackRange;
    qreal currentAdoptedTrackBearing;
    qreal currentAdoptedTrackSpeed;
    qreal currentAdoptedTrackCourse;

    qreal prevAdoptedTrackRange;
    qreal prevAdoptedTrackBearing;
    qreal prevAdoptedTrackSpeed;
    qreal prevAdoptedTrackCourse;

    // Current values for each graph type (for simulation)
    qreal currentFDWValue;  // Frequency Domain Window: 8.0-30.0 (range: 22.0)
    qreal currentBDWValue;  // Bandwidth Domain Window: 5.0-38.0 (range: 33.0)
    qreal currentBRWValue;  // Bit Rate Window: 8.0-30.0 (range: 22.0)
    qreal currentLTWValue;  // Left Track Window: 15.0-30.0 (range: 15.0)
    qreal currentBTWValue;  // Bottom Track Window: 5.0-40.0 (range: 35.0)
    qreal currentRTWValue;  // Right Track Window: 12.0-28.0 (range: 16.0)
    qreal currentFTWValue;  // Frequency Time Window: 15.0-30.0 (range: 15.0)

private slots:
    /**
     * @brief Updates simulation state every timer interval
     *
     * Called every 2 seconds to update target position, bearing, range,
     * and bearing rate calculations. Triggers widget repaint.
     */
    void updateSimulation();
    
    /**
     * @brief Handles layout type changes from the combobox
     *
     * Called when user selects a different layout type from the combobox.
     */
    void onLayoutTypeChanged(int index);
    
    // /**
    //  * @brief Updates the current time in the time visualizer
    //  *
    //  * Called every second to update the current time to system time.
    //  */
    // void updateCurrentTime();
    
};

#endif // MAINWINDOW_H
