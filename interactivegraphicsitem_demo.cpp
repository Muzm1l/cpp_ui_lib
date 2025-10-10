#include "interactivegraphicsitem_example.h"
#include <QApplication>
#include <QDebug>

/**
 * @brief Demo application for InteractiveGraphicsItem
 * 
 * This demonstrates the InteractiveGraphicsItem class with:
 * - Drag regions for moving items
 * - Rotate regions for rotating items  
 * - Custom drawing functions
 * - Visual feedback and interaction controls
 */
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Create and show the example window
    InteractiveGraphicsItemExample example;
    example.show();
    
    qDebug() << "InteractiveGraphicsItem Demo started";
    qDebug() << "Features demonstrated:";
    qDebug() << "- Drag regions: Click and drag items around the scene";
    qDebug() << "- Rotate regions: Click and drag the corner regions to rotate items";
    qDebug() << "- Custom drawing: Each item has a custom drawing function";
    qDebug() << "- Visual feedback: Regions are highlighted with different colors";
    qDebug() << "- Interactive controls: Toggle visibility and functionality";
    
    return app.exec();
}
