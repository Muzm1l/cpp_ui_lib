#ifndef CUSTOMWATERFALLGRAPH_H
#define CUSTOMWATERFALLGRAPH_H

#include "waterfallgraph.h"
#include <QColor>
#include <QPointF>

/**
 * @brief Custom waterfall graph component that inherits from waterfallgraph
 * 
 * This class provides a base for creating custom graph visualizations by
 * overriding the drawing methods. It allows for specialized rendering
 * while maintaining all the core functionality of the parent waterfallgraph.
 */
class CustomWaterfallGraph : public WaterfallGraph
{
    Q_OBJECT

public:
    /**
     * @brief Construct a new CustomWaterfallGraph object
     * 
     * @param parent Parent widget
     * @param enableGrid Whether to enable grid display
     * @param gridDivisions Number of grid divisions
     * @param timeInterval Time interval for the waterfall display
     */
    explicit CustomWaterfallGraph(QWidget *parent = nullptr, 
                                 bool enableGrid = false, 
                                 int gridDivisions = 10, 
                                 TimeInterval timeInterval = TimeInterval::FifteenMinutes);
    
    /**
     * @brief Destroy the CustomWaterfallGraph object
     */
    ~CustomWaterfallGraph();

protected:
    /**
     * @brief Override the main draw method to implement custom drawing logic
     * 
     * This method is called whenever the graph needs to be redrawn.
     * Override this to implement custom visualization logic.
     */
    virtual void draw() override;
    
    /**
     * @brief Override the data line drawing method for custom data visualization
     * 
     * This method is responsible for drawing the main data line/curve.
     * Override this to implement custom data visualization (e.g., bars, areas, etc.).
     */
    virtual void drawDataLine() override;
    
    /**
     * @brief Override the grid drawing method for custom grid visualization
     * 
     * This method is responsible for drawing the grid lines.
     * Override this to implement custom grid styles.
     */
    virtual void drawGrid() override;

public:
    /**
     * @brief Set custom drawing style for the graph
     * 
     * @param style Style identifier (can be extended for different graph types)
     */
    void setDrawingStyle(const QString& style);
    
    /**
     * @brief Get the current drawing style
     * 
     * @return QString Current drawing style
     */
    QString getDrawingStyle() const;
    
    /**
     * @brief Set custom colors for the graph elements
     * 
     * @param dataColor Color for data elements
     * @param gridColor Color for grid elements
     * @param backgroundColor Color for background
     */
    void setCustomColors(const QColor& dataColor, const QColor& gridColor, const QColor& backgroundColor);

protected:
    // Custom drawing helper methods
    void drawCustomDataArea();
    void drawCustomDataPoints();
    void drawCustomDataLine();
    void drawCustomGrid();
    
    // Style and color properties
    QString drawingStyle;
    QColor customDataColor;
    QColor customGridColor;
    QColor customBackgroundColor;
    
    // Custom drawing parameters
    bool useCustomColors;
    qreal barWidth;
    qreal pointSize;
    bool showDataPoints;
    bool showDataBars;
    bool showDataArea;
};

#endif // CUSTOMWATERFALLGRAPH_H
