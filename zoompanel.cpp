#include "zoompanel.h"

ZoomPanel::ZoomPanel(QWidget *parent)
    : QWidget(parent)
    , m_graphicsView(nullptr)
    , m_scene(nullptr)
    , m_indicator(nullptr)
    , m_leftText(nullptr)
    , m_centerText(nullptr)
    , m_rightText(nullptr)
    , m_isDragging(false)
    , m_currentValue(0.5)
    , m_startedFromRightHalf(false)
{
    // Set black background
    this->setStyleSheet("background-color: black;");
    
    // Enable mouse tracking for drag operations
    this->setMouseTracking(true);
    
    // Create graphics view programmatically
    m_graphicsView = new QGraphicsView(this);
    m_graphicsView->setFrameShape(QFrame::NoFrame);
    m_graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    // Remove all margins and padding for snug fit
    setContentsMargins(0, 0, 0, 0);
    
    setupGraphicsView();
    
    // Set up layout to contain the graphics view
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_graphicsView);
    setLayout(layout);
}

ZoomPanel::~ZoomPanel()
{
    // No UI to delete anymore
}

void ZoomPanel::setupGraphicsView()
{
    QRect drawArea = this->rect();

    int sceneWidth = drawArea.width();
    int sceneHeight = drawArea.height();

    // Create graphics scene
    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(0, 0, sceneWidth-1, sceneHeight-1);
    
    // Set black background for the graphics scene
    m_scene->setBackgroundBrush(QBrush(Qt::black));
    
    // Set the scene to the graphics view
    m_graphicsView->setScene(m_scene);
    
    // Ensure graphics view doesn't interfere with mouse events
    m_graphicsView->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    
    // Create the visual elements
    createIndicator();
    createTextItems();
}

void ZoomPanel::createIndicator()
{
    QRect drawArea = this->rect();
    int margin = qMax(2, drawArea.height() / 10); // Dynamic margin based on height
    int indicatorHeight = drawArea.height() - (2 * margin);
    int indicatorY = (drawArea.height() - indicatorHeight) / 2; // Center vertically
    int centerX = drawArea.width() / 2; // Center horizontally
    
    // Create rectangular indicator - starts at 10 pixels thin (value 0) centered
    m_indicator = new QGraphicsRectItem(centerX - 5, indicatorY, 10, indicatorHeight);
    
    QPen indicatorPen(QColor(50, 50, 50)); // Dark grey
    indicatorPen.setWidth(1);
    m_indicator->setPen(indicatorPen);
    
    QBrush indicatorBrush(QColor(100, 100, 100)); // Medium grey
    m_indicator->setBrush(indicatorBrush);
    
    m_scene->addItem(m_indicator);
}

void ZoomPanel::createTextItems()
{
    QRect drawArea = this->rect();
    
    // Create font for text with dynamic size
    int fontSize = qMax(6, qMin(12, drawArea.height() / 4)); // Scale font with height
    QFont textFont("Arial", fontSize);
    
    // Calculate vertical center position
    int textY = (drawArea.height() - fontSize) / 2; // Center vertically
    
    // Calculate horizontal positions dynamically
    int leftMargin = qMax(2, drawArea.width() / 20);
    int rightMargin = qMax(2, drawArea.width() / 20);
    
    // Create left text item
    m_leftText = new QGraphicsTextItem("0.00");
    m_leftText->setFont(textFont);
    m_leftText->setDefaultTextColor(Qt::white);
    m_leftText->setPos(leftMargin, textY);
    m_scene->addItem(m_leftText);
    
    // Create center text item
    m_centerText = new QGraphicsTextItem("0.50");
    m_centerText->setFont(textFont);
    m_centerText->setDefaultTextColor(Qt::white);
    m_centerText->setPos(drawArea.width()/2 - 15, textY); // Center horizontally
    m_scene->addItem(m_centerText);
    
    // Create right text item
    m_rightText = new QGraphicsTextItem("1.00");
    m_rightText->setFont(textFont);
    m_rightText->setDefaultTextColor(Qt::white);
    m_rightText->setPos(drawArea.width() - rightMargin - 30, textY); // Right aligned
    m_scene->addItem(m_rightText);
}

void ZoomPanel::setIndicatorValue(double value)
{
    m_currentValue = value;
    updateIndicator(value);
    
    // Calculate bounds: upperbound = center + value*|right-center|, lowerbound = center - value*|center-left|
    ZoomBounds bounds;
    bounds.upperbound = centerLabelValue + value * qAbs(rightLabelValue - centerLabelValue);
    bounds.lowerbound = centerLabelValue - value * qAbs(centerLabelValue - leftLabelValue);
    
    emit valueChanged(bounds);
}

void ZoomPanel::updateIndicator(double value)
{
    if (!m_indicator) return;
    
    // Clamp value between 0.0 and 1.0
    value = qBound(0.0, value, 1.0);
    
    QRect drawArea = this->rect();
    int margin = qMax(2, drawArea.height() / 10); // Dynamic margin based on height
    int availableWidth = drawArea.width() - (2 * margin);
    int indicatorHeight = drawArea.height() - (2 * margin);
    int indicatorY = (drawArea.height() - indicatorHeight) / 2; // Center vertically
    int centerX = drawArea.width() / 2; // Center horizontally
    
    // Calculate width: minimum 10 pixels, maximum available width
    double minWidth = 10.0;
    double maxWidth = static_cast<double>(availableWidth);
    double width = minWidth + (maxWidth - minWidth) * value;
    
    // Calculate X position to center the indicator
    double indicatorX = centerX - (width / 2);
    
    // Update indicator rectangle with centered position
    QRectF rect(indicatorX, indicatorY, width, indicatorHeight);
    m_indicator->setRect(rect);
}

void ZoomPanel::setLeftLabelValue(qreal value)
{
    leftLabelValue = value;
    if (m_leftText) {
        m_leftText->setPlainText(QString::number(value, 'f', 2));
    }
}

void ZoomPanel::setCenterLabelValue(qreal value)
{
    centerLabelValue = value;
    if (m_centerText) {
        m_centerText->setPlainText(QString::number(value, 'f', 2));
    }
}

void ZoomPanel::setRightLabelValue(qreal value)
{
    rightLabelValue = value;
    if (m_rightText) {
        m_rightText->setPlainText(QString::number(value, 'f', 2));
    }
}

void ZoomPanel::mousePressEvent(QMouseEvent *event)
{
    qDebug() << "Mouse press event received at:" << event->pos();
    
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_initialMousePos = event->pos();
        
        // Determine if started from right half
        QRect drawArea = this->rect();
        int centerX = drawArea.width() / 2;
        m_startedFromRightHalf = (event->pos().x() > centerX);
        
        qDebug() << "Started dragging from" << (m_startedFromRightHalf ? "right" : "left") << "half";
        
        // Set cursor to indicate dragging
        setCursor(Qt::ClosedHandCursor);
    }
    
    QWidget::mousePressEvent(event);
}

void ZoomPanel::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isDragging) {
        qDebug() << "Mouse move while dragging at:" << event->pos();
        updateValueFromMousePosition(event->pos());
    }
    
    QWidget::mouseMoveEvent(event);
}

void ZoomPanel::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
        setCursor(Qt::ArrowCursor);
    }
    
    QWidget::mouseReleaseEvent(event);
}

void ZoomPanel::updateValueFromMousePosition(const QPoint &currentPos)
{
    QRect drawArea = this->rect();
    int centerX = drawArea.width() / 2;
    
    // Calculate horizontal movement
    int deltaX = currentPos.x() - m_initialMousePos.x();
    
    qDebug() << "DeltaX:" << deltaX << "Current value:" << m_currentValue;
    
    // Determine value change based on starting position and movement
    qreal valueChange = 0.0;
    
    if (m_startedFromRightHalf) {
        // Started from right half
        if (deltaX > 0) {
            // Moving right - increase value
            valueChange = static_cast<qreal>(deltaX) / static_cast<qreal>(drawArea.width()) * 0.2; // Reduced scale factor
        } else {
            // Moving left - decrease value
            valueChange = static_cast<qreal>(deltaX) / static_cast<qreal>(drawArea.width()) * 0.2; // Reduced scale factor
        }
    } else {
        // Started from left half - reverse the logic
        if (deltaX > 0) {
            // Moving right - decrease value (opposite behavior)
            valueChange = -static_cast<qreal>(deltaX) / static_cast<qreal>(drawArea.width()) * 0.2;
        } else {
            // Moving left - increase value (opposite behavior)
            valueChange = -static_cast<qreal>(deltaX) / static_cast<qreal>(drawArea.width()) * 0.2;
        }
    }
    
    // Calculate new value
    qreal newValue = m_currentValue + valueChange;
    
    // Clamp value between 0.0 and 1.0
    newValue = qBound(0.0, newValue, 1.0);
    
    qDebug() << "Value change:" << valueChange << "New value:" << newValue;
    
    // Update if value changed
    if (qAbs(newValue - m_currentValue) > 0.001) { // Small threshold to avoid constant updates
        qDebug() << "Updating indicator value to:" << newValue;
        setIndicatorValue(newValue);
    }
}

void ZoomPanel::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    
    // Update graphics scene size
    if (m_scene) {
        QRect drawArea = this->rect();
        m_scene->setSceneRect(0, 0, drawArea.width()-1, drawArea.height()-1);
    }
    
    // Update all elements to new size
    updateAllElements();
}

void ZoomPanel::updateAllElements()
{
    if (!m_scene) return;
    
    // Remove existing items
    if (m_indicator) {
        m_scene->removeItem(m_indicator);
        delete m_indicator;
        m_indicator = nullptr;
    }
    if (m_leftText) {
        m_scene->removeItem(m_leftText);
        delete m_leftText;
        m_leftText = nullptr;
    }
    if (m_centerText) {
        m_scene->removeItem(m_centerText);
        delete m_centerText;
        m_centerText = nullptr;
    }
    if (m_rightText) {
        m_scene->removeItem(m_rightText);
        delete m_rightText;
        m_rightText = nullptr;
    }
    
    // Recreate elements with new size
    createIndicator();
    createTextItems();
    
    // Update indicator to current value
    updateIndicator(m_currentValue);
}
