
# pip3 install pywin32

import win32api
import win32con
import json
import re
import sys
from dataclasses import dataclass, replace
from typing import Dict, List, Tuple

@dataclass
class MonitorInfo:
    adapter_name: str         # The GDI name (e.g., \\.\DISPLAY1)
    monitor_name: str         # The friendly name (e.g., Dell U2720Q)
    hardware_id: str          # The PnP ID used in the Windows Registry
    is_primary: bool
    pos_x: int                # X coordinate relative to the primary monitor
    pos_y: int                # Y coordinate relative to the primary monitor
    res_width: int
    res_height: int
    work_area: tuple          # (Left, Top, Right, Bottom) excluding taskbar
    refresh_rate: int         # In Hz

def retrieve_monitors() -> List[MonitorInfo]:
    """Retrieves a list of all active monitors and their layout information."""
    monitors = []
    
    # EnumDisplayMonitors returns (hMonitor, hdcMonitor, PyRECT) for active displays
    for hMonitor, hdcMonitor, pyRect in win32api.EnumDisplayMonitors():
        monitor_info = win32api.GetMonitorInfo(hMonitor)
        adapter_name = monitor_info['Device']
        
        # Calculate resolution from absolute coordinate geometry
        pos_x, pos_y, right, bottom = monitor_info['Monitor']
        
        # Grab current display settings for refresh rate
        settings = win32api.EnumDisplaySettings(adapter_name, win32con.ENUM_CURRENT_SETTINGS)
        
        # The 0th device on the adapter contains the physical monitor hardware info
        device = win32api.EnumDisplayDevices(adapter_name, 0)
        
        monitors.append(MonitorInfo(
            adapter_name=adapter_name,
            monitor_name=device.DeviceString,
            hardware_id=device.DeviceID,
            is_primary=monitor_info['Flags'] == win32con.MONITORINFOF_PRIMARY,
            pos_x=pos_x,
            pos_y=pos_y,
            res_width=right - pos_x,
            res_height=bottom - pos_y,
            work_area=monitor_info['Work'],
            refresh_rate=settings.DisplayFrequency
        ))
    
    return monitors

def print_monitors(monitors: List[MonitorInfo]) -> None:
    """Iterates over the retrieved monitor data and prints it cleanly."""
    print(f"Found {len(monitors)} active monitor(s):")
    print("   NOTE: numbers DO NOT match what is shown in System > Display")
    print("-" * 50)
    
    for mon in monitors:
        primary_tag = " (PRIMARY)" if mon.is_primary else ""
        print(f"  Adapter:      {mon.adapter_name}{primary_tag}")
        print(f"  Model Name:   {mon.monitor_name}")
        print(f"  Hardware ID:  {mon.hardware_id}")
        print(f"  Resolution:   {mon.res_width}x{mon.res_height} @ {mon.refresh_rate}Hz")
        print(f"  Position:     X: {mon.pos_x}, Y: {mon.pos_y}")
        print(f"  Work Area:    Left: {mon.work_area[0]}, Top: {mon.work_area[1]}, "
              f"Right: {mon.work_area[2]}, Bottom: {mon.work_area[3]}")
        print("-" * 50)


def analyze_monitor_layout(monitors: List[MonitorInfo], gap_threshold: int = 50, tolerance: int = 2) -> List[str]:
    """
    Analyzes monitor layout for unintended gaps, overlaps, and edge misalignments.
    """
    issues = []
    
    # Compare every unique pair of monitors
    for i in range(len(monitors)):
        for j in range(i + 1, len(monitors)):
            m1, m2 = monitors[i], monitors[j]
            
            # Calculate absolute bounding boxes
            m1_left, m1_right = m1.pos_x, m1.pos_x + m1.res_width
            m1_top, m1_bottom = m1.pos_y, m1.pos_y + m1.res_height
            
            m2_left, m2_right = m2.pos_x, m2.pos_x + m2.res_width
            m2_top, m2_bottom = m2.pos_y, m2.pos_y + m2.res_height
            
            # 1. SIDE-BY-SIDE CHECK (Vertical Line of Sight)
            # They share vertical space if the top of one is higher than the bottom of the other, and vice versa
            if max(m1_top, m2_top) < min(m1_bottom, m2_bottom):
                left_mon, right_mon = (m1, m2) if m1_left <= m2_left else (m2, m1)
                
                # Check horizontal gaps / overlaps
                distance = right_mon.pos_x - (left_mon.pos_x + left_mon.res_width)
                
                if distance < -tolerance:
                    issues.append(f"OVERLAP: '{left_mon.adapter_name}' & '{right_mon.adapter_name}' overlap horizontally by {abs(distance)}px.")
                elif tolerance < distance <= gap_threshold:
                    issues.append(f"GAP: {distance}px horizontal gap between '{left_mon.adapter_name}' & '{right_mon.adapter_name}'.")
                    
                # Heuristic Edge Alignment (Top/Bottom edges)
                top_diff = abs(left_mon.pos_y - right_mon.pos_y)
                if tolerance < top_diff <= gap_threshold:
                    issues.append(f"MISALIGN: Top edges of '{left_mon.adapter_name}' & '{right_mon.adapter_name}' are off by {top_diff}px.")
                    
                bottom_diff = abs((left_mon.pos_y + left_mon.res_height) - (right_mon.pos_y + right_mon.res_height))
                if tolerance < bottom_diff <= gap_threshold:
                    issues.append(f"MISALIGN: Bottom edges of '{left_mon.adapter_name}' & '{right_mon.adapter_name}' are off by {bottom_diff}px.")

            # 2. STACKED CHECK (Horizontal Line of Sight)
            # They share horizontal space if the left of one is further left than the right of the other, and vice versa
            if max(m1_left, m2_left) < min(m1_right, m2_right):
                top_mon, bot_mon = (m1, m2) if m1_top <= m2_top else (m2, m1)
                
                # Check vertical gaps / overlaps
                distance = bot_mon.pos_y - (top_mon.pos_y + top_mon.res_height)
                
                if distance < -tolerance:
                    issues.append(f"OVERLAP: '{top_mon.adapter_name}' & '{bot_mon.adapter_name}' overlap vertically by {abs(distance)}px.")
                elif tolerance < distance <= gap_threshold:
                    issues.append(f"GAP: {distance}px vertical gap between '{top_mon.adapter_name}' & '{bot_mon.adapter_name}'.")
                    
                # Heuristic Edge Alignment (Left/Right edges)
                left_diff = abs(top_mon.pos_x - bot_mon.pos_x)
                if tolerance < left_diff <= gap_threshold:
                    issues.append(f"MISALIGN: Left edges of '{top_mon.adapter_name}' & '{bot_mon.adapter_name}' are off by {left_diff}px.")
                    
                right_diff = abs((top_mon.pos_x + top_mon.res_width) - (bot_mon.pos_x + bot_mon.res_width))
                if tolerance < right_diff <= gap_threshold:
                    issues.append(f"MISALIGN: Right edges of '{top_mon.adapter_name}' & '{bot_mon.adapter_name}' are off by {right_diff}px.")

    return issues

def print_layout_analysis(issues: List[str]) -> None:
    """Prints the discovered topology issues to the console."""
    print("Topology Layout Analysis:\n" + "=" * 50)
    if not issues:
        print("No gaps, overlaps, or misalignments detected within tolerances.")
    else:
        for issue in issues:
            print(f"- {issue}")
    print("=" * 50)


def calculate_aligned_positions(
    monitors: List[MonitorInfo], 
    gap_threshold: int = 100, 
    align_tolerance: int = 50
) -> Dict[str, Tuple[int, int]]:
    """
    Computes new (pos_x, pos_y) coordinates for each monitor to eliminate
    gaps, overlaps, and minor edge misalignments using a primary-rooted BFS.
    """
    # Map monitors by adapter name
    by_adapter = {m.adapter_name: m for m in monitors}
    primary = next((m for m in monitors if m.is_primary), monitors[0])
    
    # Store resolved positions: {adapter_name: (pos_x, pos_y)}
    new_positions: Dict[str, Tuple[int, int]] = {
        primary.adapter_name: (0, 0)
    }
    
    unplaced = set(by_adapter.keys()) - {primary.adapter_name}

    while unplaced:
        best_candidate = None
        best_parent = None
        best_relation = None  # ('horizontal' | 'vertical', direction)

        # Find the unplaced monitor with the closest proximity to an already-placed monitor
        for u_name in unplaced:
            u_mon = by_adapter[u_name]
            for p_name in new_positions:
                p_mon = by_adapter[p_name]
                p_x, p_y = new_positions[p_name]

                # Check Horizontal Line of Sight (Side-by-Side)
                shared_y = max(u_mon.pos_y, p_mon.pos_y) < min(u_mon.pos_y + u_mon.res_height, p_mon.pos_y + p_mon.res_height)
                if shared_y:
                    if u_mon.pos_x >= p_mon.pos_x + p_mon.res_width - gap_threshold:
                        dist = abs(u_mon.pos_x - (p_mon.pos_x + p_mon.res_width))
                        if dist <= gap_threshold:
                            best_candidate, best_parent = u_name, p_name
                            best_relation = ('side_by_side', 'right')
                            break
                    elif u_mon.pos_x + u_mon.res_width <= p_mon.pos_x + gap_threshold:
                        dist = abs(p_mon.pos_x - (u_mon.pos_x + u_mon.res_width))
                        if dist <= gap_threshold:
                            best_candidate, best_parent = u_name, p_name
                            best_relation = ('side_by_side', 'left')
                            break

                # Check Vertical Line of Sight (Stacked)
                shared_x = max(u_mon.pos_x, p_mon.pos_x) < min(u_mon.pos_x + u_mon.res_width, p_mon.pos_x + p_mon.res_width)
                if shared_x:
                    if u_mon.pos_y >= p_mon.pos_y + p_mon.res_height - gap_threshold:
                        dist = abs(u_mon.pos_y - (p_mon.pos_y + p_mon.res_height))
                        if dist <= gap_threshold:
                            best_candidate, best_parent = u_name, p_name
                            best_relation = ('stacked', 'bottom')
                            break
                    elif u_mon.pos_y + u_mon.res_height <= p_mon.pos_y + gap_threshold:
                        dist = abs(p_mon.pos_y - (u_mon.pos_y + u_mon.res_height))
                        if dist <= gap_threshold:
                            best_candidate, best_parent = u_name, p_name
                            best_relation = ('stacked', 'top')
                            break

            if best_candidate:
                break

        # Fallback if an orphan display isn't within gap_threshold
        if not best_candidate:
            orphan = unplaced.pop()
            new_positions[orphan] = (by_adapter[orphan].pos_x, by_adapter[orphan].pos_y)
            continue

        p_x, p_y = new_positions[best_parent]
        p_mon = by_adapter[best_parent]
        c_mon = by_adapter[best_candidate]
        
        rel_type, side = best_relation

        if rel_type == 'side_by_side':
            # X coordinate: place flush to left or right
            new_x = (p_x + p_mon.res_width) if side == 'right' else (p_x - c_mon.res_width)
            
            # Y coordinate: snap edges if within tolerance, otherwise preserve relative offset
            top_diff = abs(c_mon.pos_y - p_mon.pos_y)
            bot_diff = abs((c_mon.pos_y + c_mon.res_height) - (p_mon.pos_y + p_mon.res_height))
            
            if top_diff <= align_tolerance and top_diff <= bot_diff:
                new_y = p_y
            elif bot_diff <= align_tolerance:
                new_y = p_y + p_mon.res_height - c_mon.res_height
            else:
                new_y = p_y + (c_mon.pos_y - p_mon.pos_y)
                
            new_positions[best_candidate] = (new_x, new_y)

        elif rel_type == 'stacked':
            # Y coordinate: place flush above or below
            new_y = (p_y + p_mon.res_height) if side == 'bottom' else (p_y - c_mon.res_height)
            
            # X coordinate: snap edges if within tolerance
            left_diff = abs(c_mon.pos_x - p_mon.pos_x)
            right_diff = abs((c_mon.pos_x + c_mon.res_width) - (p_mon.pos_x + p_mon.res_width))
            
            if left_diff <= align_tolerance and left_diff <= right_diff:
                new_x = p_x
            elif right_diff <= align_tolerance:
                new_x = p_x + p_mon.res_width - c_mon.res_width
            else:
                new_x = p_x + (c_mon.pos_x - p_mon.pos_x)
                
            new_positions[best_candidate] = (new_x, new_y)

        unplaced.remove(best_candidate)

    return new_positions

def apply_monitor_positions(new_positions: Dict[str, Tuple[int, int]],
                            dry_run: bool = True) -> None:
    """
    Applies the computed (pos_x, pos_y) positions to Windows display settings.
    
    :param new_positions: Mapping of adapter_name (e.g., '\\\\.\\DISPLAY1') to (x, y) coordinates.
    :param dry_run: If True, tests and prints changes without persisting or resetting the display.
    """
    print(f"{'[DRY RUN] ' if dry_run else ''}Applying new monitor positions...")

    # Stage 1: Update registry / settings for each display device
    for adapter_name, (x, y) in new_positions.items():
        # Retrieve current DEVMODE for this display adapter
        devmode = win32api.EnumDisplaySettings(adapter_name, win32con.ENUM_CURRENT_SETTINGS)
        
        # Configure DEVMODE for position change only
        devmode.Position_x = x
        devmode.Position_y = y
        devmode.Fields = win32con.DM_POSITION

        # CDS_TEST validates the config without making changes
        # CDS_NORESET stages settings without resetting the screen per monitor
        # CDS_UPDATEREGISTRY makes changes persist across reboots
        if dry_run:
            flags = win32con.CDS_TEST
        else:
            flags = win32con.CDS_UPDATEREGISTRY | win32con.CDS_NORESET

        result = win32api.ChangeDisplaySettingsEx(adapter_name, devmode, flags)

        status_msg = "SUCCESS" if result == win32con.DISP_CHANGE_SUCCESSFUL else f"FAILED (Code: {result})"
        print(f"  {adapter_name} -> Pos({x}, {y}) : {status_msg}")

        if result != win32con.DISP_CHANGE_SUCCESSFUL:
            print(f"Aborting apply sequence due to failure on {adapter_name}.")
            return

    # Stage 2: Commit and dynamically execute the batch update across all monitors
    if not dry_run:
        commit_result = win32api.ChangeDisplaySettingsEx(None, None, 0)
        if commit_result == win32con.DISP_CHANGE_SUCCESSFUL:
            print("Successfully updated and reset display topology.")
        else:
            print(f"Topology batch commit failed (Code: {commit_result}).")
    else:
        print("[DRY RUN] All positions verified valid by Windows GDI.")
    

def main():
    dry_run = True
    if len(sys.argv) > 1:
        if sys.argv[1] == '-u':
            dry_run = False

    monitor_data = retrieve_monitors()
    print_monitors(monitor_data)

    # Pass the data structure to the analyzer
    layout_issues = analyze_monitor_layout(monitor_data, gap_threshold=50, tolerance=2)
    if not layout_issues:
        print('no layout issues!')
        return 0

    print_layout_analysis(layout_issues)

    new_positions = calculate_aligned_positions(monitor_data)
    #print(json.dumps(new_positions, indent=4))
    apply_monitor_positions(new_positions, dry_run=dry_run)
    if dry_run:
        print('add -u to the command line to perform the update')

    return 0

if __name__ == "__main__":
    exit(main())
