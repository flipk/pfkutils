#if 0
set -e -x
g++ -Wall -Werror laplot.cc -o laplot.exe
./laplot.exe
rm -f laplot.exe
exit 0
#endif

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cmath>

// Converts a hex string to a vector of 64 integers (0s and 1s), MSB at index 0
std::vector<int> hex_to_bits(const std::string& hex_str)
{
    // Convert hex string to unsigned long long
    unsigned long long value = std::stoull(hex_str, nullptr, 16);
    
    std::vector<int> bits(64);
    // Extract bits such that index 0 corresponds to the MSB (Bit 63)
    // mirroring the Python f"{value:064b}" behavior
    for(int i = 0; i < 64; ++i) {
        bits[i] = (value >> (63 - i)) & 1;
    }
    return bits;
}

// Generates a 2-line ASCII string for a digital waveform
std::string draw_wave(const std::vector<int>& bits, const std::string& label)
{
    std::string top_line = "";
    std::string bot_line = "";

    int prev = bits[0];

    // Initial state
    if (prev == 1) {
        top_line += " ";
        bot_line += "'"; // Start high
    } else {
        top_line += " ";
        bot_line += "_"; // Start low
    }

    // Loop through bits skipping the first one (already handled)
    for (size_t i = 1; i < bits.size(); ++i) {
        int bit = bits[i];

        if (bit == 1 && prev == 1) {
            top_line += "_"; // Continue high
            bot_line += " ";
        } else if (bit == 0 && prev == 0) {
            top_line += " ";
            bot_line += "_"; // Continue low
        } else if (bit == 1 && prev == 0) {
            top_line += " ";
            bot_line += "|"; // Rising edge
        } else if (bit == 0 && prev == 1) {
            top_line += "_"; // Falling edge top part
            bot_line += "|"; // Falling edge
        }

        prev = bit;
    }

    // Final extension
    if (prev == 1) {
        top_line += "_";
        bot_line += " ";
    } else {
        top_line += " ";
        bot_line += "_";
    }

    std::ostringstream oss;
    oss << label << " High: " << top_line << "\n" << label << " Low : " << bot_line;
    return oss.str();
}

struct EdgeInfo
{
    int index;
    std::string type;
    int offset_bits;
    double offset_time;
};

void analyze_logic_data(const std::string& hex_sig1, const std::string& hex_sig2)
{
    double period_ns = 1.0 / (312.5 * 1e6) * 1e9; // ~3.2 ns

    std::vector<int> bits1 = hex_to_bits(hex_sig1);
    std::vector<int> bits2 = hex_to_bits(hex_sig2);

    // Visual Header
    std::cout << "Logic Analyzer Capture @ 312.5MHz (T=" << period_ns << "ns)\n";
    std::cout << std::string(60, '-') << "\n";

    // Draw Ruler
    std::string ruler = "            "; // Padding for label (12 chars)
    for (int i = 0; i < 64; ++i)
    {
        if (i == 32)
        {
            ruler += "T"; // Trigger
        }
        else if (i % 8 == 0)
        {
            ruler += "|";
        }
        else
        {
            ruler += ".";
        }
    }
    std::cout << ruler << "\n";

    // Draw Signals
    std::cout << draw_wave(bits1, "SIG 1") << "\n";
    std::cout << draw_wave(bits2, "SIG 2") << "\n";
    std::cout << std::string(60, '-') << "\n";

    // Analyze Edges on Signal 2
    std::cout << "Signal 2 Edge Analysis (Relative to Trigger @ Bit 32):\n";

    std::vector<EdgeInfo> edges;
    int prev = bits2[0];

    for (int i = 1; i < 64; ++i)
    {
        int curr = bits2[i];
        if (curr != prev)
        {
            std::string edge_type = (curr == 1) ? "Rising " : "Falling";
            int offset_bits = i - 32;
            double offset_time = offset_bits * period_ns;
            edges.push_back({i, edge_type, offset_bits, offset_time});
        }
        prev = curr;
    }

    if (edges.empty())
    {
        std::cout << "No edges detected on Signal 2.\n";
    }
    else
    {
        // Table Header
        std::cout << std::left << std::setw(10) << "Index" 
                  << std::setw(10) << "Type" 
                  << std::setw(15) << "Offset (Bits)" 
                  << std::setw(15) << "Time (ns)" << "\n";

        // Table Rows
        for (const auto& e : edges) {
            std::cout << std::left << std::setw(10) << e.index 
                      << std::setw(10) << e.type 
                      << std::setw(15) << e.offset_bits 
                      << std::showpos << std::fixed << std::setprecision(1) 
                      << e.offset_time << "ns" 
                      << std::noshowpos << "\n"; // Reset format flags
        }
    }
}

int main()
{
    std::string hex1 = "00000000ffffffff";
    std::string hex2 = "0007fff80007fff0";

    analyze_logic_data(hex1, hex2);

    return 0;
}
