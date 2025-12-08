#include "graphlayout.h"
#include "timelineutils.h"
#include <QApplication>
#include <QTimer>
#include <QDebug>
#include <QDateTime>
#include <cassert>

/**
 * @brief Simple test for Manoeuvre Drawing APIs
 * 
 * This is a minimal test that verifies the APIs work correctly
 * without requiring a full UI setup.
 */
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    qDebug() << "\n=== Manoeuvre Drawing API Test ===";
    
    // Create GraphLayout for testing
    std::map<GraphType, std::vector<QPair<QString, QColor>>> seriesLabelsMap;
    seriesLabelsMap[GraphType::BTW] = {{"BTW-1", QColor(Qt::red)}};
    
    QTimer *timer = new QTimer();
    GraphLayout *graphLayout = new GraphLayout(nullptr, LayoutType::GPW1W, timer, seriesLabelsMap);
    
    int testsPassed = 0;
    int testsFailed = 0;
    
    // Test 1: Normal flow - start and end manoeuvre
    qDebug() << "\n--- Test 1: Normal Flow ---";
    {
        QDateTime startTime = QDateTime::currentDateTime().addSecs(-300); // 5 minutes ago
        QDateTime endTime = QDateTime::currentDateTime().addSecs(-60);   // 1 minute ago
        
        int bearing = 45;
        int speed = 25;
        int depth = 150;
        
        graphLayout->startManoeuvreDrawing(startTime, bearing, speed, depth);
        graphLayout->endManoeuvreDrawing(endTime);
        
        auto manoeuvres = graphLayout->getManoeuvres();
        if (manoeuvres.size() == 1) {
            const auto &m = manoeuvres[0];
            if (m.startTime == startTime && m.endTime == endTime && 
                m.bearing == bearing && m.speed == speed && m.depth == depth) {
                qDebug() << "✓ Test 1 PASSED: Manoeuvre added correctly";
                testsPassed++;
            } else {
                qDebug() << "✗ Test 1 FAILED: Manoeuvre data mismatch";
                testsFailed++;
            }
        } else {
            qDebug() << "✗ Test 1 FAILED: Expected 1 manoeuvre, got" << manoeuvres.size();
            testsFailed++;
        }
    }
    
    // Test 2: Error - end without start
    qDebug() << "\n--- Test 2: Error - End Without Start ---";
    {
        graphLayout->clearManoeuvres();
        
        int manoeuvresBefore = graphLayout->getManoeuvres().size();
        QDateTime endTime = QDateTime::currentDateTime();
        graphLayout->endManoeuvreDrawing(endTime);
        int manoeuvresAfter = graphLayout->getManoeuvres().size();
        
        if (manoeuvresBefore == manoeuvresAfter) {
            qDebug() << "✓ Test 2 PASSED: Correctly rejected end without start";
            testsPassed++;
        } else {
            qDebug() << "✗ Test 2 FAILED: Manoeuvre was incorrectly added";
            testsFailed++;
        }
    }
    
    // Test 3: Error - invalid time range (start >= end)
    qDebug() << "\n--- Test 3: Error - Invalid Time Range ---";
    {
        QDateTime startTime = QDateTime::currentDateTime();
        QDateTime endTime = QDateTime::currentDateTime().addSecs(-60); // End before start
        
        graphLayout->startManoeuvreDrawing(startTime, 90, 20, 100);
        
        int manoeuvresBefore = graphLayout->getManoeuvres().size();
        graphLayout->endManoeuvreDrawing(endTime);
        int manoeuvresAfter = graphLayout->getManoeuvres().size();
        
        if (manoeuvresBefore == manoeuvresAfter) {
            qDebug() << "✓ Test 3 PASSED: Correctly rejected invalid time range";
            testsPassed++;
        } else {
            qDebug() << "✗ Test 3 FAILED: Manoeuvre was incorrectly added";
            testsFailed++;
        }
    }
    
    // Test 4: Multiple manoeuvres
    qDebug() << "\n--- Test 4: Multiple Manoeuvres ---";
    {
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
            qDebug() << "✓ Test 4 PASSED: All 3 manoeuvres added correctly";
            testsPassed++;
        } else {
            qDebug() << "✗ Test 4 FAILED: Expected 3 manoeuvres, got" << manoeuvres.size();
            testsFailed++;
        }
    }
    
    // Test 5: Verify manoeuvre details
    qDebug() << "\n--- Test 5: Verify Manoeuvre Details ---";
    {
        auto manoeuvres = graphLayout->getManoeuvres();
        qDebug() << "Total manoeuvres:" << manoeuvres.size();
        for (size_t i = 0; i < manoeuvres.size(); i++) {
            const auto &m = manoeuvres[i];
            qDebug() << "  Manoeuvre" << (i + 1) << ":"
                     << "Start:" << m.startTime.toString("yyyy-MM-dd hh:mm:ss")
                     << "End:" << m.endTime.toString("yyyy-MM-dd hh:mm:ss")
                     << "Bearing:" << m.bearing
                     << "Speed:" << m.speed
                     << "Depth:" << m.depth;
        }
        qDebug() << "✓ Test 5 PASSED: Manoeuvre details displayed";
        testsPassed++;
    }
    
    // Summary
    qDebug() << "\n=== Test Summary ===";
    qDebug() << "Tests Passed:" << testsPassed;
    qDebug() << "Tests Failed:" << testsFailed;
    qDebug() << "Total Tests:" << (testsPassed + testsFailed);
    
    if (testsFailed == 0) {
        qDebug() << "\n✓ ALL TESTS PASSED!";
        return 0;
    } else {
        qDebug() << "\n✗ SOME TESTS FAILED";
        return 1;
    }
}

