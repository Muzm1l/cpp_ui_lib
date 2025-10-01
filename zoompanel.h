#ifndef ZOOMPANEL_H
#define ZOOMPANEL_H

#include <QBrush>
#include <QFont>
#include <QFrame>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QMouseEvent>
#include <QPen>
#include <QPoint>
#include <QShowEvent>
#include <QThread>
#include <QVBoxLayout>
#include <QWidget>

struct ZoomBounds
{
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

    // Label value setters
    void setLeftLabelValue(const qreal value);
    void setCenterLabelValue(const qreal value);
    void setRightLabelValue(const qreal value);

    // Getter methods for label values
    const qreal getLeftLabelValue() const;
    const qreal getCenterLabelValue() const;
    const qreal getRightLabelValue() const;

    // User modification tracking
    bool hasUserModifiedBounds() const;
    void resetUserModifiedFlag();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    QGraphicsView *m_graphicsView;
    QGraphicsScene *m_scene;
    QGraphicsRectItem *m_indicator;
    QGraphicsTextItem *m_leftText;
    QGraphicsTextItem *m_centerText;
    QGraphicsTextItem *m_rightText;

    // Label values
    qreal leftLabelValue = 0.0;   // Left reference value
    qreal centerLabelValue = 0.5; // Center value
    qreal rightLabelValue = 1.0;  // Range for upper bound

    // Mouse interaction state
    bool m_isDragging;
    bool m_isExtending;
    QPoint m_initialMousePos;
    QPoint m_initialIndicatorPos;
    qreal m_currentValue;

    // Extend mode state
    enum ExtendMode
    {
        None,
        ExtendLeft,
        ExtendRight
    };
    ExtendMode m_extendMode;

    // User modification tracking
    bool m_userModifiedBounds;

    // Actual bounds (separate from panel range)
    qreal m_actualLowerBound;
    qreal m_actualUpperBound;

    void setupGraphicsView();
    void createIndicator();
    void createTextItems();
    void updateIndicator(double value);
    void updateValueFromMousePosition(const QPoint &currentPos);
    void updateAllElements();

    // Extend mode methods
    ExtendMode detectExtendMode(const QPoint &mousePos);
    void updateExtentFromMousePosition(const QPoint &currentPos);
    void updateIndicatorToBounds();
    void updateVisualFeedback();

    // Helper method to calculate optimal font size
    int calculateOptimalFontSize(int maxWidth);
};

#endif // ZOOMPANEL_H
