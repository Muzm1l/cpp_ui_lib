#ifndef GRAPHCONTAINER_H
#define GRAPHCONTAINER_H

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QComboBox>
#include "waterfallgraph.h"
#include "timeselectionvisualizer.h"
#include "timelineview.h"
#include "zoompanel.h"

class GraphContainer : public QWidget
{
    Q_OBJECT
public:
    explicit GraphContainer(QWidget *parent = nullptr, bool showTimelineView = true);

signals:
    void NewTimeSelectionCreated(qreal startTime, qreal endTime);
    void DeltaTimeSelectionChanged(qreal deltaTime);
    void TimeSelectionsCleared();

private:
    QHBoxLayout *m_mainLayout;
    QVBoxLayout *m_leftLayout;
    QComboBox *m_comboBox;
    ZoomPanel *m_zoomPanel;
    waterfallgraph *m_waterfallGraph;
    TimeSelectionVisualizer *m_timelineSelectionView;
    TimelineView *m_timelineView;
    bool m_showTimelineView;
};

#endif // GRAPHCONTAINER_H
