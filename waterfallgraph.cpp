#include "waterfallgraph.h"

/**
 * @brief Construct a new waterfallgraph::waterfallgraph object
 * 
 * @param parent 
 * @param enableGrid 
 * @param gridDivisions 
 */
waterfallgraph::waterfallgraph(QWidget *parent, bool enableGrid, int gridDivisions)
    : QWidget(parent)
    , graphicsView(nullptr)
    , graphicsScene(nullptr)
    , gridEnabled(enableGrid)
    , gridDivisions(gridDivisions)
    , isDragging(false)
{
    // Remove all margins and padding for snug fit
    setContentsMargins(0, 0, 0, 0);
    
    // Set black background
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::black);
    setPalette(pal);
    setAutoFillBackground(true);
    
    // Set fixed size constraints
    setMaximumSize(800, 400); // Set both max width and height
    setFixedHeight(350); // Force a fixed height
    
    // Initialize scene
    graphicsScene = new QGraphicsScene(this);
    graphicsScene->setBackgroundBrush(QBrush(Qt::black));
    
    // Create graphics view
    graphicsView = new QGraphicsView(graphicsScene, this);
    graphicsView->setRenderHint(QPainter::Antialiasing);
    graphicsView->setDragMode(QGraphicsView::NoDrag); // We'll handle our own mouse events
    
    // Set black background for view
    graphicsView->setBackgroundBrush(QBrush(Qt::black));
    
    // Disable scrollbars
    graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    // Ensure the view fits the scene exactly
    graphicsView->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    graphicsView->setFrameStyle(QFrame::NoFrame);
    
    // Set size policy for graphics view to fill the widget
    graphicsView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Set up layout to make the graphics view fill the widget with no margins
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(graphicsView);
    setLayout(layout);
    
    // Size policy will be set by parent container
    
    // Enable mouse tracking
    setMouseTracking(true);
    
    // Initial setup will happen in showEvent
    qDebug() << "Constructor - Widget size:" << this->size();
}

/**
 * @brief Destroy the waterfallgraph::waterfallgraph object
 * 
 */
waterfallgraph::~waterfallgraph()
{
    // Note: graphicsView and graphicsScene are child widgets/scenes, so they will be automatically deleted by Qt's parent-child mechanism
}

/**
 * @brief Set the data for the graph.
 * 
 * @param xData 
 * @param yData 
 */
void waterfallgraph::setData(const std::vector<double>& xData, const std::vector<double>& yData)
{
    // Validate that both vectors have the same size
    if (xData.size() != yData.size()) {
        qDebug() << "Error: xData and yData must have the same size. xData size:" << xData.size() << "yData size:" << yData.size();
        return;
    }
    
    // Store the data
    this->xData = xData;
    this->yData = yData;
    
    qDebug() << "Data set successfully. Size:" << xData.size();
    // TODO: Redraw the graph with the new data
}

/**
 * @brief Handle mouse click events.
 * 
 * @param scenePos 
 */
void waterfallgraph::onMouseClick(const QPointF& scenePos)
{
    qDebug() << "Mouse clicked at scene position:" << scenePos;
    // This is a virtual function that can be overridden in derived classes
}

void waterfallgraph::onMouseDrag(const QPointF& scenePos)
{
    qDebug() << "Mouse dragged to scene position:" << scenePos;
    // This is a virtual function that can be overridden in derived classes
}

/**
 * @brief Draw the graph.
 * 
 */
void waterfallgraph::draw()
{
    if (!graphicsScene)
        return;

    // Clear existing items
    graphicsScene->clear();

    // Update the drawing area
    setupDrawingArea();

    // Draw grid if enabled
    if (gridEnabled) {
        drawGrid();
    }

    // Draw test geometry using drawutils
    DrawUtils::drawDefaultTestPattern(graphicsScene);
}

/**
 * @brief Update the graphics dimensions.
 * 
 */
void waterfallgraph::updateGraphicsDimensions()
{
    if (!graphicsView || !graphicsScene) return;
    
    // Get the current size of the widget
    QSize widgetSize = this->size();
    
    qDebug() << "updateGraphicsDimensions - Widget size:" << widgetSize;
    
    // Only update if we have valid dimensions
    if (widgetSize.width() > 0 && widgetSize.height() > 0) {
        // Set scene rect to match widget size exactly
        QRectF newSceneRect(0, 0, widgetSize.width(), widgetSize.height());
        graphicsScene->setSceneRect(newSceneRect);
        
        // Ensure the graphics view fits the scene exactly (no scrollbars)
        graphicsView->fitInView(newSceneRect, Qt::KeepAspectRatio);
        graphicsView->resetTransform(); // Reset any scaling
        graphicsView->setTransform(QTransform()); // Ensure 1:1 mapping
        
        // Update the drawing area
        setupDrawingArea();
        
        // Redraw the scene
        draw();
        
        qDebug() << "Graphics dimensions updated successfully to:" << widgetSize;
        qDebug() << "Scene rect is now:" << graphicsScene->sceneRect();
    } else {
        qDebug() << "Widget size is invalid, skipping update";
    }
}

/**
 * @brief Setup the drawing area.
 * 
 */
void waterfallgraph::setupDrawingArea()
{
    // Set up the drawing area to cover the entire scene
    drawingArea = graphicsScene->sceneRect();
    qDebug() << "Drawing area set to:" << drawingArea;
}


/**
 * @brief Draw the grid.
 * 
 */
void waterfallgraph::drawGrid()
{
    if (!graphicsScene || !gridEnabled || drawingArea.isEmpty() || gridDivisions <= 0) return;
    
    QPen gridPen(Qt::white, 1, Qt::DashLine); // White grid lines for black background
    
    // Draw vertical grid lines (for x-axis - variable)
    double stepX = drawingArea.width() / gridDivisions;
    for (int i = 0; i <= gridDivisions; ++i) {
        double x = drawingArea.left() + i * stepX;
        graphicsScene->addLine(x, drawingArea.top(), x, drawingArea.bottom(), gridPen);
    }
    
    // Draw horizontal grid lines (for y-axis - time)
    double stepY = drawingArea.height() / gridDivisions;
    for (int i = 0; i <= gridDivisions; ++i) {
        double y = drawingArea.top() + i * stepY;
        graphicsScene->addLine(drawingArea.left(), y, drawingArea.right(), y, gridPen);
    }
    
    // Draw border
    QPen borderPen(Qt::white, 2); // White border for black background
    graphicsScene->addRect(drawingArea, borderPen);
}

/**
 * @brief Handle mouse press events.
 * 
 * @param event 
 */
void waterfallgraph::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // Convert widget coordinates to scene coordinates
        QPointF scenePos = graphicsView->mapToScene(event->pos());
        
        // Check if the click is within the drawing area
        if (drawingArea.contains(scenePos)) {
            isDragging = true;
            lastMousePos = scenePos;
            onMouseClick(scenePos);
        }
    }
    
    // Call parent implementation
    QWidget::mousePressEvent(event);
}

/**
 * @brief Handle mouse move events.
 * 
 * @param event 
 */
void waterfallgraph::mouseMoveEvent(QMouseEvent *event)
{
    if (isDragging && (event->buttons() & Qt::LeftButton)) {
        // Convert widget coordinates to scene coordinates
        QPointF scenePos = graphicsView->mapToScene(event->pos());
        
        // Check if the move is within the drawing area
        if (drawingArea.contains(scenePos)) {
            onMouseDrag(scenePos);
            lastMousePos = scenePos;
        }
    }
    
    // Call parent implementation
    QWidget::mouseMoveEvent(event);
}

/**
 * @brief Handle mouse release events.
 * 
 * @param event 
 */
void waterfallgraph::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        isDragging = false;
    }
    
    // Call parent implementation
    QWidget::mouseReleaseEvent(event);
}

/**
 * @brief Handle resize events.
 * 
 * @param event 
 */
void waterfallgraph::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    
    // Ensure graphics view fits the widget exactly
    if (graphicsView) {
        graphicsView->resize(event->size());
    }
    
    // Update graphics dimensions when the widget is resized
    updateGraphicsDimensions();
    
    qDebug() << "Resize event - New size:" << size();
}

/**
 * @brief Handle show events.
 * 
 * @param event 
 */
void waterfallgraph::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    
    // This is called when the widget becomes visible
    qDebug() << "showEvent - Widget size:" << this->size();
    qDebug() << "showEvent - Graphics view size:" << graphicsView->size();
    
    // Ensure graphics view fits the widget exactly
    if (graphicsView) {
        graphicsView->resize(this->size());
    }
    
    // Update graphics dimensions now that we're visible
    updateGraphicsDimensions();
}
