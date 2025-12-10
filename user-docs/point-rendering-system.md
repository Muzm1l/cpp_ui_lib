# Point Rendering System User Manual

## Overview

The Point Rendering System is an optimized rendering mechanism for scatterplot data points in the WaterfallGraph component. It uses a **cached pixmap approach** to efficiently render thousands of data points while maintaining visual clarity and preventing color darkening when many points overlap.

### Key Features

- **High Performance**: Efficient rendering of 1000+ points with minimal overhead
- **Color Preservation**: Prevents color darkening when many points overlap (e.g., 1000 points with <0.2 x difference)
- **Pre-cached Default Colors**: Cyan, Red, Green, and Yellow are pre-created for optimal performance
- **Smallest Possible Size**: Points are rendered as 1x1 pixel rectangles by default
- **Automatic Caching**: Point pixmaps are automatically cached by color and size

---

## Table of Contents

1. [How It Works](#how-it-works)
2. [Default Colors](#default-colors)
3. [Performance Benefits](#performance-benefits)
4. [Usage Guide](#usage-guide)
5. [Technical Details](#technical-details)
6. [Troubleshooting](#troubleshooting)

---

## How It Works

### Architecture

The system uses a **two-stage caching mechanism**:

1. **Pixmap Cache**: A `QMap` stores pre-rendered pixmaps keyed by color and size
2. **Point Rendering**: Each data point uses a lightweight `QGraphicsPixmapItem` that references the cached pixmap

### Rendering Process

```
Data Point → Get/Create Pixmap (by color + size) → Create QGraphicsPixmapItem → Add to Scene
```

1. When a point needs to be drawn, the system checks if a pixmap for that color/size combination exists
2. If cached: Reuses the existing pixmap
3. If not cached: Creates a new pixmap, caches it, then uses it
4. Each point is rendered as a lightweight pixmap item referencing the shared pixmap

### Point Shape

- **Shape**: Rectangle (1x1 pixel by default)
- **Rendering**: Uses `QPainter::CompositionMode_Source` to prevent color blending
- **Antialiasing**: Disabled for crisp, pixel-perfect rectangles

---

## Default Colors

The system pre-creates four mandatory default colors at initialization:

| Color | Qt Color | RGB Values | Use Case |
|-------|----------|------------|----------|
| **Cyan** | `Qt::cyan` | (0, 255, 255) | Standard data series |
| **Red** | `Qt::red` | (255, 0, 0) | Alert/warning data |
| **Green** | `Qt::green` | (0, 255, 0) | Normal/positive data |
| **Yellow** | `Qt::yellow` | (255, 255, 0) | Caution/advisory data |

These colors are created once during `WaterfallGraph` construction, ensuring zero overhead when rendering points with these colors.

### Custom Colors

You can use any color - the system will automatically create and cache pixmaps for custom colors as needed:

```cpp
// Custom colors are automatically cached on first use
graph->drawScatterplot("MySeries", QColor(128, 64, 200), 1.0, Qt::black);
```

---

## Performance Benefits

### Before (Old System)

- **1000 points** = 1000 separate `QGraphicsEllipseItem` objects
- Each item had its own rendering overhead
- Color blending caused darkening when points overlapped
- Memory: ~1000 item objects

### After (New System)

- **1000 points (same color)** = 1 pixmap creation + 1000 lightweight references
- Shared pixmap rendering (minimal overhead)
- No color darkening (uses `CompositionMode_Source`)
- Memory: 1 pixmap + 1000 lightweight items

### Performance Metrics

| Scenario | Pixmap Creations | Memory Usage | Rendering Speed |
|----------|-----------------|--------------|-----------------|
| 1000 cyan points | 1 | Minimal | Fast |
| 1000 green points | 1 | Minimal | Fast |
| 1000 mixed (4 colors) | 4 | Minimal | Fast |
| 1000 unique colors | 1000 | Higher | Slower (but still cached) |

**Best Practice**: Use the pre-cached default colors (cyan, red, green, yellow) when possible for optimal performance.

---

## Usage Guide

### Basic Usage

The point rendering system is automatically used when calling `drawScatterplot()`:

```cpp
// Default usage (1x1 pixel, white color)
graph->drawScatterplot("MySeries");

// Specify color and size
graph->drawScatterplot("MySeries", Qt::green, 1.0, Qt::black);

// Use pre-cached default colors (optimal performance)
graph->drawScatterplot("CyanSeries", Qt::cyan, 1.0, Qt::black);
graph->drawScatterplot("RedSeries", Qt::red, 1.0, Qt::black);
graph->drawScatterplot("GreenSeries", Qt::green, 1.0, Qt::black);
graph->drawScatterplot("YellowSeries", Qt::yellow, 1.0, Qt::black);
```

### Setting Series Colors

You can set custom colors for data series:

```cpp
// Set series color
graph->setSeriesColor("MySeries", Qt::cyan);

// Draw with the series color
graph->drawScatterplot("MySeries", graph->getSeriesColor("MySeries"), 1.0, Qt::black);
```

### Point Size

The default point size is **1.0** (1x1 pixel rectangle). You can specify a different size:

```cpp
// 2x2 pixel rectangle
graph->drawScatterplot("MySeries", Qt::green, 2.0, Qt::black);

// 3x3 pixel rectangle
graph->drawScatterplot("MySeries", Qt::green, 3.0, Qt::black);
```

**Note**: Larger sizes will create separate cached pixmaps. For best performance with many points, use size 1.0.

---

## Technical Details

### Cache Key Format

Pixmaps are cached using a string key format:
```
"r_g_b_a_size"
```

Example:
- Cyan, size 1.0: `"0_255_255_255_1"`
- Red, size 1.0: `"255_0_0_255_1"`
- Green, size 2.0: `"0_255_0_255_2"`

### Cache Methods

#### `getPointPixmapKey(color, size)`
Generates a unique cache key for a color/size combination.

#### `getPointPixmap(color, size)`
- Checks cache for existing pixmap
- Creates new pixmap if not found
- Caches the pixmap for future use
- Returns the pixmap

### Rendering Properties

- **Composition Mode**: `QPainter::CompositionMode_Source`
  - Prevents color blending when points overlap
  - Ensures each point renders with its exact color
  
- **Antialiasing**: Disabled
  - Provides crisp, pixel-perfect rectangles
  - Better performance for small points

- **Z-Value**: 120
  - Points are drawn above data lines but below markers

### Memory Management

- Pixmaps are cached in `pointPixmapCache` (QMap)
- Cache persists for the lifetime of the `WaterfallGraph` instance
- Cache is cleared when the graph is destroyed
- Typical memory per pixmap: ~4-16 bytes (1x1 to 3x3 pixels)

---

## Troubleshooting

### Issue: Points appear too dark when many overlap

**Solution**: This is handled automatically. The system uses `CompositionMode_Source` to prevent color blending. If you still see darkening, ensure:
- Points are using the cached pixmap system (via `drawScatterplot()`)
- Not using old `QGraphicsEllipseItem` directly

### Issue: Performance is slow with many points

**Solutions**:
1. Use pre-cached default colors (cyan, red, green, yellow)
2. Use point size 1.0 (smallest possible)
3. Ensure points share the same color/size to maximize cache reuse

### Issue: Points are not visible

**Check**:
- Point size is at least 1.0
- Color is not transparent (alpha = 255)
- Points are within the visible time range
- Z-value is appropriate (default: 120)

### Issue: Custom colors not working

**Solution**: Custom colors are automatically cached on first use. The first point with a new color will create the pixmap, subsequent points will reuse it.

---

## Best Practices

1. **Use Default Colors**: Prefer cyan, red, green, yellow for best performance
2. **Minimal Size**: Use 1.0 pixel size for maximum efficiency
3. **Color Consistency**: Use the same color for all points in a series
4. **Batch Rendering**: Draw all points of the same color/size together
5. **Cache Awareness**: Be aware that unique color/size combinations create new cache entries

---

## API Reference

### `drawScatterplot(seriesLabel, pointColor, pointSize, outlineColor)`

Draws scatterplot points for a data series.

**Parameters**:
- `seriesLabel` (QString): Name of the data series
- `pointColor` (QColor, default: Qt::white): Color of the points
- `pointSize` (qreal, default: 1.0): Size of points in pixels (1.0 = 1x1 rectangle)
- `outlineColor` (QColor, default: Qt::black): Unused for rectangles

**Example**:
```cpp
graph->drawScatterplot("MySeries", Qt::green, 1.0, Qt::black);
```

### `getPointPixmap(color, size)`

Gets or creates a cached pixmap for a point.

**Parameters**:
- `color` (QColor): Point color
- `size` (qreal): Point size

**Returns**: `QPixmap` - Cached pixmap for the color/size combination

**Note**: This is called automatically by `drawScatterplot()`. You typically don't need to call this directly.

---

## Related Documentation

- [WaterfallGraph API](../docs/waterfallgraph.md) - Main graph component documentation
- [SCWWindow Guide](./scwwindow.md) - Window management and data series
- [Overlay System](../docs/OVERLAY_SYSTEM_ANALYSIS.md) - Interactive overlay architecture

---

## Version History

- **v1.0** (Current): Initial implementation with cached pixmap system
  - Pre-cached default colors (cyan, red, green, yellow)
  - 1x1 pixel rectangle points
  - CompositionMode_Source for color preservation
  - Automatic caching by color/size

---

## Support

For issues or questions about the point rendering system:
1. Check this documentation
2. Review the code in `waterfallgraph.cpp` (methods: `getPointPixmap()`, `drawScatterplot()`)
3. Check debug output for cache hits/misses
4. Verify point colors and sizes are consistent within series

