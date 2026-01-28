# Frozen Mode Not Staying - Bug Analysis

## Problem
The TimelineView slider is not staying in FROZEN_MODE after being set to frozen state.

## Root Cause Analysis

### Bug #3: External Sync Resetting Frozen Slider Position (PRIMARY BUG)

**Location**: `timelineview.cpp` lines 1354-1374, `graphcontainer.cpp` line 1331

**Problem**: When `GraphContainer::setTimeScope()` is called by GraphLayout to sync time scopes across containers, it calls `TimelineView::setTimeWindowSilent()`, which updates the slider position even when in FROZEN_MODE.

**Flow**:
1. User drags slider → sets to FROZEN_MODE
2. GraphLayout syncs time scopes across containers
3. `GraphContainer::setTimeScope()` calls `m_timelineView->setTimeWindowSilent(selection)`
4. `setTimeWindowSilent()` calls `m_sliderState.setTimeWindow()`
5. `setTimeWindow()` calls `syncPositionFromTimeWindow()`
6. Slider position is recalculated based on synced time window
7. If synced window represents "now", slider moves to Y=0 (top)
8. This can trigger snap-to-top logic or just reset the frozen position

**Impact**: This is the PRIMARY bug causing frozen mode to not persist. Every time GraphLayout syncs time scopes (which happens when other containers change their time windows), the frozen slider position gets reset.

**Fix**: In `setTimeWindowSilent()`, check if mode is FROZEN_MODE and return early without updating the slider position or time window.

### Bug #1: Incorrect Mode Transition Logic in `setTimelineViewMode()`

**Location**: `timelineview.cpp` lines 2029-2048

**Problem**:
```cpp
void TimelineView::setTimelineViewMode(TimelineViewMode mode)
{
    m_timelineViewMode = mode;
    
    // Update the visualizer widget's mode
    if (m_visualizerWidget)
    {
        m_visualizerWidget->setTimelineViewMode(mode);
    }

    // This is the standard location where the mode transition logic is handled
    // Case 1 : FOLLOW_MODE -> FROZEN_MODE

    handleModeTransitionLogic(TimelineViewMode::FROZEN_MODE);  // ❌ BUG: Always hardcoded!

    // Case 2 : FROZEN_MODE -> FOLLOW_MODE

    handleModeTransitionLogic(mode);  // ✅ This is correct
    
}
```

**Issue**: Line 2042 always calls `handleModeTransitionLogic(TimelineViewMode::FROZEN_MODE)` regardless of the `mode` parameter. This means:
- When setting to FROZEN_MODE: calls transition logic twice (once with hardcoded FROZEN_MODE, once with parameter)
- When setting to FOLLOW_MODE: calls transition logic with FROZEN_MODE first, then FOLLOW_MODE (incorrect order)

**Impact**: While `handleModeTransitionLogic()` is currently empty (TODO), this is still incorrect logic that could cause issues if the function is implemented.

### Bug #2: Missing Sync State Update in `setTimelineViewMode()`

**Location**: `timelineview.cpp` lines 2029-2048

**Problem**: The `setTimelineViewMode()` method does NOT update the sync state, unlike `onTimelineViewModeChanged()` which does:

```cpp
// In onTimelineViewModeChanged() - line 1997-2000
if (m_syncState)
{
    m_syncState->isGraphContainerInFollowMode = (mode == TimelineViewMode::FOLLOW_MODE);
}
```

**Impact**: If `setTimelineViewMode()` is called directly (not via signal), the sync state becomes out of sync. This could cause other containers to have incorrect state, potentially leading to mode resets.

### Potential Issue #3: Timer Restart in `onVisibleTimeWindowChanged()`

**Location**: `timelineview.cpp` lines 1917-1937

**Code**:
```cpp
void TimelineView::onVisibleTimeWindowChanged(const TimeSelectionSpan& selection)
{
    // ... update sync state and emit signal ...
    
    //---- syed ------------------------------rebase conflict
    // Ensure timer is running after window changes (e.g., after slider drag)
    // This is critical to resume animation after user interactions
    ensureTimerRunning();  // ⚠️ Always restarts timer
}
```

**Potential Issue**: This always ensures the timer is running, even when in FROZEN_MODE. However, this shouldn't directly cause mode changes since the timer tick logic respects the mode (see `setCurrentTime()` lines 383-416).

## Code Flow Analysis

### Normal Flow (Working Correctly)

1. **User drags slider** → `mouseReleaseEvent()` (line 1232)
2. **Mode set to FROZEN_MODE** → `m_timelineViewMode = FROZEN_MODE` (line 1264)
3. **Signal emitted** → `emit timelineViewModeChanged(FROZEN_MODE)` (line 1272)
4. **Signal handler** → `onTimelineViewModeChanged(FROZEN_MODE)` (line 1990)
5. **Sync state updated** → `m_syncState->isGraphContainerInFollowMode = false` (line 1999) ✅

### Problematic Flow (If `setTimelineViewMode()` is Called)

1. **External call** → `setTimelineViewMode(FROZEN_MODE)` (line 2029)
2. **Mode set** → `m_timelineViewMode = FROZEN_MODE` (line 2031)
3. **Visualizer updated** → `m_visualizerWidget->setTimelineViewMode(FROZEN_MODE)` (line 2036) ✅
4. **Bug #1** → `handleModeTransitionLogic(FROZEN_MODE)` called with hardcoded value (line 2042) ❌
5. **Bug #2** → Sync state NOT updated ❌
6. **Result**: Mode is set locally but sync state is out of sync

## Recommended Fixes

### Fix #1: Correct `setTimelineViewMode()` Logic

**Change**:
```cpp
void TimelineView::setTimelineViewMode(TimelineViewMode mode)
{
    m_timelineViewMode = mode;
    
    // Update the visualizer widget's mode
    if (m_visualizerWidget)
    {
        m_visualizerWidget->setTimelineViewMode(mode);
    }

    // Update sync state immediately (same as onTimelineViewModeChanged)
    if (m_syncState)
    {
        m_syncState->isGraphContainerInFollowMode = (mode == TimelineViewMode::FOLLOW_MODE);
    }

    // Handle mode transition logic (only once, with correct mode)
    handleModeTransitionLogic(mode);
    
    // Emit signal for mode change (for consistency)
    bool isInFollowMode = (mode == TimelineViewMode::FOLLOW_MODE);
    emit GraphContainerInFollowModeChanged(isInFollowMode);
}
```

### Fix #2: Ensure Timer Behavior Respects Mode

**Consideration**: The `ensureTimerRunning()` call in `onVisibleTimeWindowChanged()` might need to check mode:

```cpp
void TimelineView::onVisibleTimeWindowChanged(const TimeSelectionSpan& selection)
{
    // ... existing code ...
    
    // Only ensure timer is running if in FOLLOW_MODE
    // In FROZEN_MODE, timer can be stopped to save resources
    if (m_timelineViewMode == TimelineViewMode::FOLLOW_MODE)
    {
        ensureTimerRunning();
    }
}
```

**Note**: This is optional - the timer running doesn't cause mode changes since `setCurrentTime()` respects the mode.

## Verification Steps

1. **Test direct mode setting**: Call `setTimelineViewMode(FROZEN_MODE)` and verify:
   - Mode stays frozen
   - Sync state is updated correctly
   - No mode resets occur

2. **Test slider drag**: Drag slider away from top and verify:
   - Mode switches to FROZEN_MODE
   - Mode stays frozen
   - Timer continues but doesn't update slider position

3. **Test multiple containers**: If multiple GraphContainers exist:
   - Set one to FROZEN_MODE
   - Verify others don't reset it
   - Verify sync state is correct

## Related Code Locations

| Component | File | Lines | Status |
|-----------|------|-------|--------|
| `setTimelineViewMode()` | `timelineview.cpp` | 2029-2048 | ❌ Bug |
| `onTimelineViewModeChanged()` | `timelineview.cpp` | 1990-2008 | ✅ Correct |
| `setCurrentTime()` | `timelineview.cpp` | 376-420 | ✅ Respects mode |
| `onTimerTick()` | `graphcontainer.cpp` | 160-195 | ✅ Doesn't poll mode |
| `handleModeTransitionLogic()` | `timelineview.cpp` | 1981-1987 | ⚠️ Empty (TODO) |

## Summary

### Primary Bugs (FIXED)
**Bug #3**: External sync via `setTimeWindowSilent()` was resetting frozen slider positions via GraphLayout hub.

**Bug #4**: Signal-based sync via `setVisibleTimeWindow()` was resetting frozen slider positions when other timeline views changed.

**Fixes Applied**: 
- Modified `setTimeWindowSilent()` to check mode and return early in FROZEN_MODE
- Modified `setVisibleTimeWindow()` to check mode and return early in FROZEN_MODE
- Both methods now preserve frozen state regardless of external sync calls

### Secondary Bugs (FIXED)
**Bug #1**: `setTimelineViewMode()` called transition logic with hardcoded FROZEN_MODE
**Bug #2**: `setTimelineViewMode()` didn't update sync state

**Fixes Applied**: 
- Removed hardcoded transition logic call
- Added sync state update
- Added signal emission for consistency

## Fix Verification

After these fixes, frozen mode should:
1. ✅ Stay frozen when slider is dragged away from top
2. ✅ Ignore external sync calls from GraphLayout hub (`setTimeWindowSilent`)
3. ✅ Ignore signal-based sync from other timeline views (`setVisibleTimeWindow`)
4. ✅ Preserve slider position even when other containers/timelines change
5. ✅ Maintain correct sync state
6. ✅ Work correctly with SCW timeline synchronization

## Testing Checklist

- [ ] Drag slider away from top → should switch to FROZEN_MODE
- [ ] Verify slider stays in frozen position
- [ ] Change time window in another container → frozen slider should NOT move
- [ ] Change time window in another timeline view → frozen slider should NOT move
- [ ] Change SCW timeline → frozen GraphLayout timeline should NOT move
- [ ] Verify mode doesn't switch back to FOLLOW_MODE automatically
- [ ] Test with multiple containers to ensure sync doesn't break frozen mode
- [ ] Test with SCW window to ensure bidirectional sync respects frozen mode


