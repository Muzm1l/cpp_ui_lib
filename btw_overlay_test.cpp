#include "btwgraph.h"
#include "btwinteractiveoverlay.h"
#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QDebug>

/**
 * @brief Test application for BTW Interactive Overlay
 * 
 * This demonstrates the BTW graph with interactive overlay functionality:
 * - Interactive markers that can be dragged and rotated
 * - Custom drawing functions for different marker types
 * - Integration with BTW graph data
 * - Visual feedback and interaction controls
 */
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Create main window
    QMainWindow mainWindow;
    mainWindow.setWindowTitle("BTW Interactive Overlay Test");
    mainWindow.setMinimumSize(800, 600);
    
    // Create central widget
    QWidget *centralWidget = new QWidget(&mainWindow);
    mainWindow.setCentralWidget(centralWidget);
    
    // Create layout
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    
    // Create BTW graph
    BTWGraph *btwGraph = new BTWGraph();
    btwGraph->setMinimumSize(600, 400);
    
    // Add BTW graph to layout
    mainLayout->addWidget(btwGraph, 2);
    
    // Create test controls
    QWidget *testControls = new QWidget();
    QVBoxLayout *controlsLayout = new QVBoxLayout(testControls);
    
    // Test buttons
    QPushButton *addDataPointButton = new QPushButton("Add Data Point");
    QPushButton *addReferenceLineButton = new QPushButton("Add Reference Line");
    QPushButton *addAnnotationButton = new QPushButton("Add Annotation");
    QPushButton *addCustomMarkerButton = new QPushButton("Add Custom Marker");
    QPushButton *clearAllButton = new QPushButton("Clear All");
    QPushButton *toggleOverlayButton = new QPushButton("Toggle Overlay");
    
    // Connect test buttons
    QObject::connect(addDataPointButton, &QPushButton::clicked, [btwGraph]() {
        if (btwGraph->getInteractiveOverlay()) {
            QPointF position(100, 100);
            QDateTime timestamp = QDateTime::currentDateTime();
            qreal value = 42.5;
            btwGraph->getInteractiveOverlay()->addDataPointMarker(position, timestamp, value, "Test");
        }
    });
    
    QObject::connect(addReferenceLineButton, &QPushButton::clicked, [btwGraph]() {
        if (btwGraph->getInteractiveOverlay()) {
            QPointF startPos(50, 50);
            QPointF endPos(150, 150);
            btwGraph->getInteractiveOverlay()->addReferenceLineMarker(startPos, endPos, "Test Line");
        }
    });
    
    QObject::connect(addAnnotationButton, &QPushButton::clicked, [btwGraph]() {
        if (btwGraph->getInteractiveOverlay()) {
            QPointF position(200, 100);
            QString text = "Test Annotation";
            QColor color = Qt::yellow;
            btwGraph->getInteractiveOverlay()->addAnnotationMarker(position, text, color);
        }
    });
    
    QObject::connect(addCustomMarkerButton, &QPushButton::clicked, [btwGraph]() {
        if (btwGraph->getInteractiveOverlay()) {
            QPointF position(150, 200);
            
            auto drawFunction = [](QPainter *painter, const QRectF &rect) {
                painter->save();
                painter->setBrush(QBrush(Qt::green));
                painter->setPen(QPen(Qt::darkGreen, 2));
                painter->drawEllipse(rect);
                painter->setPen(QPen(Qt::white));
                painter->drawText(rect, Qt::AlignCenter, "CUSTOM");
                painter->restore();
            };
            
            btwGraph->getInteractiveOverlay()->addCustomMarker(position, drawFunction, "Custom Test");
        }
    });
    
    QObject::connect(clearAllButton, &QPushButton::clicked, [btwGraph]() {
        if (btwGraph->getInteractiveOverlay()) {
            btwGraph->getInteractiveOverlay()->clearAllMarkers();
        }
    });
    
    QObject::connect(toggleOverlayButton, &QPushButton::clicked, [btwGraph]() {
        bool currentState = btwGraph->isInteractiveOverlayEnabled();
        btwGraph->setInteractiveOverlayEnabled(!currentState);
        qDebug() << "BTW Overlay toggled to:" << !currentState;
    });
    
    // Add buttons to layout
    controlsLayout->addWidget(addDataPointButton);
    controlsLayout->addWidget(addReferenceLineButton);
    controlsLayout->addWidget(addAnnotationButton);
    controlsLayout->addWidget(addCustomMarkerButton);
    controlsLayout->addWidget(clearAllButton);
    controlsLayout->addWidget(toggleOverlayButton);
    controlsLayout->addStretch();
    
    // Add test controls to main layout
    mainLayout->addWidget(testControls, 1);
    
    // Show main window
    mainWindow.show();
    
    qDebug() << "BTW Interactive Overlay Test started";
    qDebug() << "Features available:";
    qDebug() << "- Interactive markers with drag and rotate regions";
    qDebug() << "- Custom drawing functions";
    qDebug() << "- Integration with BTW graph overlay scene";
    qDebug() << "- Visual feedback and interaction controls";
    
    return app.exec();
}
