#include "graphlayout.h"
#include "timelineutils.h"
#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QDebug>
#include <QDateTime>

/**
 * @brief Test application for Manoeuvre Drawing APIs
 * 
 * This test demonstrates the new two-step manoeuvre drawing API:
 * - startManoeuvreDrawing() - begins a manoeuvre with start time and parameters
 * - endManoeuvreDrawing() - completes the manoeuvre with end time
 * 
 * Tests include:
 * - Normal flow: start -> end
 * - Error handling: ending without starting
 * - Error handling: invalid time ranges
 * - Verification: checking manoeuvre was added correctly
 */
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Create main window
    QMainWindow mainWindow;
    mainWindow.setWindowTitle("Manoeuvre Drawing API Test");
    mainWindow.setMinimumSize(1200, 800);
    
    // Create central widget
    QWidget *centralWidget = new QWidget(&mainWindow);
    mainWindow.setCentralWidget(centralWidget);
    
    // Create layout
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    // Create test controls panel
    QWidget *testControls = new QWidget();
    QVBoxLayout *controlsLayout = new QVBoxLayout(testControls);
    
    // Create GraphLayout for testing
    std::map<GraphType, std::vector<QPair<QString, QColor>>> seriesLabelsMap;
    seriesLabelsMap[GraphType::BTW] = {{"BTW-1", QColor(Qt::red)}};
    
    QTimer *timer = new QTimer(&mainWindow);
    GraphLayout *graphLayout = new GraphLayout(centralWidget, LayoutType::GPW1W, timer, seriesLabelsMap);
    graphLayout->setMinimumSize(1000, 600);
    
    // Status label
    QLabel *statusLabel = new QLabel("Status: Ready");
    statusLabel->setStyleSheet("QLabel { background-color: #f0f0f0; padding: 10px; border: 1px solid #ccc; }");
    controlsLayout->addWidget(statusLabel);
    
    // Test buttons
    QPushButton *testNormalFlowButton = new QPushButton("Test: Normal Flow (Start -> End)");
    QPushButton *testErrorNoStartButton = new QPushButton("Test: Error - End Without Start");
    QPushButton *testErrorInvalidTimeButton = new QPushButton("Test: Error - Invalid Time Range");
    QPushButton *testMultipleManoeuvresButton = new QPushButton("Test: Multiple Manoeuvres");
    QPushButton *verifyManoeuvresButton = new QPushButton("Verify: List All Manoeuvres");
    QPushButton *clearManoeuvresButton = new QPushButton("Clear All Manoeuvres");
    
    // Test 1: Normal flow - start and end manoeuvre
    QObject::connect(testNormalFlowButton, &QPushButton::clicked, [graphLayout, statusLabel]() {
        qDebug() << "\n=== Test 1: Normal Flow ===";
        
        QDateTime startTime = QDateTime::currentDateTime().addSecs(-300); // 5 minutes ago
        QDateTime endTime = QDateTime::currentDateTime().addSecs(-60);   // 1 minute ago
        
        int bearing = 45;
        int speed = 25;
        int depth = 150;
        
        qDebug() << "Starting manoeuvre drawing...";
        graphLayout->startManoeuvreDrawing(startTime, bearing, speed, depth);
        
        qDebug() << "Ending manoeuvre drawing...";
        graphLayout->endManoeuvreDrawing(endTime);
        
        // Verify
        auto manoeuvres = graphLayout->getManoeuvres();
        if (manoeuvres.size() > 0) {
            const auto &m = manoeuvres.back();
            bool success = (m.startTime == startTime && 
                           m.endTime == endTime && 
                           m.bearing == bearing && 
                           m.speed == speed && 
                           m.depth == depth);
            
            if (success) {
                statusLabel->setText(QString("✓ Test 1 PASSED: Manoeuvre added correctly (Total: %1)").arg(manoeuvres.size()));
                statusLabel->setStyleSheet("QLabel { background-color: #d4edda; padding: 10px; border: 1px solid #c3e6cb; }");
                qDebug() << "✓ Test 1 PASSED: Manoeuvre verified";
            } else {
                statusLabel->setText("✗ Test 1 FAILED: Manoeuvre data mismatch");
                statusLabel->setStyleSheet("QLabel { background-color: #f8d7da; padding: 10px; border: 1px solid #f5c6cb; }");
                qDebug() << "✗ Test 1 FAILED: Data mismatch";
            }
        } else {
            statusLabel->setText("✗ Test 1 FAILED: No manoeuvre found");
            statusLabel->setStyleSheet("QLabel { background-color: #f8d7da; padding: 10px; border: 1px solid #f5c6cb; }");
            qDebug() << "✗ Test 1 FAILED: No manoeuvre added";
        }
    });
    
    // Test 2: Error - end without start
    QObject::connect(testErrorNoStartButton, &QPushButton::clicked, [graphLayout, statusLabel]() {
        qDebug() << "\n=== Test 2: Error - End Without Start ===";
        
        // Clear any existing drawing state by starting and resetting
        graphLayout->clearManoeuvres();
        
        QDateTime endTime = QDateTime::currentDateTime();
        
        // Try to end without starting
        qDebug() << "Attempting to end manoeuvre without starting...";
        int manoeuvresBefore = graphLayout->getManoeuvres().size();
        graphLayout->endManoeuvreDrawing(endTime);
        int manoeuvresAfter = graphLayout->getManoeuvres().size();
        
        if (manoeuvresBefore == manoeuvresAfter) {
            statusLabel->setText("✓ Test 2 PASSED: Correctly rejected end without start");
            statusLabel->setStyleSheet("QLabel { background-color: #d4edda; padding: 10px; border: 1px solid #c3e6cb; }");
            qDebug() << "✓ Test 2 PASSED: No manoeuvre added (expected)";
        } else {
            statusLabel->setText("✗ Test 2 FAILED: Manoeuvre was added (should have been rejected)");
            statusLabel->setStyleSheet("QLabel { background-color: #f8d7da; padding: 10px; border: 1px solid #f5c6cb; }");
            qDebug() << "✗ Test 2 FAILED: Manoeuvre was incorrectly added";
        }
    });
    
    // Test 3: Error - invalid time range (start >= end)
    QObject::connect(testErrorInvalidTimeButton, &QPushButton::clicked, [graphLayout, statusLabel]() {
        qDebug() << "\n=== Test 3: Error - Invalid Time Range ===";
        
        QDateTime startTime = QDateTime::currentDateTime();
        QDateTime endTime = QDateTime::currentDateTime().addSecs(-60); // End before start
        
        int bearing = 90;
        int speed = 20;
        int depth = 100;
        
        qDebug() << "Starting manoeuvre drawing...";
        graphLayout->startManoeuvreDrawing(startTime, bearing, speed, depth);
        
        qDebug() << "Attempting to end with invalid time range (end < start)...";
        int manoeuvresBefore = graphLayout->getManoeuvres().size();
        graphLayout->endManoeuvreDrawing(endTime);
        int manoeuvresAfter = graphLayout->getManoeuvres().size();
        
        if (manoeuvresBefore == manoeuvresAfter) {
            statusLabel->setText("✓ Test 3 PASSED: Correctly rejected invalid time range");
            statusLabel->setStyleSheet("QLabel { background-color: #d4edda; padding: 10px; border: 1px solid #c3e6cb; }");
            qDebug() << "✓ Test 3 PASSED: No manoeuvre added (expected)";
        } else {
            statusLabel->setText("✗ Test 3 FAILED: Manoeuvre was added (should have been rejected)");
            statusLabel->setStyleSheet("QLabel { background-color: #f8d7da; padding: 10px; border: 1px solid #f5c6cb; }");
            qDebug() << "✗ Test 3 FAILED: Manoeuvre was incorrectly added";
        }
    });
    
    // Test 4: Multiple manoeuvres
    QObject::connect(testMultipleManoeuvresButton, &QPushButton::clicked, [graphLayout, statusLabel]() {
        qDebug() << "\n=== Test 4: Multiple Manoeuvres ===";
        
        graphLayout->clearManoeuvres();
        
        QDateTime now = QDateTime::currentDateTime();
        
        // Add 3 manoeuvres
        for (int i = 0; i < 3; i++) {
            QDateTime startTime = now.addSecs(-600 + (i * 120)); // 10, 8, 6 minutes ago
            QDateTime endTime = now.addSecs(-480 + (i * 120));   // 8, 6, 4 minutes ago
            
            graphLayout->startManoeuvreDrawing(startTime, 45 + (i * 30), 20 + i, 100 + (i * 20));
            graphLayout->endManoeuvreDrawing(endTime);
        }
        
        auto manoeuvres = graphLayout->getManoeuvres();
        if (manoeuvres.size() == 3) {
            statusLabel->setText(QString("✓ Test 4 PASSED: %1 manoeuvres added correctly").arg(manoeuvres.size()));
            statusLabel->setStyleSheet("QLabel { background-color: #d4edda; padding: 10px; border: 1px solid #c3e6cb; }");
            qDebug() << "✓ Test 4 PASSED: All 3 manoeuvres added";
        } else {
            statusLabel->setText(QString("✗ Test 4 FAILED: Expected 3, got %1").arg(manoeuvres.size()));
            statusLabel->setStyleSheet("QLabel { background-color: #f8d7da; padding: 10px; border: 1px solid #f5c6cb; }");
            qDebug() << "✗ Test 4 FAILED: Expected 3 manoeuvres, got" << manoeuvres.size();
        }
    });
    
    // Verify: List all manoeuvres
    QObject::connect(verifyManoeuvresButton, &QPushButton::clicked, [graphLayout, statusLabel]() {
        qDebug() << "\n=== Verifying All Manoeuvres ===";
        
        auto manoeuvres = graphLayout->getManoeuvres();
        qDebug() << "Total manoeuvres:" << manoeuvres.size();
        
        QString info = QString("Total Manoeuvres: %1\n").arg(manoeuvres.size());
        for (size_t i = 0; i < manoeuvres.size(); i++) {
            const auto &m = manoeuvres[i];
            qDebug() << "Manoeuvre" << (i + 1) << ":"
                     << "Start:" << m.startTime.toString("yyyy-MM-dd hh:mm:ss")
                     << "End:" << m.endTime.toString("yyyy-MM-dd hh:mm:ss")
                     << "Bearing:" << m.bearing
                     << "Speed:" << m.speed
                     << "Depth:" << m.depth;
            
            info += QString("  %1: %2 - %3 (B:%4, S:%5, D:%6)\n")
                    .arg(i + 1)
                    .arg(m.startTime.toString("hh:mm:ss"))
                    .arg(m.endTime.toString("hh:mm:ss"))
                    .arg(m.bearing)
                    .arg(m.speed)
                    .arg(m.depth);
        }
        
        statusLabel->setText(info);
        statusLabel->setStyleSheet("QLabel { background-color: #d1ecf1; padding: 10px; border: 1px solid #bee5eb; }");
    });
    
    // Clear all manoeuvres
    QObject::connect(clearManoeuvresButton, &QPushButton::clicked, [graphLayout, statusLabel]() {
        graphLayout->clearManoeuvres();
        statusLabel->setText("All manoeuvres cleared");
        statusLabel->setStyleSheet("QLabel { background-color: #f0f0f0; padding: 10px; border: 1px solid #ccc; }");
        qDebug() << "All manoeuvres cleared";
    });
    
    // Add buttons to layout
    controlsLayout->addWidget(testNormalFlowButton);
    controlsLayout->addWidget(testErrorNoStartButton);
    controlsLayout->addWidget(testErrorInvalidTimeButton);
    controlsLayout->addWidget(testMultipleManoeuvresButton);
    controlsLayout->addWidget(verifyManoeuvresButton);
    controlsLayout->addWidget(clearManoeuvresButton);
    controlsLayout->addStretch();
    
    // Create horizontal layout for graph and controls
    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->addWidget(graphLayout, 2);
    contentLayout->addWidget(testControls, 1);
    
    mainLayout->addLayout(contentLayout);
    
    // Show main window
    mainWindow.show();
    
    qDebug() << "=== Manoeuvre Drawing API Test Started ===";
    qDebug() << "Available tests:";
    qDebug() << "1. Normal Flow - Start and end a manoeuvre";
    qDebug() << "2. Error Handling - End without start";
    qDebug() << "3. Error Handling - Invalid time range";
    qDebug() << "4. Multiple Manoeuvres - Add several manoeuvres";
    qDebug() << "5. Verify - List all manoeuvres";
    qDebug() << "6. Clear - Remove all manoeuvres";
    
    return app.exec();
}

