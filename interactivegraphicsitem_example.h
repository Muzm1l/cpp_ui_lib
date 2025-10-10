#ifndef INTERACTIVEGRAPHICSITEM_EXAMPLE_H
#define INTERACTIVEGRAPHICSITEM_EXAMPLE_H

#include "interactivegraphicsitem.h"
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QGroupBox>
#include <QWidget>
#include <QApplication>
#include <QMainWindow>

/**
 * @brief Example usage of InteractiveGraphicsItem
 * 
 * This class demonstrates how to use the InteractiveGraphicsItem with:
 * - Custom drawing functions
 * - Drag and rotate regions
 * - Visual feedback
 * - Configuration options
 */
class InteractiveGraphicsItemExample : public QMainWindow
{
    Q_OBJECT

public:
    explicit InteractiveGraphicsItemExample(QWidget *parent = nullptr);
    ~InteractiveGraphicsItemExample();

private slots:
    void onDragRegionToggled(bool enabled);
    void onRotateRegionToggled(bool enabled);
    void onCustomRegionsToggled(bool enabled);
    void onDragEnabledToggled(bool enabled);
    void onRotateEnabledToggled(bool enabled);
    void onRotationStepChanged(int value);
    void onItemDragged(const QPointF &delta);
    void onItemRotated(qreal angle);
    void onRegionClicked(InteractiveGraphicsItem::InteractionRegion region, const QPointF &position);
    void onAddCustomRegion();
    void onClearCustomRegions();
    void onResetItem();

private:
    void setupUI();
    void setupGraphicsScene();
    void createExampleItems();
    void updateStatusLabel();

    // UI components
    QWidget *m_centralWidget;
    QHBoxLayout *m_mainLayout;
    QVBoxLayout *m_controlLayout;
    QGraphicsView *m_graphicsView;
    QGraphicsScene *m_scene;
    
    // Control widgets
    QGroupBox *m_regionGroup;
    QCheckBox *m_showDragRegionCheck;
    QCheckBox *m_showRotateRegionCheck;
    QCheckBox *m_showCustomRegionsCheck;
    
    QGroupBox *m_interactionGroup;
    QCheckBox *m_dragEnabledCheck;
    QCheckBox *m_rotateEnabledCheck;
    QSpinBox *m_rotationStepSpin;
    
    QGroupBox *m_customRegionGroup;
    QPushButton *m_addCustomRegionButton;
    QPushButton *m_clearCustomRegionsButton;
    
    QPushButton *m_resetButton;
    QLabel *m_statusLabel;
    
    // Example items
    InteractiveGraphicsItem *m_rectangleItem;
    InteractiveGraphicsItem *m_circleItem;
    InteractiveGraphicsItem *m_triangleItem;
    
    // Custom drawing functions
    static void drawRectangle(QPainter *painter, const QRectF &rect);
    static void drawCircle(QPainter *painter, const QRectF &rect);
    static void drawTriangle(QPainter *painter, const QRectF &rect);
    static void drawStar(QPainter *painter, const QRectF &rect);
    static void drawGradient(QPainter *painter, const QRectF &rect);
};

#endif // INTERACTIVEGRAPHICSITEM_EXAMPLE_H
