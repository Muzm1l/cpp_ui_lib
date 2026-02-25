# BTW Manual Marker Bearing-Rate Box (Local per Graph)

This document describes how the bearing-rate value shown in the manual marker box is calculated **locally** for each BTW graph, based on that graph’s zoom panel range.

---

## Overview

- There can be **multiple BTW graphs** (e.g. four), each with its own **zoom panel** (e.g. 0–360° or 330–360°).
- Manual markers are synced across graphs: **rotation is stored globally** (0–360°).
- The **number shown in the bearing-rate box** is computed **per graph** and uses the **same scale as the zoom panel stickers**: graph A (0–360) shows 0–360, graph B (330–360) shows 330–360 (not 0–30).

---

## Local display formula

For each graph:

1. **Visible range** comes from that graph’s zoom panel:
   - `visibleMin` = zoom panel left (start) value  
   - `visibleMax` = zoom panel right value  
   - `visibleSpan` = `visibleMax - visibleMin` (e.g. 360 or 30)

2. **Global rotation** = marker rotation in 0–360° (same for all graphs).

3. **Normalized offset** within the visible window (0 to visibleSpan):

   ```text
   localOffset = globalRotation × (visibleSpan / 360)
   ```

4. **Box display value** (sticker-scale, same as zoom panel):

   ```text
   boxDisplayValue = visibleMin + localOffset
   ```

   Rounded to an integer for display.

**Example**

- Graph A: zoom panel 0–360° → visibleMin=0, span 360.  
  Global rotation 60° → localOffset = 60 × (360/360) = 60 → box = 0 + 60 = **60**.

- Graph B: zoom panel 330–360° → visibleMin=330, span 30.  
  Global rotation 60° → localOffset = 60 × (30/360) = 5 → box = 330 + 5 = **335**.  
  Global 330° → box **330**; global 360° → box **360**.

So the box always shows values in the range **visibleMin to visibleMax** (same as the stickers).

---

## Where it is implemented

- **Box value calculation:** `BTWInteractiveOverlay::updateBearingRateBox()` in `btwinteractiveoverlay.cpp`.  
  It calls `BTWGraph::getVisibleBearingRange()` to get this graph’s range, then computes the local value and uses it for the box text (R/L prefix logic is unchanged).

- **Range source:** `BTWGraph::getVisibleBearingRange(qreal &outMin, qreal &outMax)` in `btwgraph.cpp` / `btwgraph.h`.  
  It reads the zoom panel’s left/right label values for that graph’s container. If there is no panel or the range is invalid, it returns (0, 360).

---

## API

### `void BTWGraph::getVisibleBearingRange(qreal &outMin, qreal &outMax) const`

Returns this BTW graph’s visible bearing (X) range from its zoom panel.

- **outMin** – left (min) bearing value  
- **outMax** – right (max) bearing value  

If the zoom panel is missing or the range is invalid (e.g. `outMax <= outMin`), `outMin` and `outMax` are set to 0 and 360.

---

## Sync behaviour

- Marker **rotation** and sync data remain in **global** 0–360° units.
- Only the **display** in the bearing-rate box is local. Each graph runs `updateBearingRateBox()` with its own visible range, so no global “box value” is stored or synced.

---

## Summary

| Item              | Scope   | Notes                                      |
|-------------------|---------|--------------------------------------------|
| Marker rotation   | Global  | 0–360°, synced across graphs               |
| Box displayed value | Local   | Per graph: `visibleMin + (global × visibleSpan / 360)` → sticker scale (e.g. 0–360 or 330–360) |
| R/L prefix        | Unchanged | Still derived from global rotation direction |
