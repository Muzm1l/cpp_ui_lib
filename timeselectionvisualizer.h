#ifndef TIMESELECTIONVISUALIZER_H
#define TIMESELECTIONVISUALIZER_H

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QPainter>
#include <QPaintEvent>

// Compile-time parameters
#define BUTTON_SIZE 32
#define GRAPHICS_VIEW_WIDTH 32

namespace Ui {
class TimeSelectionVisualizer;
}

class TimeVisualizerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TimeVisualizerWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
};

class TimeSelectionVisualizer : public QWidget
{
    Q_OBJECT

public:
    explicit TimeSelectionVisualizer(QWidget *parent = nullptr);
    ~TimeSelectionVisualizer();

private:
    Ui::TimeSelectionVisualizer *ui;
    QPushButton *m_button;
    TimeVisualizerWidget *m_visualizerWidget;
    QVBoxLayout *m_layout;
};

#endif // TIMESELECTIONVISUALIZER_H
