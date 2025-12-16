# Marker Timestamp Value Changed Event Documentation

## Overview

The `markerTimestampValueChanged` signal is emitted by `GraphLayout` whenever a marker is placed or clicked on any waterfall graph. This signal provides the timestamp and value (range) of the marker, allowing external components to respond to marker interactions.

## Signal Signature

```cpp
void markerTimestampValueChanged(const QDateTime &timestamp, qreal value);
```

### Parameters

- **`timestamp`**: The `QDateTime` timestamp of the marker. This represents the time position where the marker is located on the graph.
- **`value`**: The `qreal` value (range) of the marker. This represents the X-axis position/value where the marker is located.

## When the Signal is Emitted

The `markerTimestampValueChanged` signal is emitted in the following scenarios:

1. **New Interactive Marker Placed**: When a user clicks on an empty area of a BTW (Bit Time Waterfall) graph to place a new interactive marker.
2. **Interactive Marker Clicked**: When a user clicks on an existing interactive marker (green circle with line).
3. **Magenta Marker Clicked**: When a user clicks on a magenta circle marker (BTW symbol) that appears on any waterfall graph.

## Signal Propagation Chain

The signal propagates through the following hierarchy:

```
WaterfallGraph::markerTimestampValueChanged
    ↓
GraphContainer::markerTimestampValueChanged (forwarded)
    ↓
GraphLayout::markerTimestampValueChanged (forwarded)
    ↓
Your Component (connected)
```

## Wiring Up the Event

### Method 1: Using Qt's connect() with Lambda (Recommended)

This is the simplest and most flexible approach, especially for quick UI updates:

```cpp
#include "graphlayout.h"
#include <QDateTime>
#include <QLabel>

// Assuming you have a GraphLayout instance
GraphLayout* graphLayout = new GraphLayout(parent, LayoutType::GPW4W, timer, seriesLabelsMap);

// Create a label to display the timestamp
QLabel* timestampLabel = new QLabel("Marker Timestamp: --", parent);

// Connect the signal to update the label
connect(graphLayout, &GraphLayout::markerTimestampValueChanged,
        [timestampLabel](const QDateTime &timestamp, qreal value) {
            if (timestamp.isValid()) {
                QString timestampStr = timestamp.toString("yyyy-MM-dd HH:mm:ss.zzz");
                timestampLabel->setText(
                    QString("Marker Timestamp: %1 | Value: %2")
                        .arg(timestampStr)
                        .arg(value, 0, 'f', 2)
                );
            }
        });
```

### Method 2: Using Qt's connect() with a Slot

For more complex handling, create a dedicated slot in your class:

**Header file (e.g., `myclass.h`):**

```cpp
#include <QObject>
#include <QDateTime>
#include "graphlayout.h"

class MyClass : public QObject
{
    Q_OBJECT

public:
    explicit MyClass(QObject* parent = nullptr);
    
    void setupGraphLayout(GraphLayout* graphLayout);

private slots:
    void onMarkerTimestampValueChanged(const QDateTime &timestamp, qreal value);

private:
    GraphLayout* m_graphLayout;
    // Your other members...
};
```

**Implementation file (e.g., `myclass.cpp`):**

```cpp
#include "myclass.h"
#include <QDebug>

MyClass::MyClass(QObject* parent)
    : QObject(parent), m_graphLayout(nullptr)
{
}

void MyClass::setupGraphLayout(GraphLayout* graphLayout)
{
    m_graphLayout = graphLayout;
    
    // Connect the signal to your slot
    connect(m_graphLayout, &GraphLayout::markerTimestampValueChanged,
            this, &MyClass::onMarkerTimestampValueChanged);
}

void MyClass::onMarkerTimestampValueChanged(const QDateTime &timestamp, qreal value)
{
    if (!timestamp.isValid()) {
        qDebug() << "Invalid timestamp received";
        return;
    }
    
    // Process the marker timestamp and value
    qDebug() << "Marker timestamp:" << timestamp.toString("yyyy-MM-dd HH:mm:ss.zzz");
    qDebug() << "Marker value:" << value;
    
    // Your custom logic here
    // For example: update UI, store in database, trigger analysis, etc.
}
```

### Method 3: Using std::function (Advanced)

For more flexibility, you can use a function object:

```cpp
#include <functional>
#include "graphlayout.h"

// Create a function to handle the event
std::function<void(const QDateTime&, qreal)> markerHandler = 
    [](const QDateTime &timestamp, qreal value) {
        // Your handling code
        qDebug() << "Marker event:" << timestamp << value;
    };

// Note: GraphLayout doesn't directly support std::function,
// so you'll need to create a wrapper slot or use Method 1/2
```

## Complete Example: MainWindow Integration

Here's a complete example from the codebase showing how to wire up the event in a MainWindow:

```cpp
#include "mainwindow.h"
#include "graphlayout.h"
#include <QLabel>
#include <QHBoxLayout>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    // ... other initialization code ...
    
    // Create GraphLayout
    graphgrid = new GraphLayout(ui->originalTab, LayoutType::GPW4W, 
                                timeUpdateTimer, seriesLabelsMap);
    
    // Create a label to display marker timestamp
    markerTimestampLabel = new QLabel("Marker Timestamp: --", ui->originalTab);
    markerTimestampLabel->setStyleSheet(
        "QLabel { color: white; font-size: 14px; font-weight: bold; "
        "background-color: rgba(0, 0, 0, 200); padding: 6px; "
        "border: 2px solid yellow; border-radius: 4px; }"
    );
    
    // Connect the signal to update the label
    connect(graphgrid, &GraphLayout::markerTimestampValueChanged,
            [this](const QDateTime &timestamp, qreal value) {
                if (markerTimestampLabel && timestamp.isValid()) {
                    QString timestampStr = timestamp.toString("yyyy-MM-dd HH:mm:ss.zzz");
                    markerTimestampLabel->setText(
                        QString("Marker Timestamp: %1 | Value: %2")
                            .arg(timestampStr)
                            .arg(value, 0, 'f', 2)
                    );
                }
            });
    
    // ... rest of initialization ...
}
```

## Best Practices

1. **Always Check Timestamp Validity**: The signal may occasionally emit with an invalid timestamp. Always validate before using:
   ```cpp
   if (timestamp.isValid()) {
       // Use timestamp
   }
   ```

2. **Format Timestamps Appropriately**: Use `QDateTime::toString()` with appropriate format strings:
   - `"yyyy-MM-dd HH:mm:ss.zzz"` - Full timestamp with milliseconds
   - `"HH:mm:ss"` - Time only
   - `"yyyy-MM-dd"` - Date only

3. **Handle Value Precision**: The `value` parameter is a `qreal` (double). Format it appropriately for display:
   ```cpp
   QString::number(value, 'f', 2)  // 2 decimal places
   QString::number(value, 'g', 4)  // 4 significant digits
   ```

4. **Thread Safety**: If connecting from a different thread, use `Qt::QueuedConnection`:
   ```cpp
   connect(graphLayout, &GraphLayout::markerTimestampValueChanged,
           this, &MyClass::onMarkerTimestampValueChanged,
           Qt::QueuedConnection);
   ```

5. **Disconnect When Done**: If the GraphLayout instance is destroyed or you no longer need the connection, disconnect:
   ```cpp
   disconnect(graphLayout, &GraphLayout::markerTimestampValueChanged,
              this, &MyClass::onMarkerTimestampValueChanged);
   ```

## Troubleshooting

### Signal Not Firing

1. **Check Connection**: Verify the `connect()` call succeeded. Check for compiler errors.
2. **Verify GraphLayout Instance**: Ensure the `GraphLayout` instance is valid and not null.
3. **Check Signal-Slot Compatibility**: Ensure your slot signature matches the signal exactly.
4. **Enable Debug Output**: Add `qDebug()` statements to verify the signal is being emitted.

### Invalid Timestamps

- The timestamp may be invalid if the marker is placed outside the valid time range.
- Always check `timestamp.isValid()` before using the value.

### Value Out of Range

- The `value` parameter represents the range/X-axis position.
- It may be outside expected bounds if the marker is placed outside the visible area.
- Consider clamping or validating the value for your use case.

## Related Signals

- `BTWManualMarkerPlaced(const QDateTime &timestamp, const QPointF &position)`: Emitted when a BTW marker is placed (includes position).
- `BTWManualMarkerClicked(const QDateTime &timestamp, const QPointF &position)`: Emitted when a BTW marker is clicked (includes position).
- `RTWRMarkerTimestampCaptured(const QDateTime &timestamp, const QPointF &position)`: Emitted when an RTW R marker is clicked.

## See Also

- `GraphLayout` class documentation
- `GraphContainer` class documentation
- `WaterfallGraph` class documentation
- `BTWGraph` class documentation
