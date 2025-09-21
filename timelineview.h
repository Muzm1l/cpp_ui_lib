#ifndef TIMELINEVIEW_H
#define TIMELINEVIEW_H

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QPainter>
#include <QPaintEvent>
#include <QTime>
#include <QList>
#include "timelineutils.h"

// Compile-time parameters
#define TIMELINE_VIEW_BUTTON_SIZE 64
#define TIMELINE_VIEW_GRAPHICS_VIEW_WIDTH 128

class TimelineVisualizerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TimelineVisualizerWidget(QWidget *parent = nullptr);
    ~TimelineVisualizerWidget();
    
           // No time selection management needed for TimelineView
        
    // Properties
    void setTimeLineLength(const QTime& length);
    void setCurrentTime(const QTime& currentTime);
    void setNumberOfDivisions(int divisions);
    
    QTime getTimeLineLength() const { return m_timeLineLength; }
    QTime getCurrentTime() const { return m_currentTime; }
    int getNumberOfDivisions() const { return m_numberOfDivisions; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QTime m_timeLineLength = QTime(0, 15, 0); // Default to 15 minutes
    QTime m_currentTime;
    int m_numberOfDivisions = 15; // Default to 15 segments
    
    void updateVisualization();
    void drawSegment(QPainter &painter, int segmentNumber);
    void drawChevron(QPainter &painter, int yOffset);
    QString getTimeLabel(int segmentNumber);
    void drawChevronLabels(QPainter &painter, int yOffset);


};
    

class TimelineView : public QWidget
{
    Q_OBJECT

public:
    explicit TimelineView(QWidget *parent = nullptr);
    ~TimelineView();
    
           // No time selection methods needed for TimelineView
    void setTimeLineLength(TimeInterval interval) { 
        m_visualizerWidget->setTimeLineLength(timeIntervalToQTime(interval)); 
        updateButtonText(interval);
    }
    void setCurrentTime(const QTime& currentTime) { m_visualizerWidget->setCurrentTime(currentTime); }
    void setNumberOfDivisions(int divisions) { m_visualizerWidget->setNumberOfDivisions(divisions); }


    signals:
        void intervalChanged(TimeInterval currentInterval);

private:
    QPushButton *m_button;
    TimelineVisualizerWidget *m_visualizerWidget;
    QVBoxLayout *m_layout;
    int intervalIndex = 0;
    
    void updateButtonText(TimeInterval interval);

private slots:
    void onButtonClicked();

};

#endif // TIMELINEVIEW_H
