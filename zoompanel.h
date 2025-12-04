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

    // Sticker value setters (updates sticker values used when zoomer is customized)
    void setLeftLabelValue(const qreal value);
    void setCenterLabelValue(const qreal value);
    void setRightLabelValue(const qreal value);
    
    // Original range setters (sets and locks original values set during initialization, used for calculations)
    void setOriginalRangeValues(const qreal leftValue, const qreal centerValue, const qreal rightValue);

    // Getter methods for label values
    qreal getLeftLabelValue() const;
    qreal getCenterLabelValue() const;
    qreal getRightLabelValue() const;

    // User modification tracking
    bool hasUserModifiedBounds() const;
    void resetUserModifiedFlag();

    // Rebase labels to the current bounds and reset indicator to [0,1]
    void rebaseToCurrentBounds();
    
    // Reset indicator to full range [0.0, 1.0] without changing labels
    void resetIndicatorToFullRange();
    
    // Crosshair label methods
    void updateCrosshairLabel(qreal xPosition);
    void clearCrosshairLabel();

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
    QGraphicsTextItem *m_crosshairLabel;
    QGraphicsRectItem *m_crosshairLabelBackground;

    // Sticker values (used when zoomer is customized - shown to user)
    qreal m_leftLabelValue = 0.0;   // Sticker left value (used when customized)
    qreal m_centerLabelValue = 0.5; // Sticker center value (used when customized)
    qreal m_rightLabelValue = 1.0;  // Sticker right value (used when customized)
    
    // Original values (set during initialization - constant, used for all calculations)
    qreal m_originalLeftLabelValue = 0.0;   // Original left value (set during init, constant)
    qreal m_originalCenterLabelValue = 0.5; // Original center value (set during init, constant)
    qreal m_originalRightLabelValue = 1.0;  // Original right value (set during init, constant)
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
    void createCrosshairLabel();
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
    
    // Helper method to update sticker labels to reflect current selected range
    void updateDisplayLabels();

    // Interpolation Ranges
    const qreal m_interpolationLowerBound = 0.0;
    const qreal m_interpolationUpperBound = 1.0;

};

#endif // ZOOMPANEL_H
