#ifndef RTWGRAPH_H
#define RTWGRAPH_H

#include "waterfallgraph.h"
#include "rtwsymbols.h"
#include <QDateTime>
#include <vector>

/**
 * @brief Structure to store RTW symbol data
 */
struct RTWSymbolData
{
    QString symbolName;
    QDateTime timestamp;
    qreal range;
};

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
    RTWSymbols::SymbolType symbolNameToType(const QString &symbolName) const;

    // Storage for RTW symbols
    std::vector<RTWSymbolData> rtwSymbols;
    RTWSymbols symbols;
};

#endif // RTWGRAPH_H
