# Manoeuvre API Documentation

## Overview

The Manoeuvre API allows you to display manoeuvre indicators on timeline views within graph containers. A manoeuvre represents a period of time during which a vessel performs a specific action, characterized by bearing, speed, and depth values.

Manoeuvres are visualized on the timeline as:
- **Chevron shape** at the start time (bottom of the timeline)
- **Horizontal line** at the end time (top of the timeline)
- **Labels** displaying speed (at the top line), bearing (bottom left of chevron), and depth (bottom right of chevron)

## Data Structure

The `Manoeuvre` struct is defined in `timelineutils.h`:

```cpp
struct Manoeuvre
{
    QDateTime startTime;  // Start time of the manoeuvre
    QDateTime endTime;    // End time of the manoeuvre
    int bearing;          // Bearing in degrees (0-359)
    int speed;            // Speed value
    int depth;            // Depth value
};
```

### Constructors

```cpp
// Default constructor
Manoeuvre();

// Parameterized constructor
Manoeuvre(const QDateTime &start, const QDateTime &end, int b, int s, int d);
```

## API Levels

The manoeuvre API is available at two levels:

### 1. GraphLayout Level (Recommended)

Use `GraphLayout` methods when you want to manage manoeuvres across all graph containers in a layout. This is the recommended approach as it automatically synchronizes manoeuvres across all containers.

**Available Methods:**

```cpp
// Add a single manoeuvre
void addManoeuvre(const Manoeuvre &manoeuvre);

// Set all manoeuvres (replaces existing)
void setManoeuvres(const std::vector<Manoeuvre> &manoeuvres);

// Clear all manoeuvres
void clearManoeuvres();

// Get all current manoeuvres
std::vector<Manoeuvre> getManoeuvres() const;
```

### 2. GraphContainer Level

Use `GraphContainer` methods when you need to manage manoeuvres for a specific container. This is typically used internally by `GraphLayout`, but can be used directly if needed.

**Available Methods:**

```cpp
// Set manoeuvres for this container (pass nullptr to clear)
void setManoeuvres(const std::vector<Manoeuvre> *manoeuvres);
```

## Usage Examples

### Example 1: Adding a Single Manoeuvre

```cpp
#include "timelineutils.h"
#include "graphlayout.h"

// Get your GraphLayout instance
GraphLayout *graphLayout = ...;

// Create a manoeuvre
QDateTime now = QDateTime::currentDateTime();
QDateTime startTime = now.addSecs(-180);  // 3 minutes ago
QDateTime endTime = now;                    // Current time

Manoeuvre manoeuvre(
    startTime,   // Start time
    endTime,     // End time
    045,         // Bearing: 45 degrees
    25,          // Speed: 25 knots
    150          // Depth: 150 meters
);

// Add the manoeuvre to the layout
graphLayout->addManoeuvre(manoeuvre);
```

### Example 2: Adding Multiple Manoeuvres

```cpp
#include "timelineutils.h"
#include "graphlayout.h"
#include <vector>

GraphLayout *graphLayout = ...;
QDateTime now = QDateTime::currentDateTime();

// Create multiple manoeuvres
std::vector<Manoeuvre> manoeuvres;

// Manoeuvre 1: Started 10 minutes ago, ended 5 minutes ago
Manoeuvre m1(
    now.addSecs(-600),  // 10 minutes ago
    now.addSecs(-300),  // 5 minutes ago
    090,                // Bearing: 90 degrees
    20,                 // Speed: 20 knots
    100                 // Depth: 100 meters
);
manoeuvres.push_back(m1);

// Manoeuvre 2: Started 3 minutes ago, ending now
Manoeuvre m2(
    now.addSecs(-180),  // 3 minutes ago
    now,                 // Current time
    180,                // Bearing: 180 degrees
    30,                 // Speed: 30 knots
    200                 // Depth: 200 meters
);
manoeuvres.push_back(m2);

// Set all manoeuvres at once
graphLayout->setManoeuvres(manoeuvres);
```

### Example 3: Clearing Manoeuvres

```cpp
// Clear all manoeuvres from the layout
graphLayout->clearManoeuvres();
```

### Example 4: Retrieving Current Manoeuvres

```cpp
// Get all current manoeuvres
std::vector<Manoeuvre> currentManoeuvres = graphLayout->getManoeuvres();

// Iterate through manoeuvres
for (const auto &manoeuvre : currentManoeuvres)
{
    qDebug() << "Manoeuvre:"
             << "Start:" << manoeuvre.startTime.toString("yyyy-MM-dd hh:mm:ss")
             << "End:" << manoeuvre.endTime.toString("yyyy-MM-dd hh:mm:ss")
             << "Bearing:" << manoeuvre.bearing
             << "Speed:" << manoeuvre.speed
             << "Depth:" << manoeuvre.depth;
}
```

### Example 5: Using with GraphContainer (Advanced)

```cpp
#include "graphcontainer.h"
#include "timelineutils.h"
#include <vector>

GraphContainer *container = ...;

// Create manoeuvres
std::vector<Manoeuvre> manoeuvres;
Manoeuvre m(QDateTime::currentDateTime().addSecs(-300),
            QDateTime::currentDateTime(),
            270, 15, 120);
manoeuvres.push_back(m);

// Set manoeuvres for this specific container
container->setManoeuvres(&manoeuvres);

// To clear manoeuvres for this container
container->setManoeuvres(nullptr);
```

### Example 6: Complete Integration Example

```cpp
#include "mainwindow.h"
#include "graphlayout.h"
#include "timelineutils.h"
#include <QPushButton>
#include <QDateTime>

class MyWindow : public QMainWindow
{
    Q_OBJECT

public:
    MyWindow(QWidget *parent = nullptr) : QMainWindow(parent)
    {
        // Create GraphLayout
        graphLayout = new GraphLayout(this, LayoutType::GPW4W, timer);
        
        // Create button to add manoeuvre
        QPushButton *addButton = new QPushButton("Add Manoeuvre", this);
        connect(addButton, &QPushButton::clicked, this, &MyWindow::onAddManoeuvre);
        
        // Create button to clear manoeuvres
        QPushButton *clearButton = new QPushButton("Clear Manoeuvres", this);
        connect(clearButton, &QPushButton::clicked, this, &MyWindow::onClearManoeuvres);
    }

private slots:
    void onAddManoeuvre()
    {
        QDateTime now = QDateTime::currentDateTime();
        QDateTime startTime = now.addSecs(-180);  // 3 minutes ago
        QDateTime endTime = now;                    // Current time
        
        // Generate manoeuvre data
        int bearing = 045;  // 45 degrees
        int speed = 25;     // 25 knots
        int depth = 150;    // 150 meters
        
        Manoeuvre manoeuvre(startTime, endTime, bearing, speed, depth);
        graphLayout->addManoeuvre(manoeuvre);
    }
    
    void onClearManoeuvres()
    {
        graphLayout->clearManoeuvres();
    }

private:
    GraphLayout *graphLayout;
    QTimer *timer;
};
```

## Visual Representation

When a manoeuvre is displayed on the timeline:

```
        
        ┌─────────────────────-─────────────────────--┐ ← End Time (top horizontal line)
        │                                             │
        │                                             │
        │                                             │  ← Chevron Box
        │                                             │
        │                                             │
        │                                             │  
        │            Speed Label (top)                │
        └───────────────\      /──────────────────────┘
        ↑                \    /                       ↑
    Bearing Label         \  /                 Depth Label
    (bottom left)          \/ ← Chevron Tip     (bottom right)
                                (V shape)               
    
    Start Time (bottom)
```

**Key Visual Elements:**
- **Chevron**: Located at the start time (bottom of timeline), pointing upward
- **Horizontal Line**: Located at the end time (top of timeline), spanning the full width
- **Speed Label**: Positioned at the top horizontal line (end time)
- **Bearing Label**: Positioned at the bottom left of the chevron
- **Depth Label**: Positioned at the bottom right of the chevron

## Important Notes

1. **Synchronization**: When using `GraphLayout` methods, manoeuvres are automatically synchronized across all graph containers in the layout.

2. **Time Range**: Manoeuvres are only visible when their time range overlaps with the currently visible time window in the timeline view.

3. **Pointer Lifetime**: When using `GraphContainer::setManoeuvres()`, ensure the `std::vector<Manoeuvre>` remains valid for as long as the manoeuvres need to be displayed. The container stores a pointer to the vector.

4. **Multiple Manoeuvres**: You can add multiple manoeuvres, and they will all be displayed simultaneously on the timeline.

5. **Clearing**: Use `clearManoeuvres()` to remove all manoeuvres, or `setManoeuvres()` with an empty vector.

## Best Practices

1. **Use GraphLayout API**: Prefer using `GraphLayout` methods (`addManoeuvre`, `setManoeuvres`, `clearManoeuvres`) over `GraphContainer` methods for better synchronization.

2. **Time Validation**: Ensure `startTime < endTime` for proper visualization.

3. **Data Ranges**: 
   - Bearing: Typically 0-359 degrees
   - Speed: Use appropriate units for your application
   - Depth: Use appropriate units for your application

4. **Memory Management**: When creating manoeuvres dynamically, ensure proper cleanup when clearing or replacing manoeuvres.

## Troubleshooting

**Manoeuvres not appearing:**
- Check that the manoeuvre's time range overlaps with the visible time window
- Verify that `startTime < endTime`
- Ensure the timeline view is visible (`setShowTimelineView(true)`)

**Manoeuvres not synchronized:**
- Use `GraphLayout` methods instead of `GraphContainer` methods directly
- Ensure all containers are part of the same `GraphLayout` instance

**Performance issues with many manoeuvres:**
- Consider limiting the number of manoeuvres displayed simultaneously
- Use `setManoeuvres()` to replace all manoeuvres at once rather than calling `addManoeuvre()` multiple times

