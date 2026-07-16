#ifndef CHEVRONCOMBOBOX_H
#define CHEVRONCOMBOBOX_H

#include <QComboBox>
#include <QColor>

/**
 * @brief A QComboBox whose drop-down indicator is drawn as a circle enclosing a
 *        downward double-chevron (like a "expand / scroll down" button).
 *
 * The native arrow glyph is suppressed and replaced with a vector-drawn circular
 * double-chevron so it stays crisp at any DPI. The indicator's color is fully
 * customizable at runtime.
 *
 * Usage:
 *   auto *combo = new ChevronComboBox(parent);
 *   combo->setArrowColor(QColor("#00E5FF")); // cyan chevron/circle
 *
 * The whole widget still behaves like a normal QComboBox (click anywhere to open
 * the popup); the circular indicator is purely the visual affordance.
 */
class ChevronComboBox : public QComboBox
{
    Q_OBJECT
    Q_PROPERTY(QColor arrowColor READ arrowColor WRITE setArrowColor)
    Q_PROPERTY(bool showCircle READ showCircle WRITE setShowCircle)

public:
    explicit ChevronComboBox(QWidget *parent = nullptr);

    /** Color used for the circle outline and the double-chevron. */
    QColor arrowColor() const { return m_arrowColor; }
    void setArrowColor(const QColor &color);

    /** Whether the enclosing circle is drawn (chevron is always drawn). */
    bool showCircle() const { return m_showCircle; }
    void setShowCircle(bool show);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QColor m_arrowColor;
    bool m_showCircle = true;
};

#endif // CHEVRONCOMBOBOX_H
