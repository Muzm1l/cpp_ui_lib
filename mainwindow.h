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
#include "scwwindow.h"
#include "scwsimulator.h"
#include <QMainWindow>
#include <QTimer>
#include <QPaintEvent>
#include <QPushButton>
#include <vector>
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
    QLabel* timelineModeLabel; ///< Label to display current timeline mode (FOLLOW_MODE or FROZEN_MODE)

    // Manoeuvre management buttons
    QPushButton* addManoeuvreButton; ///< Button to add a manoeuvre to the graph layout
    QPushButton* clearManoeuvresButton; ///< Button to clear all manoeuvres from the graph layout

    
    // RTW Symbols test widget
    QWidget* rtwSymbolsTestWidget; ///< Widget for testing RTW symbols
    
    // Time selection history storage (max 5 selections)
    std::vector<TimeSelectionSpan> timeSelectionHistory; ///< Vector to store up to 5 time selection timestamps
        
    // SCWWindow for SCW tab
    SCWWindow* scwWindow; ///< SCW Window widget
    SCWSimulator* scwSimulator; ///< Simulator for SCWWindow data generation

    // void configureTimeVisualizer();
    // void configureTimelineView();
    void configureZoomPanel();
    // void updateTimeline();
    void configureLayoutSelection();
    void demonstrateDataPointMethods();
    void setupCustomGraphsTab();
    void setupTestWaterfallGraph(); ///< Setup test WaterfallGraph in controls tab
    void setupTimelineView(); ///< Setup TimelineView in controls tab for testing
    void setupSCWWindow(); ///< Setup SCWWindow in a new tab
    void setupNewGraphData();
    void setBulkDataForAllGraphs();
    void initializeAllZoomPanelLimits();
    void setupRTWSymbolsTest(); ///< Setup RTW symbols test widget
    void setupTimeSelectionHistory(); ///< Setup time selection history storage
    void setupManoeuvreButton(); ///< Setup button to add manoeuvres

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
     * @brief Handles time selection created events
     *
     * Called when a time selection is created on the timeline.
     * Stores the selection timestamps in history (max 5).
     */
    void onTimeSelectionCreated(const TimeSelectionSpan &selection);

    /**
     * @brief Handles add manoeuvre button click
     *
     * Called when the add manoeuvre button is clicked.
     * Creates a sample manoeuvre and adds it to the graph layout.
     */
    void onAddManoeuvreButtonClicked();

    /**
     * @brief Handles clear manoeuvres button click
     *
     * Called when the clear manoeuvres button is clicked.
     * Clears all manoeuvres from the graph layout.
     */
    void onClearManoeuvresButtonClicked();

    // /**
    //  * @brief Updates the current time in the time visualizer
    //  *
    //  * Called every second to update the current time to system time.
    //  */
    // void updateCurrentTime();
};

#endif // MAINWINDOW_H
