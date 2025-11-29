#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "bdwgraph.h"
#include "brwgraph.h"
#include "btwgraph.h"
#include "fdwgraph.h"
#include "ftwgraph.h"
#include "graphcontainer.h"
#include "graphlayout.h"
#include "ltwgraph.h"
#include "rtwgraph.h"
#include "simulator.h"
#include "timelineview.h"
#include "timeselectionvisualizer.h"
#include "waterfalldata.h"
#include "waterfallgraph.h"
#include "zoompanel.h"
#include "rtwsymboldrawing.h"
#include <QMainWindow>
#include <QTimer>
#include <QPaintEvent>
#include <QPushButton>
#include <cstdlib>
#include <ctime>

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
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow* ui;

    QTimer* timer;           ///< Timer for simulation updates
    QTimer* timeUpdateTimer; ///< Timer for updating current time

    GraphLayout* graphgrid; ///< Graph layout widget
    Simulator* simulator;   ///< Simulator for generating data

    // New graph components for the second tab
    FDWGraph* fdwGraph; ///< FDW Graph component
    BDWGraph* bdwGraph; ///< BDW Graph component
    BRWGraph* brwGraph; ///< BRW Graph component
    LTWGraph* ltwGraph; ///< LTW Graph component
    BTWGraph* btwGraph; ///< BTW Graph component
    RTWGraph* rtwGraph; ///< RTW Graph component
    FTWGraph* ftwGraph; ///< FTW Graph component

    // WaterfallData objects for the second tab
    WaterfallData* fdwData; ///< FDW Data source
    WaterfallData* bdwData; ///< BDW Data source
    WaterfallData* brwData; ///< BRW Data source
    WaterfallData* ltwData; ///< LTW Data source
    WaterfallData* btwData; ///< BTW Data source
    WaterfallData* rtwData; ///< RTW Data source
    WaterfallData* ftwData; ///< FTW Data source
    
    // Test WaterfallGraph for controls tab
    WaterfallGraph* testWaterfallGraph; ///< Test graph for crosshair testing
    WaterfallData* testWaterfallData; ///< Test data for waterfall graph

    // TimelineView for controls tab testing
    TimelineView* testTimelineView; ///< Timeline view for testing slider functionality
    QLabel* timespanStartLabel; ///< Label to display start time
    QLabel* timespanEndLabel; ///< Label to display end time
    QLabel* timespanDurationLabel; ///< Label to display duration
    
    // RTW Symbols test widget
    QWidget* rtwSymbolsTestWidget; ///< Widget for testing RTW symbols
    
    // RTW R marker indicator button
    QPushButton* rtwMarkerIndicatorButton; ///< Button that lights up when R marker is clicked
    QTimer* rtwMarkerIndicatorTimer; ///< Timer to turn off the indicator after a few seconds
    
    // BTW manual marker indicator button
    QPushButton* btwMarkerIndicatorButton; ///< Button that lights up when BTW manual marker is clicked
    QTimer* btwMarkerIndicatorTimer; ///< Timer to turn off the indicator after a few seconds
    
    // void configureTimeVisualizer();
    // void configureTimelineView();
    void configureZoomPanel();
    // void updateTimeline();
    void configureLayoutSelection();
    void demonstrateDataPointMethods();
    void setupCustomGraphsTab();
    void setupTestWaterfallGraph(); ///< Setup test WaterfallGraph in controls tab
    void setupTimelineView(); ///< Setup TimelineView in controls tab for testing
    void setupNewGraphData();
    void setBulkDataForAllGraphs();
    void initializeAllZoomPanelLimits();
    void setupRTWSymbolsTest(); ///< Setup RTW symbols test widget
    void setupRTWMarkerIndicator(); ///< Setup RTW R marker indicator button
    void setupBTWMarkerIndicator(); ///< Setup BTW manual marker indicator button

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
    
    /**
     * @brief Handles RTW R marker click events
     *
     * Called when an R marker is clicked on an RTW graph.
     */
    void onRTWRMarkerClicked(const QDateTime &timestamp, const QPointF &position);
    
    /**
     * @brief Turns off the RTW marker indicator button
     *
     * Called by timer to reset the button after it lights up.
     */
    void turnOffRTWMarkerIndicator();
    
    /**
     * @brief Handles BTW manual marker click events
     *
     * Called when a BTW manual marker is clicked on a BTW graph.
     */
    void onBTWManualMarkerClicked(const QDateTime &timestamp, const QPointF &position);
    
    /**
     * @brief Turns off the BTW marker indicator button
     *
     * Called by timer to reset the button after it lights up.
     */
    void turnOffBTWMarkerIndicator();

    // /**
    //  * @brief Updates the current time in the time visualizer
    //  *
    //  * Called every second to update the current time to system time.
    //  */
    // void updateCurrentTime();
};

#endif // MAINWINDOW_H
