# Manual Marker Bearing Rate Box Formula

This document explains how the bearing rate value displayed in the marker box is calculated and how to modify the formula if needed.

---

## Overview

When a manual marker is placed on a BTW graph, a **bearing rate box** appears next to it displaying a value with an R/L prefix. This value represents the bearing rate derived from the marker's rotation angle.

```
Example display: "R45" (rotating right 45°), "L30" (rotating left 30°), "0" (no rotation)
```

---

## Current Formula

### Display Value Calculation

The bearing rate box value is calculated in `BTWInteractiveOverlay::updateBearingRateBox()`:

```cpp
// Location: btwinteractiveoverlay.cpp, updateBearingRateBox() function

// Step 1: Get the marker's rotation angle (in degrees)
qreal rawRotation = marker->rotation();

// Step 2: Normalize rotation to 0-359 degrees (one full rotation = 360 values: 0 to 359)
qreal normalizedRotation = rawRotation;
while (normalizedRotation < 0) {
    normalizedRotation += 360.0;
}
while (normalizedRotation >= 360.0) {
    normalizedRotation -= 360.0;
}

// Step 3: Determine prefix based on rotation direction:
// Clockwise = increasing (0→359), values 0-179 = "R"
// Anti-clockwise = decreasing (359→0), values 180-359 = "L"
// 0 degrees = no prefix
QString prefix = "";
if (normalizedRotation > 0 && normalizedRotation < 180) {
    prefix = "R";  // Clockwise (increasing 0→359)
} else if (normalizedRotation >= 180 && normalizedRotation < 360) {
    prefix = "L";  // Anti-clockwise (decreasing 359→0)
}

// Step 4: Format the display value (normalized rotation, no decimal places)
QString displayValue = QString::number(normalizedRotation, 'f', 0);
QString bearingRateText = prefix + displayValue;
```

### Current Formula Summary

| Component | Formula | Description |
|-----------|---------|-------------|
| **Raw Value** | `marker->rotation()` | Qt rotation angle in degrees (can be any value) |
| **Normalized Value** | `normalize(rotation)` to 0-359 | Normalized to one full rotation (0-359, 360 values) |
| **Display Value** | `normalizedRotation` | Normalized value, no decimals |
| **Prefix** | `0-179° → "R"`, `180-359° → "L"`, `0° → ""` | Direction indicator: R=Clockwise (increasing), L=Anti-clockwise (decreasing) |

---

## Sync Data Formula

When marker data is synchronized across containers, a **scaling factor of 10.0** is applied:

```cpp
// Extracting bearing rate for sync (btwinteractiveoverlay.cpp)
data.bearingRate = marker->rotation() / 10.0;

// Applying bearing rate from sync (btwinteractiveoverlay.cpp)
marker->setRotation(markerData.bearingRate * 10.0);
```

This means:
- **Internal rotation**: 450° represents a sync bearing rate of 45.0
- **Sync data**: bearingRate of 45.0 represents 450° internal rotation

---

## How to Modify the Formula

### Option 1: Change Display Format Only

To modify how the bearing rate is displayed without affecting the underlying calculation, edit the `updateBearingRateBox()` function in `btwinteractiveoverlay.cpp`:

```cpp
void BTWInteractiveOverlay::updateBearingRateBox(InteractiveGraphicsItem *marker)
{
    // ... existing code ...
    
    qreal bearingRate = marker->rotation();
    
    // ========== MODIFY HERE FOR CUSTOM DISPLAY ==========
    
    // Example 1: Apply a scaling factor to display value
    // qreal displayRate = bearingRate / 10.0;  // Show 1/10th of rotation
    
    // Example 2: Apply a conversion factor (e.g., degrees to milliradians)
    // qreal displayRate = bearingRate * 17.453;  // deg to mrad
    
    // Example 3: Custom formula
    // qreal displayRate = yourCustomFormula(bearingRate);
    
    // Then use displayRate instead of bearingRate for formatting:
    // QString prefix = (0 == displayRate) ? "" : (displayRate >= 0) ? "R" : "L";
    // QString displayValue = QString::number(qAbs(displayRate), 'f', 1);  // 1 decimal
    
    // ========== END MODIFICATION AREA ==========
    
    // ... rest of function ...
}
```

### Option 2: Change the Underlying Rotation-to-BearingRate Mapping

If you need to change how rotation maps to bearing rate throughout the system, modify these locations:

#### 2.1 When Extracting Data for Sync
**File:** `btwinteractiveoverlay.cpp`
**Functions:** `getMarkerData()`, `addDataPointMarker()`

```cpp
// In getMarkerData() - around line 964
data.bearingRate = marker->rotation() / 10.0;  // Change divisor or formula

// In addDataPointMarker() - around line 132
markerData.bearingRate = marker->rotation() / 10.0;  // Change divisor or formula
```

#### 2.2 When Applying Data from Sync
**File:** `btwinteractiveoverlay.cpp`
**Functions:** `createMarkerFromData()`, `updateMarkerFromData()`

```cpp
// In createMarkerFromData() - around line 886
marker->setRotation(markerData.bearingRate * 10.0);  // Change multiplier

// In updateMarkerFromData() - around line 989
marker->setRotation(markerData.bearingRate * 10.0);  // Change multiplier
```

### Option 3: Add a Configurable Conversion Factor

For maximum flexibility, you can add a member variable for the conversion factor:

```cpp
// In btwinteractiveoverlay.h, add to private members:
qreal m_bearingRateConversionFactor = 10.0;

// Add public setter:
void setBearingRateConversionFactor(qreal factor) { 
    m_bearingRateConversionFactor = factor; 
}

// Then use m_bearingRateConversionFactor instead of hardcoded 10.0
```

---

## File Locations Reference

| File | Purpose |
|------|---------|
| `btwinteractiveoverlay.cpp` | Main bearing rate calculation and display logic |
| `btwinteractiveoverlay.h` | Class definition and member variables |
| `interactivegraphicsitem.cpp` | Rotation handling (mouse drag to rotate) |
| `sharedsyncstate.h` | `BTWSyncMarkerData` structure definition |

### Key Functions

| Function | Location | Purpose |
|----------|----------|---------|
| `updateBearingRateBox()` | btwinteractiveoverlay.cpp:500 | Calculates and displays the bearing rate box |
| `getMarkerData()` | btwinteractiveoverlay.cpp:948 | Extracts marker data for synchronization |
| `createMarkerFromData()` | btwinteractiveoverlay.cpp:823 | Creates marker from sync data |
| `updateMarkerFromData()` | btwinteractiveoverlay.cpp:977 | Updates marker from sync data |

---

## Example: Custom Formula Implementation

Here's a complete example of implementing a custom bearing rate formula:

```cpp
// Custom formula: Convert rotation to bearing rate in degrees/minute
// Assume: rotation angle represents rate scaled by time constant

void BTWInteractiveOverlay::updateBearingRateBox(InteractiveGraphicsItem *marker)
{
    if (!marker || !m_overlayScene) {
        return;
    }
    
    QPointF markerPos = marker->scenePos();
    qreal markerRadius = 10.0;
    
    // Custom conversion: rotation to bearing rate
    qreal rotationDegrees = marker->rotation();
    
    // YOUR CUSTOM FORMULA HERE:
    // Example: bearingRate = rotation * scaleFactor + offset
    qreal bearingRateDegreesPerMinute = rotationDegrees * 0.5;  // Custom scaling
    
    // Format with custom precision (e.g., 1 decimal place)
    QString prefix = (bearingRateDegreesPerMinute == 0) ? "" 
                   : (bearingRateDegreesPerMinute > 0) ? "R" : "L";
    QString displayValue = QString::number(qAbs(bearingRateDegreesPerMinute), 'f', 1);
    QString bearingRateText = prefix + displayValue;
    
    // ... rest of display code unchanged ...
}
```

---

## Notes

1. **Rotation Range**: One full rotation = 360 values (0 to 359 degrees). Qt's `rotation()` function can return any value, which is normalized to 0-359.

2. **Rotation Direction**:
   - **Clockwise** = increasing rotation (0→359 degrees)
   - **Anti-clockwise** = decreasing rotation (359→0 degrees)

3. **L/R Assignment**:
   - **Clockwise rotation** (increasing 0→359): Values 0-179 degrees → "R" prefix
   - **Anti-clockwise rotation** (decreasing 359→0): Values 180-359 degrees → "L" prefix
   - **0 degrees** = no prefix (no rotation)

4. **Normalization**: The rotation is normalized to 0-359 degrees using modulo arithmetic:
   - Values < 0: Add 360 until in range
   - Values ≥ 360: Subtract 360 until in range

5. **Sync Consistency**: If you change the conversion factor, ensure the same factor is used in both extraction (`/ factor`) and application (`* factor`) to maintain sync consistency.

6. **Units**: The current implementation treats the rotation angle directly as the bearing rate value. If your application requires specific units (e.g., degrees/hour, mrad/s), apply the appropriate conversion factor.

7. **Examples**:
   - Rotation = 45° → Display: "R45" (clockwise, increasing)
   - Rotation = 200° → Display: "L200" (anti-clockwise, decreasing)
   - Rotation = 0° → Display: "0" (no prefix)
   - Rotation = -30° → Normalized to 330° → Display: "L330" (anti-clockwise, decreasing)
   - Rotation = 450° → Normalized to 90° → Display: "R90" (clockwise, increasing)

