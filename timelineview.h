#ifndef TIMELINEVIEW_H
#define TIMELINEVIEW_H

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QPainter>
#include <QPaintEvent>
#include <QTime>
#include <QList>
#include <QObject>
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
        
    // Properties
    void setTimeLineLength(const QTime& length);
    void setCurrentTime(const QTime& currentTime);
    void setNumberOfDivisions(int divisions);
    void setIsAbsoluteTime(bool isAbsoluteTime);
    QTime getTimeLineLength() const { return m_timeLineLength; }
    QTime getCurrentTime() const { return m_currentTime; }
    int getNumberOfDivisions() const { return m_numberOfDivisions; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QTime m_timeLineLength = QTime(0, 15, 0); // Default to 15 minutes
    QTime m_currentTime;
    int m_numberOfDivisions = 15; // Default to 15 segments
    bool m_isAbsoluteTime = true;
    
    // Smooth shifting state
    QTime m_lastCurrentTime;
    double m_pixelSpeed; // pixels per second
    double m_accumulatedOffset; // accumulated pixel offset
    
    void updateVisualization();
    void drawSegment(QPainter &painter, int segmentNumber);
    void drawChevron(QPainter &painter, int yOffset);
    QString getTimeLabel(int segmentNumber, bool isAbsoluteTime);
    void drawChevronLabels(QPainter &painter, int yOffset);
    double calculateTimeOffset();
    void updatePixelSpeed();
    double calculateSmoothOffset();

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
    QPushButton *m_intervalChangeButton;
    QPushButton *m_timeModeChangeButton;
    TimelineVisualizerWidget *m_visualizerWidget;
    QVBoxLayout *m_layout;
    int intervalIndex = 0;
    bool m_isAbsoluteTime = true;
    
    void updateButtonText(TimeInterval interval);
    void updateTimeModeButtonText(bool isAbsoluteTime);

private slots:
    void onIntervalButtonClicked();
    void onTimeModeButtonClicked();

};

#endif // TIMELINEVIEW_H
