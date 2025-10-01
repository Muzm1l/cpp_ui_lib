#include "zoompanel.h"
#include <QFontMetrics>

ZoomPanel::ZoomPanel(QWidget *parent)
    : QWidget(parent), m_graphicsView(nullptr), m_scene(nullptr), m_indicator(nullptr), m_leftText(nullptr), m_centerText(nullptr), m_rightText(nullptr), m_isDragging(false), m_isExtending(false), m_currentValue(1.0), m_extendMode(None), m_userModifiedBounds(false), m_actualLowerBound(0.0), m_actualUpperBound(1.0)
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
    m_scene->setSceneRect(0, 0, sceneWidth - 1, sceneHeight - 1);

    // Set black background for the graphics scene
    m_scene->setBackgroundBrush(QBrush(Qt::black));

    // Set the scene to the graphics view
    m_graphicsView->setScene(m_scene);

    // Ensure graphics view doesn't interfere with mouse events
    m_graphicsView->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    // Create the visual elements
    createIndicator();
    createTextItems();

    // Update indicator to current value to ensure proper initial state
    updateIndicator(m_currentValue);
}

void ZoomPanel::createIndicator()
{
    QRect drawArea = this->rect();
    int margin = qMax(2, drawArea.height() / 10); // Dynamic margin based on height
    int indicatorHeight = drawArea.height() - (2 * margin);
    int indicatorY = (drawArea.height() - indicatorHeight) / 2; // Center vertically

    // Create indicator that spans the full extent (full width minus margins)
    int indicatorWidth = drawArea.width() - (2 * margin);
    int initialX = margin; // Start at left edge

    m_indicator = new QGraphicsRectItem(initialX, indicatorY, indicatorWidth, indicatorHeight);

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

    // Calculate maximum width for each label (20% of total width)
    int maxLabelWidth = drawArea.width() * 0.20;

    // Calculate optimal font size to fit within maxLabelWidth
    int fontSize = calculateOptimalFontSize(maxLabelWidth);
    QFont textFont("Arial", fontSize);

    // Calculate vertical center position
    int textY = (drawArea.height() - fontSize) / 2; // Center vertically

    // Calculate horizontal positions with proper spacing
    int leftMargin = qMax(2, drawArea.width() / 20);
    int rightMargin = qMax(2, drawArea.width() / 20);

    // Create left text item - positioned at left margin, constrained to maxLabelWidth
    m_leftText = new QGraphicsTextItem(QString::number(leftLabelValue, 'f', 2));
    m_leftText->setFont(textFont);
    m_leftText->setDefaultTextColor(Qt::white);
    m_leftText->setPos(leftMargin, textY);
    m_leftText->setTextWidth(maxLabelWidth); // Constrain width to 20% of slider width
    m_scene->addItem(m_leftText);

    // Create center text item - centered, constrained to maxLabelWidth
    m_centerText = new QGraphicsTextItem(QString::number(centerLabelValue, 'f', 2));
    m_centerText->setFont(textFont);
    m_centerText->setDefaultTextColor(Qt::white);
    m_centerText->setPos(drawArea.width() / 2 - maxLabelWidth / 2, textY); // Center horizontally with width constraint
    m_centerText->setTextWidth(maxLabelWidth);                             // Constrain width to 20% of slider width
    m_scene->addItem(m_centerText);

    // Create right text item - positioned at right margin, constrained to maxLabelWidth
    m_rightText = new QGraphicsTextItem(QString::number(rightLabelValue, 'f', 2));
    m_rightText->setFont(textFont);
    m_rightText->setDefaultTextColor(Qt::white);
    m_rightText->setPos(drawArea.width() - rightMargin - maxLabelWidth, textY); // Right aligned with width constraint
    m_rightText->setTextWidth(maxLabelWidth);                                   // Constrain width to 20% of slider width
    m_scene->addItem(m_rightText);
}

/**
 * @brief Calculate the optimal font size to fit text within the given width constraint
 *
 * @param maxWidth Maximum width constraint in pixels
 * @return int Optimal font size
 */
int ZoomPanel::calculateOptimalFontSize(int maxWidth)
{
    // Sample text to measure (using the longest expected text format)
    QString sampleText = "999.99"; // Worst case scenario for numeric labels

    // Start with a reasonable maximum font size
    int maxFontSize = 12;
    int minFontSize = 4;

    // Binary search for the optimal font size
    int fontSize = maxFontSize;

    while (maxFontSize > minFontSize)
    {
        fontSize = (maxFontSize + minFontSize) / 2;

        QFont testFont("Arial", fontSize);
        QFontMetrics fontMetrics(testFont);

        int textWidth = fontMetrics.horizontalAdvance(sampleText);

        if (textWidth <= maxWidth)
        {
            // Font size fits, try larger
            minFontSize = fontSize + 1;
        }
        else
        {
            // Font size too large, try smaller
            maxFontSize = fontSize - 1;
        }
    }

    // Ensure we don't go below minimum
    fontSize = qMax(4, qMin(12, fontSize));

    qDebug() << "ZoomPanel: Calculated optimal font size:" << fontSize << "for max width:" << maxWidth;

    return fontSize;
}

void ZoomPanel::updateIndicator(double value)
{
    if (!m_indicator)
        return;

    // Clamp value between 0.0 and 1.0
    value = qBound(0.0, value, 1.0);

    QRect drawArea = this->rect();
    int margin = qMax(2, drawArea.height() / 10); // Dynamic margin based on height
    int indicatorHeight = drawArea.height() - (2 * margin);
    int indicatorY = (drawArea.height() - indicatorHeight) / 2; // Center vertically

    // Calculate available horizontal space
    int availableWidth = drawArea.width() - (2 * margin);

    // Calculate indicator width based on value (0.0 = minimum width, 1.0 = full width)
    int minWidth = 20; // Minimum visible width
    int indicatorWidth = minWidth + static_cast<int>((availableWidth - minWidth) * value);

    // Center the indicator horizontally
    int indicatorX = margin + (availableWidth - indicatorWidth) / 2;

    // Update indicator rectangle
    QRectF rect(indicatorX, indicatorY, indicatorWidth, indicatorHeight);
    m_indicator->setRect(rect);
}

void ZoomPanel::setLeftLabelValue(const qreal value)
{
    leftLabelValue = value;
    if (m_leftText)
    {
        m_leftText->setPlainText(QString::number(value, 'f', 2));
    }
}

void ZoomPanel::setCenterLabelValue(const qreal value)
{
    centerLabelValue = value;
    if (m_centerText)
    {
        m_centerText->setPlainText(QString::number(value, 'f', 2));
    }
}

void ZoomPanel::setRightLabelValue(const qreal value)
{
    rightLabelValue = value;
    if (m_rightText)
    {
        m_rightText->setPlainText(QString::number(value, 'f', 2));
    }
}

const qreal ZoomPanel::getLeftLabelValue() const
{
    return leftLabelValue;
}

const qreal ZoomPanel::getCenterLabelValue() const
{
    return centerLabelValue;
}

const qreal ZoomPanel::getRightLabelValue() const
{
    return rightLabelValue;
}

bool ZoomPanel::hasUserModifiedBounds() const
{
    return m_userModifiedBounds;
}

void ZoomPanel::resetUserModifiedFlag()
{
    m_userModifiedBounds = false;
}

void ZoomPanel::mousePressEvent(QMouseEvent *event)
{
    qDebug() << "Mouse press event received at:" << event->pos();

    if (event->button() == Qt::LeftButton)
    {
        m_initialMousePos = event->pos();

        // First check if we clicked on the indicator
        if (m_indicator && m_indicator->rect().contains(event->pos()))
        {
            // Detect if we're in extend mode or drag mode
            m_extendMode = detectExtendMode(event->pos());

            if (m_extendMode != None)
            {
                // Enter extend mode
                m_isExtending = true;
                m_isDragging = false;

                qDebug() << "Entered extend mode:" << (m_extendMode == ExtendLeft ? "Left" : "Right");
                setCursor(Qt::SizeHorCursor);
            }
            else
            {
                // Enter drag mode (clicked on indicator but not on edges)
                m_isDragging = true;
                m_isExtending = false;
                m_userModifiedBounds = true; // Mark as user modified when dragging

                // Store the initial indicator position for reference
                QRectF indicatorRect = m_indicator->rect();
                m_initialIndicatorPos = QPoint(static_cast<int>(indicatorRect.x()), static_cast<int>(indicatorRect.y()));

                qDebug() << "Started dragging from position:" << m_initialMousePos;
                setCursor(Qt::ClosedHandCursor);
            }
        }
        else
        {
            // Clicked outside indicator - no action
            qDebug() << "Clicked outside indicator - no action";
        }
    }
}

void ZoomPanel::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isDragging)
    {
        qDebug() << "Mouse move while dragging at:" << event->pos();
        updateValueFromMousePosition(event->pos());
    }
    else if (m_isExtending)
    {
        qDebug() << "Mouse move while extending at:" << event->pos();
        updateExtentFromMousePosition(event->pos());
    }
    else
    {
        // Update cursor based on hover position over indicator
        if (m_indicator && m_indicator->rect().contains(event->pos()))
        {
            ExtendMode hoverMode = detectExtendMode(event->pos());
            if (hoverMode != None)
            {
                setCursor(Qt::SizeHorCursor);
            }
            else
            {
                setCursor(Qt::OpenHandCursor);
            }
        }
        else
        {
            setCursor(Qt::ArrowCursor);
        }
    }
}

void ZoomPanel::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (m_isDragging)
        {
            m_isDragging = false;
            setCursor(Qt::ArrowCursor);
            qDebug() << "Drag mode ended";
        }

        if (m_isExtending)
        {
            m_isExtending = false;
            m_extendMode = None;
            setCursor(Qt::ArrowCursor);
            qDebug() << "Extend mode ended";
        }
    }
}

void ZoomPanel::updateValueFromMousePosition(const QPoint &currentPos)
{
    if (!m_indicator)
        return;

    QRect drawArea = this->rect();
    int margin = qMax(2, drawArea.height() / 10);
    int indicatorWidth = 20;

    // Calculate available horizontal space for movement
    int availableWidth = drawArea.width() - (2 * margin) - indicatorWidth;

    // Calculate the new X position based on mouse movement
    int deltaX = currentPos.x() - m_initialMousePos.x();
    int newIndicatorX = m_initialIndicatorPos.x() + deltaX;

    // Apply edge collision detection
    int minX = margin;
    int maxX = margin + availableWidth;

    // Clamp the indicator position to stay within bounds
    newIndicatorX = qBound(minX, newIndicatorX, maxX);

    qDebug() << "Mouse delta:" << deltaX << "New indicator X:" << newIndicatorX << "Bounds: [" << minX << ", " << maxX << "]";

    // Convert position back to value (0.0 to 1.0)
    qreal newValue = static_cast<qreal>(newIndicatorX - minX) / static_cast<qreal>(availableWidth);
    newValue = qBound(0.0, newValue, 1.0);

    qDebug() << "Calculated new value:" << newValue;

    // Update if value changed significantly
    if (qAbs(newValue - m_currentValue) > 0.001)
    {
        qDebug() << "Updating indicator value to:" << newValue;

        // Calculate the current range (difference between upper and lower bounds)
        qreal currentRange = m_actualUpperBound - m_actualLowerBound;

        // Calculate new bounds based on the drag position while maintaining the same range
        qreal newLowerBound = newValue - (currentRange / 2.0);
        qreal newUpperBound = newValue + (currentRange / 2.0);

        // Clamp bounds to valid range (0.0 to 1.0)
        if (newLowerBound < 0.0)
        {
            newLowerBound = 0.0;
            newUpperBound = currentRange;
        }
        if (newUpperBound > 1.0)
        {
            newUpperBound = 1.0;
            newLowerBound = 1.0 - currentRange;
        }

        // Update actual bounds
        m_actualLowerBound = newLowerBound;
        m_actualUpperBound = newUpperBound;
        m_currentValue = newValue;

        // Update indicator position to reflect new bounds
        updateIndicatorToBounds();

        // Emit current actual bounds
        ZoomBounds bounds;
        bounds.upperbound = m_actualUpperBound;
        bounds.lowerbound = m_actualLowerBound;
        qDebug() << "ZoomPanel: Emitting valueChanged signal (drag) - Lower:" << bounds.lowerbound << "Upper:" << bounds.upperbound;
        emit valueChanged(bounds);
    }
}

void ZoomPanel::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    // Update graphics scene size
    if (m_scene)
    {
        QRect drawArea = this->rect();
        m_scene->setSceneRect(0, 0, drawArea.width() - 1, drawArea.height() - 1);
    }

    // Update all elements to new size
    updateAllElements();
}

/**
 * @brief Handle show events to ensure proper initialization when becoming visible
 *
 * @param event
 */
void ZoomPanel::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    // Ensure the zoom panel is properly initialized when becoming visible
    if (m_scene && m_indicator)
    {
        // Update indicator to current value to ensure proper visual state
        updateIndicator(m_currentValue);
        qDebug() << "ZoomPanel: Show event - updated indicator to value:" << m_currentValue;
    }
}

void ZoomPanel::updateAllElements()
{
    if (!m_scene)
        return;

    // Remove existing items
    if (m_indicator)
    {
        m_scene->removeItem(m_indicator);
        delete m_indicator;
        m_indicator = nullptr;
    }
    if (m_leftText)
    {
        m_scene->removeItem(m_leftText);
        delete m_leftText;
        m_leftText = nullptr;
    }
    if (m_centerText)
    {
        m_scene->removeItem(m_centerText);
        delete m_centerText;
        m_centerText = nullptr;
    }
    if (m_rightText)
    {
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

ZoomPanel::ExtendMode ZoomPanel::detectExtendMode(const QPoint &mousePos)
{
    if (!m_indicator)
        return None;

    QRectF indicatorRect = m_indicator->rect();
    qreal edgeZone = indicatorRect.width() * 0.05; // 5% of indicator width

    // Check if mouse is within the indicator rectangle
    if (!indicatorRect.contains(mousePos))
        return None;

    // Check if mouse is in left edge zone of indicator (0% to 5%)
    if (mousePos.x() <= indicatorRect.left() + edgeZone)
    {
        return ExtendLeft;
    }

    // Check if mouse is in right edge zone of indicator (95% to 100%)
    if (mousePos.x() >= indicatorRect.right() - edgeZone)
    {
        return ExtendRight;
    }

    return None;
}

void ZoomPanel::updateExtentFromMousePosition(const QPoint &currentPos)
{
    if (m_extendMode == None || !m_indicator)
        return;

    QRect drawArea = this->rect();
    int margin = qMax(2, drawArea.height() / 10);

    // Calculate available horizontal space for the indicator
    int availableWidth = drawArea.width() - (2 * margin);

    // Convert mouse X position to a value between 0.0 and 1.0
    qreal mouseValue = static_cast<qreal>(currentPos.x() - margin) / static_cast<qreal>(availableWidth);
    mouseValue = qBound(0.0, mouseValue, 1.0);

    if (m_extendMode == ExtendLeft)
    {
        // Extend left edge - modify the actual lower bound
        qreal newLowerBound = mouseValue;
        newLowerBound = qBound(0.0, newLowerBound, m_actualUpperBound - 0.01); // Don't exceed upper bound
        m_actualLowerBound = newLowerBound;
        m_userModifiedBounds = true; // Mark as user modified
    }
    else if (m_extendMode == ExtendRight)
    {
        // Extend right edge - modify the actual upper bound
        qreal newUpperBound = mouseValue;
        newUpperBound = qBound(m_actualLowerBound + 0.01, newUpperBound, 1.0); // Don't go below current lower bound
        m_actualUpperBound = newUpperBound;
        m_userModifiedBounds = true; // Mark as user modified
    }

    // Update the indicator to span from leftLabelValue to rightLabelValue
    updateIndicatorToBounds();

    // Update bounds calculation and emit signal
    ZoomBounds bounds;
    bounds.upperbound = m_actualUpperBound;
    bounds.lowerbound = m_actualLowerBound;
    qDebug() << "ZoomPanel: Emitting valueChanged signal (extend) - Lower:" << bounds.lowerbound << "Upper:" << bounds.upperbound;
    emit valueChanged(bounds);
}

void ZoomPanel::updateIndicatorToBounds()
{
    if (!m_indicator)
        return;

    QRect drawArea = this->rect();
    int margin = qMax(2, drawArea.height() / 10);
    int indicatorHeight = drawArea.height() - (2 * margin);
    int indicatorY = (drawArea.height() - indicatorHeight) / 2;

    // Calculate available horizontal space
    int availableWidth = drawArea.width() - (2 * margin);

    // Calculate indicator position and width based on bounds
    int leftX = margin + static_cast<int>(m_actualLowerBound * availableWidth);
    int rightX = margin + static_cast<int>(m_actualUpperBound * availableWidth);

    // Ensure minimum width
    int indicatorWidth = qMax(1, rightX - leftX);
    int indicatorX = leftX;

    // Update indicator rectangle to span from leftLabelValue to rightLabelValue
    QRectF rect(indicatorX, indicatorY, indicatorWidth, indicatorHeight);
    m_indicator->setRect(rect);

    qDebug() << "Updated indicator bounds - Left:" << m_actualLowerBound << "Right:" << m_actualUpperBound
             << "Width:" << indicatorWidth << "X:" << indicatorX;
}
