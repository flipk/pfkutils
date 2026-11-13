#!/usr/bin/env python3
# 
# opencode v1:
#    opencode export <session_id> > export.json
# opencode v2:
#    opencode session export <session_id> > export.json
# python render.py export.json session.html

# differences between json schema of opencode v1 and v2:
#
# 1. Message Role & Type Resolution (get_message_role):
#    - v1: Role was stored in msg["info"]["role"] or msg["role"].
#    - v2: Role/type is stored directly in msg["type"] ("user",
#      "assistant", "idle", "compaction", etc.).
#    - Updated helper to check msg["info"]["role"], msg["role"],
#      and msg["type"]. Non-transcript internal state messages (such
#      as type: "idle") are filtered out automatically.
#
# 2. Message Text & Part Extraction (extract_activities):
#    - v1: User text and assistant parts were stored in msg["parts"]
#      or msg["content"].
#    - v2: User prompts are in msg["text"], while assistant responses
#      (text, reasoning, tool calls) are in msg["content"].
#    - Updated helper to extract content across msg["parts"],
#      msg["content"] (string or array of part dicts), and
#      msg["text"].
#
# 3. Tool Call & Execution Formatting (format_tool_call):
#    - v1: Tool name was part["tool"], call ID was part["callID"],
#      state output was part["state"][ "output"], and timestamps were
#      state["time"]["start"]/end.
#    - v2: Tool name is part["name"], call ID is part["id"], tool
#      execution output is inside part[ "state"]["content"] (or
#      part["state"]["error"]), and timestamps are in part["time"] or
#      state[ "time"] (created, ran, completed).
#    - Updated format_tool_call to normalize tool names, call IDs
#      (stripping call_ prefix if present), inputs, errors, outputs,
#      and execution duration calculation across both versions.
#
# 4. Timestamps & Compaction Checks:
#    - Normalized timestamp extraction across msg["time"]["created"],
#      msg["info"]["time"], and msg[ "created"].
#    - Updated compaction message detection to match both v1
#      (info.mode == "compaction") and v2 (type == "compaction").
#    - Display session title in header and <title> when present in
#    - top-level info.title.


import json
import sys
import os
import html
import tempfile
from datetime import datetime

try:
    import markdown
except ImportError:
    print("Error: The 'markdown' package is missing.")
    print("Please install it by running: pip install markdown")
    sys.exit(1)

def usage():
    print("Usage: opencode v1: opencode         export <session_id> > export.json")
    print("Usage: opencode v2: opencode session export <session_id> > export.json")
    print("       python render.py <export.json> <output.html>")
    print("       python render.py <session_id> <output.html>")
    sys.exit(1)

if len(sys.argv) < 2:
    usage()

fname = sys.argv[1]

if fname.startswith('ses_'):
    # testing: with open('poo.json', mode='w+') as tmpf:
    with tempfile.NamedTemporaryFile(mode='r+', delete=True) as tmpf:
        # attempt the opencode v1 version of command
        ret = os.system(f'opencode export {fname} > {tmpf.name} 2>/dev/null')
        # if it returns a nonzero exit code,
        if ret != 0:
            # attempt the opencode v2 version
            ret = os.system(f'opencode session export {fname} > {tmpf.name} 2>/dev/null')
        data = json.load(tmpf)
elif fname.endswith('.json'):
    with open(fname, "r", encoding="utf-8") as f:
        data = json.load(f)
else:
    print('must supply a session id or json filename')
    usage()


out_path = sys.argv[2] if len(sys.argv) > 2 else "session.html"

messages = data.get("messages", data.get("history", []))
session_title = data.get("title") or data.get("info", {}).get("title") or ""

md = markdown.Markdown(extensions=['fenced_code', 'tables'])


def format_timestamp(ts_ms):
    """Convert milliseconds timestamp to readable format."""
    if ts_ms is None:
        return ""
    try:
        dt = datetime.fromtimestamp(ts_ms / 1000)
        return dt.strftime("%Y-%m-%d %H:%M:%S")
    except Exception:
        return ""


def get_message_timestamp(msg):
    """Extract creation timestamp from v1 or v2 message structures."""
    t = msg.get("time")
    if isinstance(t, dict):
        ts = t.get("created") or t.get("start")
        if ts:
            return format_timestamp(ts)
    info = msg.get("info")
    if isinstance(info, dict):
        t = info.get("time")
        if isinstance(t, dict):
            ts = t.get("created") or t.get("start")
            if ts:
                return format_timestamp(ts)
        if "created" in info:
            return format_timestamp(info["created"])
    if "created" in msg:
        return format_timestamp(msg["created"])
    return ""


def get_message_role(msg):
    """Extract message role/type across v1 and v2 schemas."""
    info = msg.get("info")
    if isinstance(info, dict) and "role" in info:
        return info["role"]
    if "role" in msg:
        return msg["role"]
    if "type" in msg:
        return msg["type"]
    return "unknown"


def is_compaction_message(msg):
    """Check if a message is a compaction message."""
    info = msg.get("info")
    if isinstance(info, dict):
        if info.get("mode") == "compaction" and info.get("agent") == "compaction":
            return True
    if msg.get("type") == "compaction" or msg.get("role") == "compaction":
        return True
    return False


def format_tool_call(part):
    tool_name = part.get("name") or part.get("tool") or "unknown"
    call_id = part.get("id") or part.get("callID") or ""
    if call_id.startswith("call_"):
        call_id = call_id[5:]
    call_id = call_id[:8]

    state = part.get("state", {})
    status = state.get("status", "unknown")
    input_data = state.get("input", {})
    error = state.get("error")

    output_str = ""
    if error:
        if isinstance(error, dict):
            err_msg = error.get("message") or json.dumps(error, indent=2)
            output_str = f"ERROR: {err_msg}"
        else:
            output_str = f"ERROR: {error}"
    elif "output" in state and state["output"] is not None:
        output_data = state["output"]
        if isinstance(output_data, (dict, list)):
            output_str = json.dumps(output_data, indent=2)
        else:
            output_str = str(output_data)
    elif "content" in state and state["content"] is not None:
        content_list = state["content"]
        if isinstance(content_list, list):
            text_parts = []
            for item in content_list:
                if isinstance(item, dict):
                    if item.get("type") == "text" and "text" in item:
                        text_parts.append(item["text"])
                    else:
                        text_parts.append(json.dumps(item, indent=2))
                elif isinstance(item, str):
                    text_parts.append(item)
            output_str = "\n".join(text_parts)
        elif isinstance(content_list, dict):
            output_str = json.dumps(content_list, indent=2)
        else:
            output_str = str(content_list)
    else:
        output_str = "(no output)"

    if not output_str:
        output_str = "(no output)"

    time_info = part.get("time") or state.get("time") or {}
    start = time_info.get("start") or time_info.get("ran") or time_info.get("created")
    end = time_info.get("end") or time_info.get("completed")
    time_str = ""
    if start and end:
        duration = end - start
        time_str = f" ({duration}ms)"

    input_str = json.dumps(input_data, indent=2) if input_data else ""

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
<summary class="tool-summary">🔧 {html.escape(tool_name)} [{html.escape(status)}]{time_str} <span class="call-id">#{html.escape(call_id)}</span></summary>
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


def extract_activities(msg):
    """Extract activity groups and regular content from a message."""
    regular_parts = []
    activity_groups = []
    current_activity = []

    parts = []
    if "parts" in msg and isinstance(msg["parts"], list):
        parts = msg["parts"]
    elif "content" in msg:
        if isinstance(msg["content"], list):
            parts = msg["content"]
        elif isinstance(msg["content"], str):
            regular_parts.append(msg["content"])

    for p in parts:
        if isinstance(p, dict):
            ptype = p.get("type", "text")
            if ptype in ("reasoning", "thought"):
                text = p.get("text", "") or p.get("reasoning", "")
                if text:
                    current_activity.append(("reasoning", text))
            elif ptype in ("tool", "tool-call", "tool_use"):
                current_activity.append(("tool", p))
            elif ptype == "text":
                text = p.get("text", "")
                if text:
                    if current_activity:
                        activity_groups.append(current_activity)
                        current_activity = []
                    regular_parts.append(text)
            else:
                text = p.get("text", "")
                if text:
                    if current_activity:
                        activity_groups.append(current_activity)
                        current_activity = []
                    regular_parts.append(text)
        elif isinstance(p, str):
            if current_activity:
                activity_groups.append(current_activity)
                current_activity = []
            regular_parts.append(p)

    if current_activity:
        activity_groups.append(current_activity)

    if "text" in msg and isinstance(msg["text"], str) and msg["text"]:
        if msg["text"] not in regular_parts:
            regular_parts.append(msg["text"])

    return regular_parts, activity_groups


def has_activities(activity_groups):
    """Check if activity groups contain any reasoning or tool calls."""
    for activity in activity_groups:
        for item_type, _ in activity:
            if item_type in ("reasoning", "tool"):
                return True
    return False


page_title = f"OpenCode Session: {session_title}" if session_title else "OpenCode Session Export"
header_title = f"OpenCode Session Transcript: {html.escape(session_title)}" if session_title else "OpenCode Session Transcript"

html_parts = [
    "<!DOCTYPE html><html><head><meta charset='utf-8'>",
    f"<title>{html.escape(page_title)}</title>",
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
    f"</style></head><body><h1>{header_title}</h1>"
]


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
        content = "\n\n".join(regular_parts)
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
            plural = "s" if tool_count > 1 else ""
            label_parts.append(f"{tool_count} tool call{plural}")
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
    role = get_message_role(msg)
    timestamp = get_message_timestamp(msg)
    compaction = is_compaction_message(msg)

    reg_test, act_test = extract_activities(msg)
    if role == "idle" and not reg_test and not has_activities(act_test):
        i += 1
        continue

    if role == "assistant" and not compaction:
        combined_regular = []
        combined_activities = []
        combined_timestamp = timestamp
        j = i
        while j < len(messages):
            next_msg = messages[j]
            next_role = get_message_role(next_msg)
            next_reg, next_act = extract_activities(next_msg)
            if next_role == "idle" and not next_reg and not has_activities(next_act):
                j += 1
                continue
            next_compaction = is_compaction_message(next_msg)

            if next_role == "assistant" and not next_compaction:
                combined_regular.extend(next_reg)
                combined_activities.extend(next_act)
                next_ts = get_message_timestamp(next_msg)
                if next_ts and (not combined_timestamp or next_ts < combined_timestamp):
                    combined_timestamp = next_ts
                j += 1
            else:
                break

        render_message(msg, role, combined_regular, combined_activities, combined_timestamp)
        i = j
    else:
        regular_parts, activity_groups = extract_activities(msg)
        render_message(msg, role, regular_parts, activity_groups, timestamp)
        i += 1

html_parts.append("</body></html>")

with open(out_path, "w", encoding="utf-8") as f:
    f.write("\n".join(html_parts))

print(f"Rendered to {out_path}")
