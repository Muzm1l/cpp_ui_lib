#ifndef RTWGRAPH_H
#define RTWGRAPH_H

#include "waterfallgraph.h"
#include "rtwsymboldrawing.h"
#include "waterfalldata.h"  // For RTWSymbolData
#include <QDateTime>
#include <vector>

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

protected:
    // Override the draw method to create scatterplots by default
    void draw() override;

    // Override mouse event handlers if needed
    void onMouseClick(const QPointF &scenePos) override;
    void onMouseDrag(const QPointF &scenePos) override;

private:
    // RTW-specific properties and methods can be added here
    void drawRTWScatterplot();
    void drawCustomRMarkers(const QString &seriesLabel);
    void drawRTWSymbols();
    RTWSymbolDrawing::SymbolType symbolNameToType(const QString &symbolName) const;

    // RTW symbol drawing utility (symbols are stored in WaterfallData)
    RTWSymbolDrawing symbols;

signals:
    /**
     * @brief Emitted when an R marker is clicked
     * @param timestamp The timestamp of the clicked R marker
     * @param position The scene position where the marker was clicked
     */
    void rMarkerTimestampCaptured(const QDateTime &timestamp, const QPointF &position);
};

#endif // RTWGRAPH_H
