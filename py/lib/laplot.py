import math

def hex_to_bits(hex_str):
    """Converts a hex string to a list of 64 integers (0s and 1s)."""
    # Convert hex to integer, then formatting to 64-bit binary string
    value = int(hex_str, 16)
    binary_string = f"{value:064b}"
    return [int(b) for b in binary_string]

def draw_wave(bits, label):
    """Generates a 2-line ASCII string for a digital waveform."""
    top_line = ""
    bot_line = ""

    prev = bits[0]

    # Initial state
    if prev == 1:
        top_line += " "
        bot_line += "'" # Start high
    else:
        top_line += " "
        bot_line += "_" # Start low

    for i, bit in enumerate(bits):
        if i == 0: continue # Skip first, already handled start

        if bit == 1 and prev == 1:
            top_line += "_" # Continue high
            bot_line += " "
        elif bit == 0 and prev == 0:
            top_line += " "
            bot_line += "_" # Continue low
        elif bit == 1 and prev == 0:
            top_line += " "
            bot_line += "|" # Rising edge
        elif bit == 0 and prev == 1:
            top_line += "_" # Falling edge top part (optional, looks better without)
            bot_line += "|" # Falling edge

        prev = bit

    # Final extension to make it look complete
    if prev == 1:
        top_line += "_"
        bot_line += " "
    else:
        top_line += " "
        bot_line += "_"

    return f"{label} High: {top_line}\n{label} Low : {bot_line}"

def analyze_logic_data(hex_sig1, hex_sig2, sample_rate_mhz):
    """
    Parses two 64-bit hex strings and displays logic analyzer traces.
    Calculates edge offsets relative to the center trigger (bit 32).
    """
    period_ns = 1 / (sample_rate_mhz * 1e6) * 1e9 # 3.2 ns

    bits1 = hex_to_bits(hex_sig1)
    bits2 = hex_to_bits(hex_sig2)

    # Visual Header
    print(f"Logic Analyzer Capture @ {sample_rate_mhz}MHz (T={period_ns}ns)")
    print("-" * 60)

    # Draw Ruler marking the trigger point (Bit 32)
    # The string length grows by 1 char per bit roughly,
    # but exact alignment depends on the draw_wave function.
    # Our draw_wave adds 1 char per bit.

    ruler = " " * 12 # Padding for label
    for i in range(64):
        if i == 32:
            ruler += "T" # Trigger
        elif i % 8 == 0:
            ruler += "|"
        else:
            ruler += "."
    print(ruler)

    # Draw Signals
    print(draw_wave(bits1, "SIG 1"))
    print(draw_wave(bits2, "SIG 2"))
    print("-" * 60)

    # Analyze Edges on Signal 2 relative to Trigger (Bit 32)
    print("Signal 2 Edge Analysis (Relative to Trigger @ Bit 32):")

    edges = []
    prev = bits2[0]
    for i in range(1, 64):
        curr = bits2[i]
        if curr != prev:
            edge_type = "Rising " if curr == 1 else "Falling"
            offset_bits = i - 32
            offset_time = offset_bits * period_ns
            edges.append((i, edge_type, offset_bits, offset_time))
        prev = curr

    if not edges:
        print("No edges detected on Signal 2.")
    else:
        print(f"{'Index':<10} {'Type':<10} {'Offset (Bits)':<15} {'Time (ns)':<15}")
        for idx, etype, obit, otime in edges:
            print(f"{idx:<10} {etype:<10} {obit:<15} {otime:+.1f}ns")

def test_laplot():
    # Example Usage
    # sample rate = 312.5 MHz (3.2 nS per tick)
    # Signal 1: Transition at 32
    # Signal 2: A pulse train
    hex1 = "00000000ffffffff"
    hex2 = "0007fff80007fff0"
    analyze_logic_data(hex1, hex2, 312.5)

if __name__ == '__main__':
    test_laplot()
