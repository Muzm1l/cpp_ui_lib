# System Components Overview

This document lists all components in the C++ UI Library system.

## Main Application Components

### Core Application
- **MainWindow** - Main application window (QMainWindow)
  - Manages tabs and overall application structure
  - Coordinates GraphLayout and Simulator
  - Handles UI initialization

- **Simulator** - Data generation and simulation engine
  - Generates test data for all graph types
  - Updates GraphLayout with new data points
  - Configurable simulation parameters

## Layout & Container Components

### Layout Management
- **GraphLayout** - Main layout manager (QWidget)
  - Manages multiple GraphContainer instances
  - Supports multiple layout types (GPW1W, GPW4W, GPW2WV, GPW2WH, GPW4WH, NOGPW2WH, HIDDEN)
  - Handles data source distribution
  - Synchronizes timeline views across containers
  - Manages cursor synchronization

- **GraphContainer** - Container for graph components (QWidget)
  - Contains TimelineView, WaterfallGraph, ZoomPanel, TimeSelectionVisualizer
  - Manages graph type switching via ComboBox
  - Handles data options and graph lifecycle
  - Coordinates cursor time changes

## Graph Components

### Base Graph
- **WaterfallGraph** - Base waterfall graph class (QWidget)
  - Time-based data visualization
  - Crosshair and cursor functionality
  - Time axis cursor (shared cursor line)
  - Data point rendering
  - Grid and axis drawing
  - Mouse interaction (selection, crosshair)

### Specialized Graph Types
- **BDWGraph** - Bandwidth Display Waterfall (extends WaterfallGraph)
- **BRWGraph** - Bandwidth Range Waterfall (extends WaterfallGraph)
- **BTWGraph** - Bit Time Waterfall (extends WaterfallGraph)
  - Includes interactive overlay for markers
  - Scatterplot rendering
- **FDWGraph** - Frequency Display Waterfall (extends WaterfallGraph)
- **FTWGraph** - Frequency Time Waterfall (extends WaterfallGraph)
- **LTWGraph** - Line Time Waterfall (extends WaterfallGraph)
- **RTWGraph** - Range Time Waterfall (extends WaterfallGraph)

## Timeline Components

### Timeline View
- **TimelineView** - Timeline control widget (QWidget)
  - Interval selection buttons
  - Time mode toggle (Absolute/Relative)
  - Wraps TimelineVisualizerWidget

- **TimelineVisualizerWidget** - Timeline visualization widget (QWidget)
  - Displays time segments
  - Slider for time window selection
  - Crosshair timestamp label
  - Smooth animation and scrolling
  - 12-hour time range display

### Timeline Helpers
- **SliderGeometry** - Slider geometry calculations
  - Calculates slider dimensions and positions
  - Maps time windows to Y positions
  - Handles slider bounds

- **SliderState** - Slider state management
  - Manages slider position and time window
  - Handles drag operations
  - Synchronizes position and time window

### Timeline Drawing Objects
- **TimelineDrawingObject** - Base class for timeline drawing
- **TimelineSegmentDrawer** - Draws timeline segments with labels
- **TimelineChevronDrawer** - Draws chevron indicators
- **TimelineBackgroundDrawer** - Draws timeline background

## Time Selection Components

### Time Selection Visualizer
- **TimeSelectionVisualizer** - Time selection widget (QWidget)
  - Visual time selection interface
  - Clear button for selections
  - Multiple selection support (up to 5)

- **TimeVisualizerWidget** - Time visualization widget (QWidget)
  - Displays time selections
  - Mouse interaction for creating selections
  - Valid selection range management

## Control & UI Components

### Zoom Panel
- **ZoomPanel** - Zoom control widget (QWidget)
  - Y-axis range control
  - Min/max value indicators
  - Crosshair label display
  - User modification tracking

### Interactive Components
- **BTWInteractiveOverlay** - Interactive overlay for BTW graphs
  - Marker management
  - Interactive graphics items
  - Event handling

- **InteractiveGraphicsItem** - Interactive graphics item (QGraphicsItem)
  - Draggable, rotatable markers
  - Click and drag interactions
  - Customizable appearance

## Data Components

### Data Management
- **WaterfallData** - Data source for waterfall graphs
  - Time-series data storage
  - Multiple series support
  - Time and value range calculations
  - Data point management

- **TwoAxisData** - Two-axis data source
  - Dual-axis data management

- **RTWSymbolData** - RTW symbol data structure
  - Symbol-specific data format

## Utility Components

### Drawing Utilities
- **DrawUtils** - Drawing utility functions
  - Common drawing operations
  - Helper functions for graphics

### Navigation & Time Utilities
- **NavTimeUtils** - Navigation time utilities
  - Time conversion functions
  - Navigation-specific time handling

### Timeline Utilities
- **TimeInterval** - Enum for time intervals
  - OneMinute, FiveMinutes, FifteenMinutes, ThirtyMinutes
  - OneHour, TwoHours, ThreeHours, FourHours, SixHours, TwelveHours

- **TimeSelectionSpan** - Time range structure
  - startTime, endTime (QDateTime)

### Graph Type Utilities
- **GraphType** - Enum for graph types
  - BDW, BRW, BTW, FDW, FTW, LTW, RTW

### Layout Type Utilities
- **LayoutType** - Enum for layout types
  - GPW1W, GPW4W, GPW2WV, GPW2WH, GPW4WH, NOGPW2WH, HIDDEN

## Symbol & Drawing Components

### RTW Symbols
- **RTWSymbolDrawing** - RTW symbol rendering
  - Symbol drawing functionality
  - RTW-specific graphics

- **RTWSymbolsTestWidget** - Test widget for RTW symbols

## Other Graph Components

### Two-Axis Graph
- **TwoAxisGraph** - Two-axis graph widget (QWidget)
  - Dual-axis visualization

### Tactical Solution View
- **TacticalSolutionView** - Tactical solution visualization (QGraphicsView)
  - Graphics view for tactical displays

## Example & Test Components

- **WaterfallGraphExample** - Example implementation
- **InteractiveGraphicsItemExample** - Example for interactive items
- **RTWSymbolsTestWidget** - Test widget for symbols

## Data Structures

### Configuration Structures
- **SimulatorConfig** - Simulator configuration
  - minValue, maxValue, startValue, deltaValue

- **ZoomBounds** - Zoom panel bounds
  - min, max values

- **SymbolInfo** - Symbol information structure

## Component Relationships

### Hierarchy
```
MainWindow
├── GraphLayout
│   ├── GraphContainer (multiple instances)
│   │   ├── TimelineView
│   │   │   └── TimelineVisualizerWidget
│   │   │       ├── SliderState
│   │   │       └── Timeline Drawing Objects
│   │   ├── WaterfallGraph (one of: BDW, BRW, BTW, FDW, FTW, LTW, RTW)
│   │   │   └── BTWInteractiveOverlay (BTW only)
│   │   │       └── InteractiveGraphicsItem (multiple)
│   │   ├── ZoomPanel
│   │   └── TimeSelectionVisualizer
│   │       └── TimeVisualizerWidget
│   └── WaterfallData (multiple instances, one per GraphType)
└── Simulator
```

### Data Flow
1. **Simulator** generates data → **WaterfallData**
2. **WaterfallData** → **GraphLayout** → **GraphContainer** → **WaterfallGraph**
3. **TimelineView** changes → **GraphContainer** → **WaterfallGraph** (time range update)
4. **WaterfallGraph** cursor → **GraphContainer** → **GraphLayout** → All containers (cursor sync)
5. **TimelineView** slider → **GraphContainer** → **WaterfallGraph** (scope sync)

## Key Features

### Synchronization
- **Timeline View Sync**: All timeline views sync interval and scope changes
- **Cursor Sync**: Shared cursor time synchronized across all graphs
- **Time Scope Sync**: Slider position syncs across timeline views and graphs

### Layout Types
- **GPW1W**: 1 window with timeline view
- **GPW4W**: 4 windows (2x2), 2 timeline views
- **GPW2WV**: 2 windows vertical, 2 timeline views
- **GPW2WH**: 2 windows horizontal, 1 timeline view
- **GPW4WH**: 4 windows horizontal, 1 timeline view
- **NOGPW2WH**: 2 windows horizontal (full screen), 1 timeline view
- **HIDDEN**: All containers hidden

### Graph Types
- **BDW**: Bandwidth Display Waterfall
- **BRW**: Bandwidth Range Waterfall
- **BTW**: Bit Time Waterfall (with interactive markers)
- **FDW**: Frequency Display Waterfall
- **FTW**: Frequency Time Waterfall
- **LTW**: Line Time Waterfall
- **RTW**: Range Time Waterfall

