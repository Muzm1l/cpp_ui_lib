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
#include <QDebug>

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

    // Label value setters (updates display values)
    void setLeftLabelValue(const qreal value);
    void setCenterLabelValue(const qreal value);
    void setRightLabelValue(const qreal value);
    
    // Original range setters (sets and locks original values used for calculations)
    void setOriginalRangeValues(const qreal leftValue, const qreal centerValue, const qreal rightValue);

    // Getter methods for label values
    const qreal getLeftLabelValue() const;
    const qreal getCenterLabelValue() const;
    const qreal getRightLabelValue() const;

    // User modification tracking
    bool hasUserModifiedBounds() const;
    void resetUserModifiedFlag();

    // Rebase labels to the current bounds and reset indicator to [0,1]
    void rebaseToCurrentBounds();
    
    // Reset indicator to full range [0.0, 1.0] without changing labels
    void resetIndicatorToFullRange();

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

    // Label values (display values shown to user)
    qreal m_leftLabelValue = 0.0;   // Left reference value (display)
    qreal m_centerLabelValue = 0.5; // Center value (display)
    qreal m_rightLabelValue = 1.0;  // Range for upper bound (display)
    
    // Original range values (constant, used for all calculations)
    qreal m_originalLeftLabelValue = 0.0;   // Original left reference value (constant)
    qreal m_originalCenterLabelValue = 0.5; // Original center value (constant)
    qreal m_originalRightLabelValue = 1.0;  // Original right reference value (constant)
    bool m_originalValuesSet = false;  // Track if original values have been initialized

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

    // Indicator bounds (separate from panel range)
    qreal m_indicatorLowerBoundValue;
    qreal m_indicatorUpperBoundValue;

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

    // Helper method to calculate interpolated bounds
    ZoomBounds calculateInterpolatedBounds() const;
    
    // Helper method to update display labels to reflect current selected range
    void updateDisplayLabels();

    // Interpolation Ranges
    const qreal m_interpolationLowerBound = 0.0;
    const qreal m_interpolationUpperBound = 1.0;

};

#endif // ZOOMPANEL_H
