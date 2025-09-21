#ifndef GRAPHLAYOUT_H
#define GRAPHLAYOUT_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <vector>
#include "graphcontainer.h"


enum class LayoutType {
    GPW1W = 0,      // 1 window only
    GPW4W = 1,      // 4 windows in 2x2 grid
    GPW2WV = 2,     // 2 windows in vertical line
    GPW2WH = 3,     // 2 windows in horizontal line
    GPW4WH = 4,     // 4 windows in horizontal line
    HIDDEN = 5      // Hidden

};


class GraphLayout : public QWidget
{
    Q_OBJECT
public:
    explicit GraphLayout(QWidget *parent = nullptr, LayoutType layoutType = LayoutType::GPW4W);
    ~GraphLayout();

    void setLayoutType(LayoutType layoutType);
    LayoutType getLayoutType() const;
    
        // Sizing methods
        void setGraphViewSize(int width, int height);
        void updateLayoutSizing();

private:
    LayoutType m_layoutType;

    std::vector<GraphContainer*> m_graphContainers;

    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_graphContainersRow1Layout;
    QHBoxLayout *m_graphContainersRow2Layout;
    

signals:
};

#endif // GRAPHLAYOUT_H
