#ifndef MANOEUVREOVERLAY_H
#define MANOEUVREOVERLAY_H

#include "timelineutils.h"
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QDateTime>
#include <vector>

class ManoeuvreOverlay : public QGraphicsView
{
    Q_OBJECT

public:
    explicit ManoeuvreOverlay(QWidget *parent = nullptr);
    ~ManoeuvreOverlay();

    // Set manoeuvres to display
    void setManoeuvres(const std::vector<Manoeuvre> *manoeuvres);
    
    // Set time range for mapping time to Y position
    void setTimeRange(const QDateTime &minTime, const QDateTime &maxTime);
    
    // Update the overlay display
    void updateOverlay();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QGraphicsScene *m_scene;
    const std::vector<Manoeuvre> *m_manoeuvres;
    QDateTime m_minTime;
    QDateTime m_maxTime;
    
    // Helper methods
    qreal timeToY(const QDateTime &time) const;
    void drawManoeuvre(const Manoeuvre &manoeuvre);
    void clearScene();
};

#endif // MANOEUVREOVERLAY_H

