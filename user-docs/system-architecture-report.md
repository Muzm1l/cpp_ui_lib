# System Architecture Report

## Overview
This is a Qt-based C++ application for visualizing multiple types of waterfall graphs (BDW, BRW, BTW, FDW, FTW, LTW, RTW) with synchronized time-based navigation and interactive features. The system is designed for real-time data visualization with multi-container synchronization capabilities.

## Architecture Layers

### 1. Application Layer
- **MainWindow** (`mainwindow.h/cpp`): Top-level application window
  - Manages application lifecycle and UI tabs
  - Coordinates multiple graph layouts and views
  - Handles simulation data generation (optional)
  - Manages timers for time updates and simulation

### 2. Layout Management Layer
- **GraphLayout** (`graphlayout.h/cpp`): Container orchestration
  - Manages multiple `GraphContainer` instances in various layouts (1x1, 2x2, 2x1, 4x1, etc.)
  - Centralized data source management
  - Synchronization hub for all containers
  - Manages manoeuvres, markers, and symbols across all graphs
  - Provides unified API for graph operations

### 3. Container Layer
- **GraphContainer** (`graphcontainer.h/cpp`): Individual graph container
  - Wraps a `WaterfallGraph` with UI controls
  - Manages `TimelineView` for time navigation
  - Handles `ZoomPanel` for range controls
  - Supports multiple data options (graph types) via combo box
  - Synchronizes with other containers via `GraphContainerSyncState`

### 4. Visualization Layer
- **WaterfallGraph** (`waterfallgraph.h/cpp`): Base graph widget
  - Renders time-series waterfall data
  - Handles mouse interactions and selections
  - Manages graphics items (markers, symbols, lines)
  - Supports multiple data series with different colors

- **Specialized Graph Types**:
  - `BDWGraph`, `BRWGraph`, `BTWGraph`, `FDWGraph`, `FTWGraph`, `LTWGraph`, `RTWGraph`
  - Each extends `WaterfallGraph` with type-specific features
  - BTW graphs support interactive overlays, horizontal lines, and shaded regions
  - RTW graphs support symbol drawing

### 5. Data Layer
- **WaterfallData** (`waterfalldata.h/cpp`): Data storage and management
  - Stores time-series data as vectors of (value, timestamp) pairs
  - Supports multiple series per data source
  - Provides query methods for time/range filtering
  - Handles data point addition and bulk updates

### 6. Synchronization Layer
- **GraphContainerSyncState** (`sharedsyncstate.h`): Centralized sync state
  - Time interval synchronization
  - Time scope (selection) synchronization
  - Cursor time synchronization
  - BTW marker synchronization
  - Shaded region synchronization
  - Manoeuvre synchronization
  - Follow mode synchronization

## Key Design Patterns

### 1. Hub-and-Spoke Synchronization
- `GraphLayout` acts as the central hub
- All `GraphContainer` instances connect to the hub
- Changes in one container propagate to all others via signals/slots
- Prevents infinite loops by skipping source container in propagation

### 2. Strategy Pattern for Graph Types
- Base `WaterfallGraph` class with type-specific implementations
- Graph type determined at runtime via `GraphType` enum
- Each graph type can have specialized rendering and interaction

### 3. Observer Pattern
- Qt signals/slots for event propagation
- Containers observe sync state changes
- Timeline views observe time interval changes

### 4. Factory Pattern
- `GraphContainer` creates appropriate graph type via `createWaterfallGraph()`
- Graph creation based on `GraphType` enum

## Component Relationships

```
MainWindow
├── GraphLayout (1 or more)
│   ├── GraphContainer (1-4 depending on layout)
│   │   ├── WaterfallGraph (specialized: BDW/BRW/BTW/etc.)
│   │   ├── TimelineView
│   │   ├── ZoomPanel
│   │   └── TimeSelectionVisualizer
│   └── GraphContainerSyncState (shared)
├── Simulator (optional)
└── SCWWindow (optional)
```

## Data Flow

1. **Data Input**: Data points added via `GraphLayout::addDataPointToDataSource()`
2. **Data Storage**: Stored in `WaterfallData` objects keyed by `GraphType`
3. **Data Distribution**: Each `GraphContainer` subscribes to data sources
4. **Rendering**: `WaterfallGraph` reads from `WaterfallData` and renders
5. **Synchronization**: User interactions trigger signals that update `GraphContainerSyncState` and propagate to all containers

## Key Features

### Time Management
- Multiple time intervals (15min, 30min, 1hr, 2hr, 4hr, 8hr)
- Time selection spans for zooming
- Follow mode (auto-scroll) vs frozen mode
- Absolute vs relative time display

### Interactive Features
- Mouse selection for time ranges
- Interactive markers (BTW, RTW)
- Symbol drawing (RTW symbols)
- Horizontal line drawing (BTW)
- Shaded regions (BTW)
- Manoeuvre overlays

### Synchronization Features
- All containers share same time interval
- Time selections propagate across containers
- Cursor position synchronized
- Markers and symbols synchronized
- Shaded regions synchronized across BTW graphs

## Build System
- Qt Project file (`ui-sandbox.pro`)
- Uses Qt5/6 with C++11
- MOC (Meta-Object Compiler) for Qt signals/slots
- OpenGL support for macOS

## Dependencies
- Qt Core, GUI, Widgets
- Qt Graphics View Framework
- Standard C++ libraries (STL)

## Performance Considerations
- Incremental data updates supported
- Batch data operations available
- Redraw methods for selective updates
- Timer-based updates for real-time data

## Extension Points
- New graph types can be added by extending `WaterfallGraph`
- New layout types via `LayoutType` enum
- Custom synchronization via `GraphContainerSyncState`
- Additional data series via `WaterfallData` series management

