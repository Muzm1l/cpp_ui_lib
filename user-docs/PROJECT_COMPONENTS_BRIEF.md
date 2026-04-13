# Project Components Brief

This document provides a concise overview of the main components in `cpp_ui_lib` and how they fit together.

---

## 1) Application Entry and Shell

- `main.cpp`
  - Application entry point.
  - Creates and starts the Qt app and main window.

- `mainwindow.h` / `mainwindow.cpp`
  - Top-level UI shell (`QMainWindow`).
  - Creates/wires core layout, timers, and optional simulator/test flows.
  - Owns high-level tabs/views and app-wide actions.

---

## 2) Layout and Orchestration

- `graphlayout.h` / `graphlayout.cpp`
  - Central orchestration layer.
  - Manages multiple `GraphContainer` instances and layout modes.
  - Owns graph engines/data sources per graph type.
  - Handles cross-container synchronization and time-scope propagation.

- `sharedsyncstate.h` (and related sync helpers)
  - Shared state object used to synchronize timeline, cursor, and selection behavior across containers.

---

## 3) Data Engine and Model

- `graphengine.h` / `graphengine.cpp`
  - Runtime data engine for a graph type.
  - Accepts appended data and emits update/range/symbol/marker signals.
  - Bridges producers (simulator/live input) to views.

- `waterfalldata.h` / `waterfalldata.cpp`
  - Core data model for time-series values.
  - Stores Y values, timestamps, and metadata/symbol/marker streams.
  - Provides APIs for full data access and visible-range extraction.

- `circularbuffer.h`
  - Generic fixed-capacity (or unlimited) circular container.
  - Used heavily for rolling data history storage.

---

## 4) Container and Visualization Widgets

- `graphcontainer.h` / `graphcontainer.cpp`
  - Per-panel container for one active graph view + controls.
  - Connects timeline, zoom controls, and selected graph type.
  - Routes time-scope/data changes to active rendering widgets.

- `waterfallgraph.h` / `waterfallgraph.cpp`
  - Main rendering widget for waterfall/time-series drawing.
  - Handles visible data caching, incremental/full redraw logic, and scene updates.
  - Base class for graph-type-specific renderers.

- Specialized graph classes (`btwgraph.cpp`, `rtwgraph.cpp`, `ltwgraph.cpp`, etc.)
  - Graph-type-specific drawing logic and symbol behavior.
  - Extend or customize `WaterfallGraph` for domain-specific visuals.

---

## 5) Timeline and Interaction

- `timelineview.h` / `timelineview.cpp`
  - Timeline interaction and visible time-window control.
  - Emits scope-change events that drive graph updates.
  - Supports drag/follow/frozen behaviors and time labels.

- Related helpers (`timeselectionvisualizer.*`, timeline drawing helpers)
  - Visual and interaction support for selection windows and timeline markers.

---

## 6) Simulation and Test Data Paths

- `simulator.h` / `simulator.cpp`
  - Generates synthetic data for graph types.
  - Pushes data periodically through layout/engine interfaces.
  - Useful for stress/performance and UI behavior testing.

---

## 7) Optional/Secondary Views

- `tacticalsolutionview.h` / `tacticalsolutionview.cpp`
  - Additional graphics-based tactical display.
  - Separate from the main waterfall pipeline; may be enabled/disabled by UI flow.

- SCW-related modules (`scwwindow.*`, related docs)
  - Secondary workflow/view subsystem for scenario-specific UI.

---

## 8) Typical Data Flow

1. Data source (simulator/live) appends values.
2. `GraphEngine` updates `WaterfallData` and emits change signals.
3. `GraphLayout`/`GraphContainer` propagate updates and scope changes.
4. Active `WaterfallGraph` refreshes visible cache and repaints.
5. `TimelineView` interactions change visible time window and trigger redraw.

---

## 9) Quick Mental Model

- `MainWindow` = app shell  
- `GraphLayout` = coordinator  
- `GraphEngine` + `WaterfallData` = data core  
- `GraphContainer` = panel-level UI wrapper  
- `WaterfallGraph` (+ subclasses) = renderer  
- `TimelineView` = time navigation controller  

