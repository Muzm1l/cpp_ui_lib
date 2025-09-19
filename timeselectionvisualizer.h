#ifndef TIMESELECTIONVISUALIZER_H
#define TIMESELECTIONVISUALIZER_H

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QPainter>
#include <QPaintEvent>
#include <QTime>
#include <QList>
#include "timelineutils.h"

// Compile-time parameters
#define BUTTON_SIZE 32
#define GRAPHICS_VIEW_WIDTH 32
#define MAX_TIME_SELECTIONS 5



namespace Ui {
class TimeSelectionVisualizer;
}

class TimeVisualizerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TimeVisualizerWidget(QWidget *parent = nullptr);
    
    // Time selection management
    void addTimeSelection(TimeSelectionSpan span);
    void clearTimeSelections();
    
    // Properties
    void setTimeLineLength(const QTime& length);
    void setCurrentTime(const QTime& currentTime);
    
    QTime getTimeLineLength() const { return m_timeLineLength; }
    QTime getCurrentTime() const { return m_currentTime; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QList<TimeSelectionSpan> m_timeSelections;
    QTime m_timeLineLength;
    QTime m_currentTime;
    
    void updateVisualization();
    void drawSelection(QPainter &painter, const TimeSelectionSpan &span);
};

class TimeSelectionVisualizer : public QWidget
{
    Q_OBJECT

public:
    explicit TimeSelectionVisualizer(QWidget *parent = nullptr);
    ~TimeSelectionVisualizer();
    
    // Delegate methods to the visualizer widget
    void addTimeSelection(TimeSelectionSpan span) { m_visualizerWidget->addTimeSelection(span); }
    void clearTimeSelections() { m_visualizerWidget->clearTimeSelections(); }
    void setTimeLineLength(const QTime& length) { m_visualizerWidget->setTimeLineLength(length); }
    void setCurrentTime(const QTime& currentTime) { m_visualizerWidget->setCurrentTime(currentTime); }

private slots:
    void onButtonClicked();

private:
    Ui::TimeSelectionVisualizer *ui;
    QPushButton *m_button;
    TimeVisualizerWidget *m_visualizerWidget;
    QVBoxLayout *m_layout;
};

#endif // TIMESELECTIONVISUALIZER_H
