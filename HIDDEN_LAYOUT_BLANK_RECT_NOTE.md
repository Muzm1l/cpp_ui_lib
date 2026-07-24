# Blank rectangle on `LayoutType::HIDDEN` — analysis note

## What was fixed in this repo (`cpp_ui_lib`)

`GraphLayout::setLayoutType()` in `graphlayout.cpp` now resets size constraints
that were previously left stale when switching to `HIDDEN`:

1. **On the `GraphLayout` widget itself** (before the `switch`, ~line 122):
   ```cpp
   setMinimumSize(0, 0);
   setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
   ```
   Runs unconditionally on every mode change.

2. **On each container** in the reset loop (~line 248):
   ```cpp
   container->setMinimumSize(0, 0);
   container->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
   container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
   ```

### Why this was needed
- `updateLayoutSizing()` calls `setFixedWidth(totalWidth)` + `setFixedHeight(standardHeight)`
  on the widget (`graphlayout.cpp:742-743`) for every non-hidden mode.
- `GraphContainer::setContainerWidth/Height/Size()` call `setFixedSize(...)`
  (`graphcontainer.cpp:438, 452, 465`).
- `setFixed*` pins **both** the minimum and maximum size. On switch to `HIDDEN`
  the old code only reset `setMaximumWidth(QWIDGETSIZE_MAX)` — min width, min
  height, and max height stayed pinned → widget still demanded the last visible
  layout's size → blank rectangle.

## Why this may NOT fully fix the MAIN SYSTEM (TMA page)

The fix makes `GraphLayout` *willing* to shrink; something in the parent still has
to *make* it shrink. That logic lives in the TMA / `WFGraphChangeSlot` codebase,
which is **NOT present in this repo**. The following could NOT be verified here:

- `WFGraphChangeSlot`, `titleDisplay`, and the "spread rail" widget do not exist
  anywhere in `cpp_ui_lib`.
- In this repo, `graphgrid` is embedded via absolute `setGeometry(4, 4, 548, 900)`
  (`mainwindow.cpp:47`), not a layout/splitter. With absolute geometry, resetting
  the minimum does NOT shrink the widget on its own.

## Checklist to analyse / solve in the main system

1. **How is WFGraphView/GraphLayout embedded on the TMA page?**
   - `QLayout` → after hiding, call `parentLayout->invalidate(); parentLayout->activate();`
     to force an immediate re-layout (otherwise stale rect may persist a frame or longer).
   - `QSplitter` → the splitter caches sizes and will NOT reclaim space; call
     `splitter->refresh()` or recompute and `splitter->setSizes(...)`.
   - Manual `setGeometry` / corner widget → you must explicitly resize/hide the
     widget yourself; the min-size reset alone won't move it.

2. **Does the hide path call `setVisible(false)` on the whole widget, or only
   `setLayoutType(HIDDEN)` while the widget stays visible?**
   - If only the mode is set to HIDDEN, the widget stays in the layout and the
     parent must reclaim the space (see #1).

3. **Sibling sizing:** find any `titleDisplay->setFixedSize(...)` / spread-rail
   width set when GraphLayout is hidden/shown. These assume GraphLayout's old
   width and can leave a gap regardless of GraphLayout's internal state.

4. **After hiding**, confirm no other code re-applies a fixed size to the corner
   widget (e.g. a saved geometry restore).

## Files touched
- `graphlayout.cpp` — `setLayoutType()` size-constraint resets (tasks 2, 3, 5).

## Not done (out of scope / not in this repo)
- Task 4 (parent `QLayout`/`QSplitter` re-layout in `WFGraphChangeSlot`) — code
  not present here; apply in the TMA/main-window repo per the checklist above.
