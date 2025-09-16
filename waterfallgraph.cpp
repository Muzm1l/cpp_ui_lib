#include "waterfallgraph.h"
#include "ui_waterfallgraph.h"

waterfallgraph::waterfallgraph(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::waterfallgraph)
    , graphicsView(nullptr)
    , graphicsScene(nullptr)
{
    ui->setupUi(this);
    
    // Create graphics scene
    graphicsScene = new QGraphicsScene(this);
    graphicsScene->setSceneRect(0, 0, 600, 600);
    
    // Create graphics view
    graphicsView = new QGraphicsView(graphicsScene, this);
    graphicsView->setGeometry(10, 10, 680, 680);
    graphicsView->setRenderHint(QPainter::Antialiasing);
    graphicsView->setDragMode(QGraphicsView::RubberBandDrag);
    
    // Draw test geometry using drawutils
    DrawUtils::drawDefaultTestPattern(graphicsScene);
    
    // Add some additional test patterns
    QRectF testRect(50, 50, 200, 150);
    DrawUtils::addTestPattern(graphicsScene, testRect);
    
    // Add a test line
    QLineF testLine(100, 100, 400, 300);
    DrawUtils::addTestLine(graphicsScene, testLine);
    
    // Add a course vector
    QPointF startPoint(300, 300);
    DrawUtils::drawCourseVector(graphicsScene, startPoint, 100, 45, Qt::red);
}

waterfallgraph::~waterfallgraph()
{
    delete ui;
    // Note: graphicsView and graphicsScene are child widgets/scenes, 
    // so they will be automatically deleted by Qt's parent-child mechanism
}
