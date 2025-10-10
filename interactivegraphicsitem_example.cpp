#include "interactivegraphicsitem_example.h"
#include <QDebug>
#include <QPainterPath>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QConicalGradient>

InteractiveGraphicsItemExample::InteractiveGraphicsItemExample(QWidget *parent)
    : QMainWindow(parent)
    , m_centralWidget(nullptr)
    , m_mainLayout(nullptr)
    , m_controlLayout(nullptr)
    , m_graphicsView(nullptr)
    , m_scene(nullptr)
    , m_rectangleItem(nullptr)
    , m_circleItem(nullptr)
    , m_triangleItem(nullptr)
{
    setWindowTitle("Interactive Graphics Item Example");
    setMinimumSize(800, 600);
    
    setupUI();
    setupGraphicsScene();
    createExampleItems();
    updateStatusLabel();
}

InteractiveGraphicsItemExample::~InteractiveGraphicsItemExample()
{
    // Qt's parent-child system handles cleanup
}

void InteractiveGraphicsItemExample::setupUI()
{
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    
    m_mainLayout = new QHBoxLayout(m_centralWidget);
    
    // Graphics view
    m_graphicsView = new QGraphicsView();
    m_graphicsView->setMinimumSize(500, 400);
    m_graphicsView->setRenderHint(QPainter::Antialiasing);
    m_graphicsView->setDragMode(QGraphicsView::RubberBandDrag);
    
    // Control panel
    m_controlLayout = new QVBoxLayout();
    
    // Region visibility group
    m_regionGroup = new QGroupBox("Region Visibility");
    QVBoxLayout *regionLayout = new QVBoxLayout(m_regionGroup);
    
    m_showDragRegionCheck = new QCheckBox("Show Drag Region");
    m_showDragRegionCheck->setChecked(true);
    connect(m_showDragRegionCheck, &QCheckBox::toggled, this, &InteractiveGraphicsItemExample::onDragRegionToggled);
    
    m_showRotateRegionCheck = new QCheckBox("Show Rotate Region");
    m_showRotateRegionCheck->setChecked(true);
    connect(m_showRotateRegionCheck, &QCheckBox::toggled, this, &InteractiveGraphicsItemExample::onRotateRegionToggled);
    
    m_showCustomRegionsCheck = new QCheckBox("Show Custom Regions");
    m_showCustomRegionsCheck->setChecked(true);
    connect(m_showCustomRegionsCheck, &QCheckBox::toggled, this, &InteractiveGraphicsItemExample::onCustomRegionsToggled);
    
    regionLayout->addWidget(m_showDragRegionCheck);
    regionLayout->addWidget(m_showRotateRegionCheck);
    regionLayout->addWidget(m_showCustomRegionsCheck);
    
    // Interaction group
    m_interactionGroup = new QGroupBox("Interaction Settings");
    QVBoxLayout *interactionLayout = new QVBoxLayout(m_interactionGroup);
    
    m_dragEnabledCheck = new QCheckBox("Drag Enabled");
    m_dragEnabledCheck->setChecked(true);
    connect(m_dragEnabledCheck, &QCheckBox::toggled, this, &InteractiveGraphicsItemExample::onDragEnabledToggled);
    
    m_rotateEnabledCheck = new QCheckBox("Rotate Enabled");
    m_rotateEnabledCheck->setChecked(true);
    connect(m_rotateEnabledCheck, &QCheckBox::toggled, this, &InteractiveGraphicsItemExample::onRotateEnabledToggled);
    
    QHBoxLayout *rotationStepLayout = new QHBoxLayout();
    rotationStepLayout->addWidget(new QLabel("Rotation Step:"));
    m_rotationStepSpin = new QSpinBox();
    m_rotationStepSpin->setRange(1, 90);
    m_rotationStepSpin->setValue(15);
    m_rotationStepSpin->setSuffix("°");
    connect(m_rotationStepSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &InteractiveGraphicsItemExample::onRotationStepChanged);
    rotationStepLayout->addWidget(m_rotationStepSpin);
    
    interactionLayout->addWidget(m_dragEnabledCheck);
    interactionLayout->addWidget(m_rotateEnabledCheck);
    interactionLayout->addLayout(rotationStepLayout);
    
    // Custom region group
    m_customRegionGroup = new QGroupBox("Custom Regions");
    QVBoxLayout *customLayout = new QVBoxLayout(m_customRegionGroup);
    
    m_addCustomRegionButton = new QPushButton("Add Custom Region");
    connect(m_addCustomRegionButton, &QPushButton::clicked, this, &InteractiveGraphicsItemExample::onAddCustomRegion);
    
    m_clearCustomRegionsButton = new QPushButton("Clear Custom Regions");
    connect(m_clearCustomRegionsButton, &QPushButton::clicked, this, &InteractiveGraphicsItemExample::onClearCustomRegions);
    
    customLayout->addWidget(m_addCustomRegionButton);
    customLayout->addWidget(m_clearCustomRegionsButton);
    
    // Reset button
    m_resetButton = new QPushButton("Reset Items");
    connect(m_resetButton, &QPushButton::clicked, this, &InteractiveGraphicsItemExample::onResetItem);
    
    // Status label
    m_statusLabel = new QLabel("Ready");
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setMaximumHeight(100);
    
    // Add widgets to control layout
    m_controlLayout->addWidget(m_regionGroup);
    m_controlLayout->addWidget(m_interactionGroup);
    m_controlLayout->addWidget(m_customRegionGroup);
    m_controlLayout->addWidget(m_resetButton);
    m_controlLayout->addWidget(m_statusLabel);
    m_controlLayout->addStretch();
    
    // Add to main layout
    m_mainLayout->addWidget(m_graphicsView, 2);
    m_mainLayout->addLayout(m_controlLayout, 1);
}

void InteractiveGraphicsItemExample::setupGraphicsScene()
{
    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(-200, -200, 400, 400);
    m_graphicsView->setScene(m_scene);
}

void InteractiveGraphicsItemExample::createExampleItems()
{
    // Rectangle item
    m_rectangleItem = new InteractiveGraphicsItem();
    m_rectangleItem->setGeometry(QRectF(-50, -50, 100, 80));
    m_rectangleItem->setPos(-100, -100);
    m_rectangleItem->setCustomDrawFunction(drawRectangle);
    m_rectangleItem->setDragRegion(QRectF(-50, -50, 100, 80));
    m_rectangleItem->setRotateRegion(QRectF(30, 30, 20, 20));
    
    connect(m_rectangleItem, &InteractiveGraphicsItem::itemDragged, this, &InteractiveGraphicsItemExample::onItemDragged);
    connect(m_rectangleItem, &InteractiveGraphicsItem::itemRotated, this, &InteractiveGraphicsItemExample::onItemRotated);
    connect(m_rectangleItem, &InteractiveGraphicsItem::regionClicked, this, &InteractiveGraphicsItemExample::onRegionClicked);
    
    m_scene->addItem(m_rectangleItem);
    
    // Circle item
    m_circleItem = new InteractiveGraphicsItem();
    m_circleItem->setGeometry(QRectF(-40, -40, 80, 80));
    m_circleItem->setPos(100, -100);
    m_circleItem->setCustomDrawFunction(drawCircle);
    m_circleItem->setDragRegion(QRectF(-40, -40, 80, 80));
    m_circleItem->setRotateRegion(QRectF(20, 20, 20, 20));
    
    connect(m_circleItem, &InteractiveGraphicsItem::itemDragged, this, &InteractiveGraphicsItemExample::onItemDragged);
    connect(m_circleItem, &InteractiveGraphicsItem::itemRotated, this, &InteractiveGraphicsItemExample::onItemRotated);
    connect(m_circleItem, &InteractiveGraphicsItem::regionClicked, this, &InteractiveGraphicsItemExample::onRegionClicked);
    
    m_scene->addItem(m_circleItem);
    
    // Triangle item
    m_triangleItem = new InteractiveGraphicsItem();
    m_triangleItem->setGeometry(QRectF(-40, -40, 80, 80));
    m_triangleItem->setPos(0, 100);
    m_triangleItem->setCustomDrawFunction(drawTriangle);
    m_triangleItem->setDragRegion(QRectF(-40, -40, 80, 80));
    m_triangleItem->setRotateRegion(QRectF(20, 20, 20, 20));
    
    connect(m_triangleItem, &InteractiveGraphicsItem::itemDragged, this, &InteractiveGraphicsItemExample::onItemDragged);
    connect(m_triangleItem, &InteractiveGraphicsItem::itemRotated, this, &InteractiveGraphicsItemExample::onItemRotated);
    connect(m_triangleItem, &InteractiveGraphicsItem::regionClicked, this, &InteractiveGraphicsItemExample::onRegionClicked);
    
    m_scene->addItem(m_triangleItem);
}

void InteractiveGraphicsItemExample::onDragRegionToggled(bool enabled)
{
    if (m_rectangleItem) m_rectangleItem->setShowDragRegion(enabled);
    if (m_circleItem) m_circleItem->setShowDragRegion(enabled);
    if (m_triangleItem) m_triangleItem->setShowDragRegion(enabled);
}

void InteractiveGraphicsItemExample::onRotateRegionToggled(bool enabled)
{
    if (m_rectangleItem) m_rectangleItem->setShowRotateRegion(enabled);
    if (m_circleItem) m_circleItem->setShowRotateRegion(enabled);
    if (m_triangleItem) m_triangleItem->setShowRotateRegion(enabled);
}

void InteractiveGraphicsItemExample::onCustomRegionsToggled(bool enabled)
{
    if (m_rectangleItem) m_rectangleItem->setShowCustomRegions(enabled);
    if (m_circleItem) m_circleItem->setShowCustomRegions(enabled);
    if (m_triangleItem) m_triangleItem->setShowCustomRegions(enabled);
}

void InteractiveGraphicsItemExample::onDragEnabledToggled(bool enabled)
{
    if (m_rectangleItem) m_rectangleItem->setDragEnabled(enabled);
    if (m_circleItem) m_circleItem->setDragEnabled(enabled);
    if (m_triangleItem) m_triangleItem->setDragEnabled(enabled);
}

void InteractiveGraphicsItemExample::onRotateEnabledToggled(bool enabled)
{
    if (m_rectangleItem) m_rectangleItem->setRotateEnabled(enabled);
    if (m_circleItem) m_circleItem->setRotateEnabled(enabled);
    if (m_triangleItem) m_triangleItem->setRotateEnabled(enabled);
}

void InteractiveGraphicsItemExample::onRotationStepChanged(int value)
{
    if (m_rectangleItem) m_rectangleItem->setRotationStep(value);
    if (m_circleItem) m_circleItem->setRotationStep(value);
    if (m_triangleItem) m_triangleItem->setRotationStep(value);
}

void InteractiveGraphicsItemExample::onItemDragged(const QPointF &delta)
{
    updateStatusLabel();
    qDebug() << "Item dragged by:" << delta;
}

void InteractiveGraphicsItemExample::onItemRotated(qreal angle)
{
    updateStatusLabel();
    qDebug() << "Item rotated by:" << angle << "degrees";
}

void InteractiveGraphicsItemExample::onRegionClicked(InteractiveGraphicsItem::InteractionRegion region, const QPointF &position)
{
    QString regionName;
    switch (region) {
        case InteractiveGraphicsItem::DragRegion:
            regionName = "Drag Region";
            break;
        case InteractiveGraphicsItem::RotateRegion:
            regionName = "Rotate Region";
            break;
        case InteractiveGraphicsItem::CustomRegion:
            regionName = "Custom Region";
            break;
        default:
            regionName = "Unknown Region";
            break;
    }
    
    m_statusLabel->setText(QString("Clicked %1 at (%2, %3)")
                          .arg(regionName)
                          .arg(position.x(), 0, 'f', 1)
                          .arg(position.y(), 0, 'f', 1));
    
    qDebug() << "Region clicked:" << regionName << "at" << position;
}

void InteractiveGraphicsItemExample::onAddCustomRegion()
{
    // Add a custom region to the rectangle item
    if (m_rectangleItem) {
        QRectF customRegion(-30, -30, 20, 20);
        m_rectangleItem->addCustomRegion(customRegion, InteractiveGraphicsItem::CustomRegion);
        
        // Change the drawing function to show a star
        m_rectangleItem->setCustomDrawFunction(drawStar);
    }
}

void InteractiveGraphicsItemExample::onClearCustomRegions()
{
    if (m_rectangleItem) {
        m_rectangleItem->clearCustomRegions();
        m_rectangleItem->setCustomDrawFunction(drawRectangle);
    }
    if (m_circleItem) {
        m_circleItem->clearCustomRegions();
    }
    if (m_triangleItem) {
        m_triangleItem->clearCustomRegions();
    }
}

void InteractiveGraphicsItemExample::onResetItem()
{
    // Reset all items to their original positions and rotations
    if (m_rectangleItem) {
        m_rectangleItem->setPos(-100, -100);
        m_rectangleItem->setRotation(0);
    }
    if (m_circleItem) {
        m_circleItem->setPos(100, -100);
        m_circleItem->setRotation(0);
    }
    if (m_triangleItem) {
        m_triangleItem->setPos(0, 100);
        m_triangleItem->setRotation(0);
    }
    
    updateStatusLabel();
}

void InteractiveGraphicsItemExample::updateStatusLabel()
{
    QString status = "Ready - Items can be dragged and rotated";
    m_statusLabel->setText(status);
}

// Custom drawing functions
void InteractiveGraphicsItemExample::drawRectangle(QPainter *painter, const QRectF &rect)
{
    painter->save();
    
    // Draw gradient background
    QLinearGradient gradient(rect.topLeft(), rect.bottomRight());
    gradient.setColorAt(0, QColor(100, 150, 255, 150));
    gradient.setColorAt(1, QColor(50, 100, 200, 150));
    
    painter->setBrush(QBrush(gradient));
    painter->setPen(QPen(QColor(0, 100, 200), 2));
    painter->drawRect(rect);
    
    // Draw text
    painter->setPen(QPen(Qt::white));
    painter->drawText(rect, Qt::AlignCenter, "Rectangle");
    
    painter->restore();
}

void InteractiveGraphicsItemExample::drawCircle(QPainter *painter, const QRectF &rect)
{
    painter->save();
    
    // Draw gradient background
    QRadialGradient gradient(rect.center(), rect.width() / 2);
    gradient.setColorAt(0, QColor(255, 100, 100, 150));
    gradient.setColorAt(1, QColor(200, 50, 50, 150));
    
    painter->setBrush(QBrush(gradient));
    painter->setPen(QPen(QColor(200, 50, 50), 2));
    painter->drawEllipse(rect);
    
    // Draw text
    painter->setPen(QPen(Qt::white));
    painter->drawText(rect, Qt::AlignCenter, "Circle");
    
    painter->restore();
}

void InteractiveGraphicsItemExample::drawTriangle(QPainter *painter, const QRectF &rect)
{
    painter->save();
    
    // Create triangle path
    QPainterPath triangle;
    triangle.moveTo(rect.center().x(), rect.top());
    triangle.lineTo(rect.left(), rect.bottom());
    triangle.lineTo(rect.right(), rect.bottom());
    triangle.closeSubpath();
    
    // Draw gradient background
    QLinearGradient gradient(rect.topLeft(), rect.bottomRight());
    gradient.setColorAt(0, QColor(100, 255, 100, 150));
    gradient.setColorAt(1, QColor(50, 200, 50, 150));
    
    painter->setBrush(QBrush(gradient));
    painter->setPen(QPen(QColor(50, 200, 50), 2));
    painter->drawPath(triangle);
    
    // Draw text
    painter->setPen(QPen(Qt::white));
    painter->drawText(rect, Qt::AlignCenter, "Triangle");
    
    painter->restore();
}

void InteractiveGraphicsItemExample::drawStar(QPainter *painter, const QRectF &rect)
{
    painter->save();
    
    // Create star path
    QPainterPath star;
    QPointF center = rect.center();
    qreal outerRadius = qMin(rect.width(), rect.height()) / 2;
    qreal innerRadius = outerRadius * 0.4;
    
    for (int i = 0; i < 10; ++i) {
        qreal angle = i * M_PI / 5;
        qreal radius = (i % 2 == 0) ? outerRadius : innerRadius;
        QPointF point = center + QPointF(radius * cos(angle), radius * sin(angle));
        
        if (i == 0) {
            star.moveTo(point);
        } else {
            star.lineTo(point);
        }
    }
    star.closeSubpath();
    
    // Draw gradient background
    QConicalGradient gradient(center, 0);
    gradient.setColorAt(0, QColor(255, 255, 100, 150));
    gradient.setColorAt(0.5, QColor(255, 200, 50, 150));
    gradient.setColorAt(1, QColor(255, 255, 100, 150));
    
    painter->setBrush(QBrush(gradient));
    painter->setPen(QPen(QColor(255, 200, 50), 2));
    painter->drawPath(star);
    
    // Draw text
    painter->setPen(QPen(Qt::black));
    painter->drawText(rect, Qt::AlignCenter, "Star");
    
    painter->restore();
}

void InteractiveGraphicsItemExample::drawGradient(QPainter *painter, const QRectF &rect)
{
    painter->save();
    
    // Draw complex gradient
    QLinearGradient gradient(rect.topLeft(), rect.bottomRight());
    gradient.setColorAt(0, QColor(255, 100, 255, 150));
    gradient.setColorAt(0.3, QColor(100, 255, 255, 150));
    gradient.setColorAt(0.7, QColor(255, 255, 100, 150));
    gradient.setColorAt(1, QColor(255, 100, 100, 150));
    
    painter->setBrush(QBrush(gradient));
    painter->setPen(QPen(QColor(150, 150, 150), 2));
    painter->drawRect(rect);
    
    // Draw text
    painter->setPen(QPen(Qt::black));
    painter->drawText(rect, Qt::AlignCenter, "Gradient");
    
    painter->restore();
}
