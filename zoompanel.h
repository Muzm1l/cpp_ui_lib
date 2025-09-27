#ifndef ZOOMPANEL_H
#define ZOOMPANEL_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QThread>
#include <QMouseEvent>
#include <QPoint>
#include <QFrame>
#include <QVBoxLayout>

struct ZoomBounds {
    qreal upperbound;
    qreal lowerbound;
};

class ZoomPanel : public QWidget
{
    Q_OBJECT

signals:
    void valueChanged(ZoomBounds bounds);

public:
    explicit ZoomPanel(QWidget *parent = nullptr);
    ~ZoomPanel();

    void setIndicatorValue(double value);
    void testIndicator(); // Test method to demonstrate functionality
    
    // Label value setters
    void setLeftLabelValue(qreal value);
    void setCenterLabelValue(qreal value);
    void setRightLabelValue(qreal value);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QGraphicsView *m_graphicsView;
    QGraphicsScene *m_scene;
    QGraphicsRectItem *m_indicator;
    QGraphicsTextItem *m_leftText;
    QGraphicsTextItem *m_centerText;
    QGraphicsTextItem *m_rightText;
    
    // Label values
    qreal leftLabelValue = 0.0;  // Left reference value
    qreal centerLabelValue = 0.5; // Center value
    qreal rightLabelValue = 1.0;  // Range for upper bound
    
    // Mouse interaction state
    bool m_isDragging;
    QPoint m_initialMousePos;
    qreal m_currentValue;
    bool m_startedFromRightHalf;

    void setupGraphicsView();
    void createIndicator();
    void createTextItems();
    void updateIndicator(double value);
    void updateValueFromMousePosition(const QPoint &currentPos);
    void updateAllElements();
    
    // Helper method to calculate optimal font size
    int calculateOptimalFontSize(int maxWidth);
};

#endif // ZOOMPANEL_H
