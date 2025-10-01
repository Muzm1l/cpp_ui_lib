# ZoomPanel Label Display Fix

## Problem
The ZoomPanel's `createTextItems()` method was creating text items with hardcoded values instead of using the stored label values, causing incorrect initial display of labels.

## Root Cause
In `zoompanel.cpp`, the `createTextItems()` method was creating QGraphicsTextItem objects with hardcoded strings:

```cpp
// Before (hardcoded values):
m_leftText = new QGraphicsTextItem("0.00");
m_centerText = new QGraphicsTextItem("0.50");
m_rightText = new QGraphicsTextItem("1.00");
```

This meant that when the ZoomPanel was first created, it would always display "0.00", "0.50", and "1.00" regardless of the actual stored label values.

## Solution
Modified the text item creation to use the stored label values from the ZoomPanel's member variables:

```cpp
// After (using stored values):
m_leftText = new QGraphicsTextItem(QString::number(leftLabelValue, 'f', 2));
m_centerText = new QGraphicsTextItem(QString::number(centerLabelValue, 'f', 2));
m_rightText = new QGraphicsTextItem(QString::number(rightLabelValue, 'f', 2));
```
