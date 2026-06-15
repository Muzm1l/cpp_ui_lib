#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "bdwgraph.h"
#include "brwgraph.h"
#include "btwgraph.h"  // For BTWGraph::HorizontalLineMode enum
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
#include "btwsymboldrawing.h"
#include "scwwindow.h"
#include "scwsimulator.h"
#include <QMainWindow>
#include <QTimer>
#include <QPaintEvent>
#include <QPushButton>
#include <QLabel>
#include <QList>
#include <vector>
#include <cstdlib>
#include <ctime>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class QGridLayout;

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
    Simulator* simulator = nullptr;   ///< Simulator for generating data (disabled by default)

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
    QLabel* markerTimestampLabel; ///< Label to display marker timestamp in first tab
    QLabel* rtwSymbolTimestampLabel; ///< Label to display RTW symbol timestamp when clicked
    QLabel* rtwRMarkerTimestampLabel; ///< Label to display RTW R marker timestamp when clicked
    QLabel* rtwRulerStatusLabel = nullptr; ///< Label showing RTW ruler test / selection state

    // Manoeuvre management buttons
    QPushButton* addManoeuvreButton; ///< Button to add a manoeuvre to the graph layout
    QPushButton* clearManoeuvresButton; ///< Button to clear all manoeuvres from the graph layout
    QPushButton* startManoeuvreButton; ///< Button to start drawing a manoeuvre (new API)
    QPushButton* endManoeuvreButton; ///< Button to end drawing a manoeuvre (new API)
    QPushButton* btwLineModeButton; ///< Button to toggle BTW horizontal line mode (Normal/DrawLine/DeleteLine)
    QPushButton* clearAllGraphsButton; ///< Button to test clearAllGraphs API
    QPushButton* redrawGraphsButton; ///< Button to manually trigger redraw of all graphs
    QPushButton* testEmptyDataButton; ///< Button to test setDataToDataSource with empty data for FTW and FDW
    QPushButton* clearFTWFDWButton; ///< Button to test clearGraph API for FTW and FDW
    QPushButton* showHistorySelectionsButton; ///< Test button to show timeframe of history selections
    QPushButton* refreshVisibleGraphsButton = nullptr; ///< Button to refresh the on-screen graph names list
    QLabel* visibleGraphsLabel = nullptr; ///< Label showing names of graphs currently shown on screen
    QPushButton* testRtwRulersButton = nullptr; ///< Button to exercise the RTW ruler API
    QPushButton* clearRtwRulersButton = nullptr; ///< Button to clear all RTW rulers

    // Controls are duplicated across the Original View panel and a dedicated "Controls"
    // tab. These lists let the shared updaters keep every copy in sync.
    QList<QPushButton*> m_btwLineModeButtons;   ///< All BTW-mode toggle buttons (kept in sync)
    QList<QLabel*> m_visibleGraphsLabels;       ///< All "graphs on screen" labels (kept in sync)
    QList<QLabel*> m_rtwRulerStatusLabels;     ///< All RTW ruler status labels (kept in sync)
    
    // BTW horizontal line mode state
    BTWGraph::HorizontalLineMode m_currentBTWLineMode; ///< Current BTW horizontal line mode

    
    // RTW Symbols test widget
    QWidget* rtwSymbolsTestWidget; ///< Widget for testing RTW symbols
    QWidget* btwSymbolsTestWidget; ///< Widget for testing BTW symbols
    QLabel* btwRulerApiStatusLabel = nullptr; ///< Status label on the BTW Rulers API tab
    
    // Time selection history storage (max 5 selections)
    std::vector<TimeSelectionSpan> timeSelectionHistory; ///< Vector to store up to 5 time selection timestamps
        
    // SCWWindow for SCW tab (disabled by default)
    SCWWindow* scwWindow = nullptr; ///< SCW Window widget
    SCWSimulator* scwSimulator = nullptr; ///< Simulator for SCWWindow data generation

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
    void setupBTWSymbolsTest(); ///< Setup BTW symbols gallery tab
    void setupBtwRulersApiTestTab(); ///< Dedicated tab to exercise BTW ruler (numbered circle) API
    void testBTWSymbolsAPI();   ///< Place predefined BTW symbols on the live BTW graph
    void setupTimeSelectionHistory(); ///< Setup time selection history storage
    void setupManoeuvreButton(); ///< Setup button to add manoeuvres
    void updateBTWLineModeButton(); ///< Update BTW line mode button text and style
    void setupVisibleGraphsWidget(); ///< Setup widget showing names of graphs currently on screen
    void updateVisibleGraphsWidget(); ///< Refresh the on-screen graph names via GraphLayout::getVisibleGraphNames()
    void setupRtwRulersTest(); ///< Add RTW ruler API test controls to the Original View panel
    void updateRtwRulerStatusLabel(const QString &text); ///< Update all RTW ruler status labels
    void setupControlsTab(); ///< Create a dedicated "Controls" tab with a spread-out duplicate of the controls
    void buildControlsInto(QWidget* host, QGridLayout* layout, bool spread); ///< Build the control buttons + graphs panel into a grid

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
    void onTimeSelectionModified(int index, const TimeSelectionSpan &newSpan);
    void onShowHistorySelectionsButtonClicked();

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

    /**
     * @brief Handles start manoeuvre drawing button click
     *
     * Called when the start manoeuvre button is clicked.
     * Starts a new manoeuvre drawing session with current time and random parameters.
     */
    void onStartManoeuvreButtonClicked();

    /**
     * @brief Handles end manoeuvre drawing button click
     *
     * Called when the end manoeuvre button is clicked.
     * Completes the current manoeuvre drawing session with current time.
     */
    void onEndManoeuvreButtonClicked();
    
    /**
     * @brief Handles BTW line mode toggle button click
     *
     * Called when the BTW line mode button is clicked.
     * Cycles through Normal -> DrawLine -> DeleteLine -> Normal
     */
    void onBTWLineModeButtonClicked();

    /** @brief Activate sample RTW rulers via GraphLayout API. */
    void onTestRtwRulersButtonClicked();

    /** @brief Clear all RTW rulers via GraphLayout API. */
    void onClearRtwRulersButtonClicked();

    /** @brief Activate sample BTW rulers via GraphLayout API. */
    void onTestBtwRulersButtonClicked();

    /** @brief Clear all BTW rulers via GraphLayout API. */
    void onClearBtwRulersButtonClicked();

    // /**
    //  * @brief Updates the current time in the time visualizer
    //  *
    //  * Called every second to update the current time to system time.
    //  */
    // void updateCurrentTime();
};

#endif // MAINWINDOW_H
