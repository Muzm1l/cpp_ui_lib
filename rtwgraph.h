#ifndef RTWGRAPH_H
#define RTWGRAPH_H

#include "waterfallgraph.h"
#include "rtwsymboldrawing.h"
#include "waterfalldata.h"  // For RTWSymbolData
#include "rulerstate.h"
#include <QDateTime>
#include <vector>
#include <array>

/**
 * @brief RTW Graph component that inherits from waterfallgraph
 *
 * This component creates scatterplots by default and can be extended
 * for specific RTW (Rate Time Waterfall) functionality.
 */
class RTWGraph : public WaterfallGraph
{
    Q_OBJECT

public:
    explicit RTWGraph(QWidget *parent = nullptr, bool enableGrid = false, int gridDivisions = 10, TimeInterval timeInterval = TimeInterval::FifteenMinutes);
    ~RTWGraph();

    /**
     * @brief Add an RTW symbol to the graph
     *
     * @param symbolName Name of the symbol (e.g., "TM", "DP", "LY", "CircleI", etc.)
     * @param timestamp Timestamp when the symbol should be displayed
     * @param range Range value (Y-axis position) where the symbol should be displayed
     */
    void addRTWSymbol(const QString &symbolName, const QDateTime &timestamp, qreal range);

    // ========== Ruler indicator API ==========
    // The main system owns up to 4 rulers (1-4). Each active ruler is drawn as a
    // numbered circle; at most one ruler may be selected at a time.
    static constexpr int RulerCount = 4;

    /**
     * @brief Activate/position a ruler indicator (0-based index 0..3).
     * @param index Ruler index (0..3)
     * @param timestamp Time-axis position of the ruler
     * @param range Range-axis position of the ruler
     */
    void setRulerActive(int index, const QDateTime &timestamp, qreal range);

    /** @brief Deactivate a single ruler (removes it from the graph). */
    void clearRuler(int index);

    /** @brief Deactivate all rulers and clear selection. */
    void clearAllRulers();

    /**
     * @brief Select a ruler (turns it yellow); deselects all others.
     * @param index Ruler index (0..3), or -1 to clear the selection.
     *              Selecting an inactive ruler is ignored.
     */
    void setSelectedRuler(int index);

    /** @brief Index of the currently selected ruler, or -1 if none. */
    int selectedRuler() const { return m_selectedRuler; }

    /** @brief Whether the given ruler index is currently active. */
    bool isRulerActive(int index) const;

protected:
    // Override the draw method to create scatterplots by default
    void draw() override;
    void refreshOverlaysAfterVisibleTimeRangeChange() override;
    void augmentOverlayPassAfterSymbols() override;

    // Override mouse event handlers if needed
    void onMouseClick(const QPointF &scenePos) override;
    void onMouseDrag(const QPointF &scenePos) override;

private:
    // RTW-specific properties and methods can be added here
    void drawRTWScatterplot();
    void drawCustomRMarkers();
    void drawRTWSymbols();
    RTWSymbolDrawing::SymbolType symbolNameToType(const QString &symbolName) const;

    // Ruler rendering and helpers
    void drawRulers();
    void removeRulerItems();
    RTWSymbolDrawing::SymbolType rulerSymbolType(int index, bool selected) const;

    // RTW symbol drawing utility (symbols are stored in WaterfallData)
    RTWSymbolDrawing symbols;

    // Ruler indicator state (view-local; driven by the main system via GraphLayout)
    std::array<RulerState, RulerCount> m_rulers{};
    int m_selectedRuler = -1;  // 0..3, or -1 for none

signals:
    /**
     * @brief Emitted when an R marker is clicked
     * @param timestamp The timestamp of the clicked R marker
     * @param position The scene position where the marker was clicked
     */
    void rMarkerTimestampCaptured(const QDateTime &timestamp, const QPointF &position);

    /**
     * @brief Emitted when an RTW symbol is clicked
     * @param timestamp The timestamp of the clicked RTW symbol
     * @param position The scene position where the symbol was clicked
     * @param symbolName The name of the clicked symbol
     */
    void rtwSymbolTimestampCaptured(const QDateTime &timestamp, const QPointF &position, const QString &symbolName);

    /**
     * @brief Emitted when a ruler indicator is clicked.
     * Does not change selection; use setSelectedRuler() via GraphLayout for that.
     */
    void rulerClicked(int index, const QDateTime &timestamp, qreal range);
};

#endif // RTWGRAPH_H
