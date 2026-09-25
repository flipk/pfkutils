#!/usr/bin/env python3
# 
# opencode export <session_id> > export.json
# python render.py export.json session.html

import json
import sys
import html
from datetime import datetime

try:
    import markdown
except ImportError:
    print("Error: The 'markdown' package is missing.")
    print("Please install it by running: pip install markdown")
    sys.exit(1)

if len(sys.argv) < 2:
    print("Usage: opencode export <session_id> > export.json")
    print("       python render.py <export.json> <output.html>")
    sys.exit(1)

with open(sys.argv[1], "r", encoding="utf-8") as f:
    data = json.load(f)

out_path = sys.argv[2] if len(sys.argv) > 2 else "session.html"

messages = data.get("messages", data.get("history", []))

md = markdown.Markdown(extensions=['fenced_code', 'tables'])


def format_timestamp(ts_ms):
    """Convert milliseconds timestamp to readable format."""
    if ts_ms is None:
        return ""
    dt = datetime.fromtimestamp(ts_ms / 1000)
    return dt.strftime("%Y-%m-%d %H:%M:%S")

def is_compaction_message(msg):
    """Check if a message is a compaction message."""
    info = msg.get("info", {})
    return info.get("mode") == "compaction" and info.get("agent") == "compaction"

def format_tool_call(part):
    tool_name = part.get("tool", "unknown")
    call_id = part.get("callID", "")[:8]
    state = part.get("state", {})
    status = state.get("status", "unknown")
    input_data = state.get("input", {})
    output_data = state.get("output", {})
    error = state.get("error")
    time_info = state.get("time", {})
    
    input_str = json.dumps(input_data, indent=2) if input_data else ""
    if error:
        output_str = f"ERROR: {error}"
    elif output_data:
        if isinstance(output_data, dict):
            output_str = json.dumps(output_data, indent=2)
        else:
            output_str = str(output_data)
    else:
        output_str = "(no output)"
    
    time_str = ""
    if time_info.get("start") and time_info.get("end"):
        duration = time_info["end"] - time_info["start"]
        time_str = f" ({duration}ms)"
    
    output_lines = output_str.split('\n')
    is_long = len(output_lines) > 5
    summary_lines = output_lines[:5] if is_long else output_lines
    summary = '\n'.join(summary_lines)
    if is_long:
        summary += f"\n... ({len(output_lines)} lines total, click to expand)"
    
    if is_long:
        output_html = f"""<details class="tool-output-full">
<summary><pre>{html.escape(summary)}</pre></summary>
<pre>{html.escape(output_str)}</pre>
</details>"""
    else:
        output_html = f"""<div class="tool-output-summary"><pre>{html.escape(summary)}</pre></div>"""
    
    return f"""<details class="tool-call">
<summary class="tool-summary">🔧 {tool_name} [{status}]{time_str} <span class="call-id">#{call_id}</span></summary>
<div class="tool-detail">
<div class="tool-input"><strong>Input:</strong><pre>{html.escape(input_str)}</pre></div>
<div class="tool-output"><strong>Output:</strong>
{output_html}
</div>
</div>
</details>"""

def format_reasoning(text, open_by_default=False):
    rendered = md.convert(text)
    md.reset()
    open_attr = " open" if open_by_default else ""
    return f"""<details class="thinking"{open_attr}>
<summary>💭 Thinking ({len(text)} chars)</summary>
<div class="thinking-content">{rendered}</div>
</details>"""

html_parts = [
    "<!DOCTYPE html><html><head><meta charset='utf-8'>",
    "<title>OpenCode Session Export</title>",
    "<style>",
    "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; max-width: 860px; margin: 2rem auto; padding: 0 1rem; background: #0f172a; color: #e2e8f0; line-height: 1.6; }",
    ".msg { margin-bottom: 1.5rem; padding: 1.25rem 1.5rem; border-radius: 8px; }",
    ".user { background: #1e293b; border-left: 4px solid #38bdf8; }",
    ".assistant { background: #1e293b; border-left: 4px solid #4ade80; }",
    ".role { font-weight: bold; text-transform: uppercase; font-size: 0.75rem; letter-spacing: 0.05em; margin-bottom: 1rem; color: #94a3b8; }",
    ".timestamp { font-weight: normal; text-transform: none; font-size: 0.7rem; color: #64748b; margin-left: 0.75rem; }",
    ".compaction { background: #1e293b; border-left: 4px solid #f87171; }",
    ".compaction .role { color: #f87171; }",
    ".compaction-summary { cursor: pointer; font-weight: 600; color: #f87171; user-select: none; padding: 0.75rem 0; }",
    ".compaction-summary:hover { color: #fca5a5; }",
    ".compaction[open] .compaction-summary::before { content: '▼ '; }",
    ".compaction .compaction-summary::before { content: '▶ '; }",
    ".compaction-content { padding: 0.5rem 0 0.75rem; }",
    ".compaction-content .role { display: none; }",
    ".content p { margin-top: 0; margin-bottom: 1rem; }",
    ".content p:last-child { margin-bottom: 0; }",
    ".content a { color: #38bdf8; text-decoration: none; }",
    ".content a:hover { text-decoration: underline; }",
    ".content code { font-family: ui-monospace, SFMono-Regular, monospace; font-size: 0.9em; background: #0b1120; padding: 0.2rem 0.4rem; border-radius: 4px; color: #e2e8f0; }",
    ".content pre { background: #0b1120; padding: 1rem; border-radius: 6px; overflow-x: auto; border: 1px solid #334155; margin-bottom: 1rem; }",
    ".content pre code { padding: 0; background: transparent; border-radius: 0; color: inherit; }",
    ".content table { border-collapse: collapse; width: 100%; margin-bottom: 1rem; }",
    ".content th, .content td { border: 1px solid #334155; padding: 0.5rem; text-align: left; }",
    ".content th { background-color: #0b1120; font-weight: 600; }",
    ".content blockquote { border-left: 4px solid #475569; margin: 0; padding-left: 1rem; color: #cbd5e1; }",
    ".activity { border-left: 4px solid #fbbf24; background: #1e293b; margin: 0.75rem 0; padding: 1rem; border-radius: 6px; }",
    ".activity summary { cursor: pointer; font-weight: 600; color: #fbbf24; user-select: none; }",
    ".activity summary:hover { color: #fcd34d; }",
    ".activity[open] summary::before { content: '▼ '; }",
    ".activity summary::before { content: '▶ '; }",
    ".activity-content { margin-top: 0.75rem; }",
    ".tool-call { background: #0b1120; border: 1px solid #334155; border-radius: 4px; margin: 0.5rem 0; }",
    ".tool-call summary { padding: 0.5rem 0.75rem; cursor: pointer; user-select: none; }",
    ".tool-call summary:hover { background: #1e293b; }",
    ".tool-summary { display: flex; align-items: center; gap: 0.5rem; }",
    ".call-id { color: #64748b; font-size: 0.8em; }",
    ".tool-detail { padding: 0.75rem; border-top: 1px solid #334155; }",
    ".tool-input pre, .tool-output pre { background: #0b1120; padding: 0.5rem; border-radius: 4px; overflow-x: auto; font-size: 0.85em; color: #94a3b8; margin: 0.25rem 0; white-space: pre-wrap; }",
    ".tool-output-full { margin-top: 0.5rem; }",
    ".tool-output-full summary { cursor: pointer; user-select: none; padding: 0; margin: 0; }",
    ".tool-output-full summary pre { color: #94a3b8; background: #0b1120; padding: 0.5rem; border-radius: 4px; overflow-x: auto; font-size: 0.85em; margin: 0.25rem 0; white-space: pre-wrap; }",
    ".tool-output-full summary pre:hover { color: #cbd5e1; }",
    ".tool-output-full summary::marker { display: none; }",
    ".tool-output-full summary::-webkit-details-marker { display: none; }",
    ".tool-output-full[open] summary { display: none; }",
    ".tool-output-full pre { background: #0b1120; color: #e2e8f0; }",
    ".tool-output-summary pre { color: #94a3b8; }",
    ".thinking { margin: 0.5rem 0; padding: 0.75rem; background: #0b1120; border-radius: 4px; border-left: 3px solid #fbbf24; }",
    ".thinking summary { cursor: pointer; font-weight: 600; color: #fbbf24; user-select: none; font-size: 0.9em; }",
    ".thinking summary:hover { color: #fcd34d; }",
    ".thinking .thinking-content { margin-top: 0.5rem; white-space: pre-wrap; font-family: ui-monospace, SFMono-Regular, monospace; font-size: 0.85em; color: #94a3b8; }",
    ".thinking[open] summary::before { content: '▼ '; }",
    ".thinking summary::before { content: '▶ '; }",
    "</style></head><body><h1>OpenCode Session Transcript</h1>"
]

def extract_activities(msg):
    """Extract activity groups and regular content from a message."""
    regular_parts = []
    activity_groups = []
    current_activity = []
    
    if "parts" in msg:
        for p in msg["parts"]:
            ptype = p.get("type", "text")
            if ptype == "reasoning":
                text = p.get("text", "")
                if text:
                    current_activity.append(("reasoning", text))
            elif ptype == "tool":
                current_activity.append(("tool", p))
            else:
                text = p.get("text", "")
                if text:
                    if current_activity:
                        activity_groups.append(current_activity)
                        current_activity = []
                    regular_parts.append(text)
        if current_activity:
            activity_groups.append(current_activity)
    elif "content" in msg:
        content = msg["content"] if isinstance(msg["content"], str) else json.dumps(msg["content"], indent=2)
        regular_parts.append(content)
    
    return regular_parts, activity_groups


def has_activities(activity_groups):
    """Check if activity groups contain any reasoning or tool calls."""
    for activity in activity_groups:
        for item_type, _ in activity:
            if item_type in ("reasoning", "tool"):
                return True
    return False


def render_message(msg, role, regular_parts, activity_groups, timestamp=""):
    """Render a single message or group of messages."""
    compaction = is_compaction_message(msg)
    msg_class = "compaction" if compaction else html.escape(role)
    
    if compaction:
        html_parts.append(f"<details class='msg {msg_class}'><summary class='compaction-summary'>COMPACTION")
        if timestamp:
            html_parts.append(f" <span class='timestamp'>{html.escape(timestamp)}</span>")
        html_parts.append("</summary>")
        html_parts.append("<div class='compaction-content'>")
    else:
        html_parts.append(f"<div class='msg {msg_class}'>")
        role_label = html.escape(role).upper()
        time_label = f" <span class='timestamp'>{html.escape(timestamp)}</span>" if timestamp else ""
        html_parts.append(f"<div class='role'>{role_label}{time_label}</div>")
    
    if regular_parts:
        content = "\n".join(regular_parts)
        rendered_html = md.convert(content)
        md.reset()
        html_parts.append(f"<div class='content'>{rendered_html}</div>")
    
    if activity_groups:
        all_reasoning = []
        all_tools = []
        for activity in activity_groups:
            for item_type, item_data in activity:
                if item_type == "reasoning":
                    all_reasoning.append(item_data)
                elif item_type == "tool":
                    all_tools.append(item_data)
        
        reasoning_count = len(all_reasoning)
        tool_count = len(all_tools)
        total_chars = sum(len(t) for t in all_reasoning)
        
        label_parts = []
        if reasoning_count:
            label_parts.append(f"{reasoning_count} reasoning")
        if tool_count:
            label_parts.append(f"{tool_count} tool call{'s' if tool_count > 1 else ''}")
        if compaction:
            label_parts.append("compaction")
        label = ", ".join(label_parts)
        
        html_parts.append(f"<details class='activity'><summary>Activity: {label} ({total_chars} chars)</summary>")
        html_parts.append("<div class='activity-content'>")
        
        for activity in activity_groups:
            for item_type, item_data in activity:
                if item_type == "reasoning":
                    html_parts.append(format_reasoning(item_data, open_by_default=not compaction))
                else:
                    html_parts.append(format_tool_call(item_data))
        
        html_parts.append("</div></details>")
    
    if compaction:
        html_parts.append("</div></details>")
    else:
        html_parts.append("</div>")


i = 0
while i < len(messages):
    msg = messages[i]
    role = msg.get("info", {}).get("role", "unknown")
    info = msg.get("info", {})
    
    # Get timestamp from message info
    timestamp = ""
    if "time" in info and "created" in info["time"]:
        timestamp = format_timestamp(info["time"]["created"])
    elif "created" in info:
        timestamp = format_timestamp(info["created"])
    
    regular_parts, activity_groups = extract_activities(msg)
    msg_has_activities = has_activities(activity_groups)
    compaction = is_compaction_message(msg)
    
    if role == "assistant" and msg_has_activities and not compaction:
        combined_regular = []
        combined_activities = []
        combined_timestamp = timestamp
        j = i
        while j < len(messages):
            next_msg = messages[j]
            next_role = next_msg.get("info", {}).get("role", "unknown")
            next_info = next_msg.get("info", {})
            next_regular, next_activities = extract_activities(next_msg)
            next_has_activities = has_activities(next_activities)
            next_compaction = is_compaction_message(next_msg)
            
            if next_role == "assistant" and next_has_activities and not next_compaction:
                combined_regular.extend(next_regular)
                combined_activities.extend(next_activities)
                # Use the earliest timestamp
                next_ts = ""
                if "time" in next_info and "created" in next_info["time"]:
                    next_ts = format_timestamp(next_info["time"]["created"])
                elif "created" in next_info:
                    next_ts = format_timestamp(next_info["created"])
                if next_ts and (not combined_timestamp or next_ts < combined_timestamp):
                    combined_timestamp = next_ts
                j += 1
            else:
                break
        
        render_message(msg, role, combined_regular, combined_activities, combined_timestamp)
        i = j
    else:
        render_message(msg, role, regular_parts, activity_groups, timestamp)
        i += 1

html_parts.append("</body></html>")

with open(out_path, "w", encoding="utf-8") as f:
    f.write("\n".join(html_parts))

print(f"Rendered to {out_path}")
