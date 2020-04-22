
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

using namespace std;

bool enable_line_numbers = true;

bool in_template = false;
int template_line_no;
string template_file_name;
string template_name;
string template_body;

#define BOUNDARY "________"
#define BOUNDARY_LEN 8

#define OPTION_LINE "______OPTION"
#define OPTION_LENGTH 12

#ifndef ECHO_OUTPUT
#define ECHO_OUTPUT 0
#endif

struct at_info {
    int pos;
    int line_no;
    at_info(int _p, int _l) { pos = _p; line_no = _l; }
};
vector<at_info> ats;
struct pattern_info {
    int line_no;
    int start_pos;
    int end_pos;
    string  pattern;
};
vector<pattern_info> patterns;
map<string,string> unique_patterns;

string
escape(const string &in, int line_no)
{
    bool do_line_no = (line_no > 0) && enable_line_numbers;
    bool did_line_no = false;
    string out;
    for (int ind = 0; ind < (int)in.length(); ind++)
    {
        if (in[ind] == '"')
        {
            out += "\\\"";
            continue;
        }
        else if (in[ind] == 10)
        {
            line_no ++;
            out += "\\n\"\n";
            if (do_line_no)
            {
                if (!did_line_no)
                {
#ifdef TRUE_LINE_NUMBERS
                    ostringstream os;
                    os << "        << \""
                       << "#line " << line_no << " \\\""
                       << template_file_name << "\\\"\\n\"\n";
                    out += os.str();
#endif
                    did_line_no = true;
                }
            }
            out += "        << \"";
            continue;
        }
        else if (in[ind] == '\\')
        {
            out += "\\\\";
            continue;
        }
        // else
        out += in[ind];
    }
    return out;
}

void
add_template(ostream &cc_out, ostream &h_out,
             const string &name, const string &body, int in_line_no)
{
    int line_no = in_line_no;
    if (ECHO_OUTPUT)
        fprintf(stderr,
                "template '%s' of length %d from file '%s' line %d\n",
                name.c_str(), (int) body.length(),
                template_file_name.c_str(), line_no);

    // now to identify all the @@patterns@@
    for (int ind = 0; ind < (int) body.length() - 2; ind++)
    {
        char c = body[ind];
        if (c == '@' && body[ind+1] == '@')
            ats.push_back(at_info(ind++, line_no));
        if (c == 10)
            line_no++;
    }

    if (ECHO_OUTPUT)
        for (int ind = 0; ind < (int) ats.size(); ind++)
            printf("ats[%d] = pos %d line %d\n", ind,
                   ats[ind].pos, ats[ind].line_no);

    for (int ind = 0; ind < (int) ats.size(); ind += 2)
    {
        pattern_info pi;
        pi.line_no = (in_line_no > 0) ? ats[ind].line_no : -1;
        pi.start_pos = ats[ind].pos;
        pi.end_pos = ats[ind+1].pos;
        pi.pattern = body.substr(pi.start_pos + 2,
                                 pi.end_pos - pi.start_pos - 2);
        unique_patterns[pi.pattern] = 1;
        patterns.push_back(pi);
    }

    if (ECHO_OUTPUT)
        for (int ind = 0; ind < (int) patterns.size(); ind++)
        {
            pattern_info &pi = patterns[ind];
            fprintf(stderr, "  pattern: [%d-%d] line %d %s\n",
                    pi.start_pos, pi.end_pos, pi.line_no,
                    pi.pattern.c_str());
        }

    cc_out << "\n"
           << "void output_" << name
           << "(std::ostream &out, const pattern_value_map &pattern_values)\n"
           << "{\n";

#ifdef TRUE_LINE_NUMBERS
    if (in_line_no != -1 && enable_line_numbers)
    {
        cc_out << "    out << \"#line " << in_line_no << " \\\""
               << template_file_name << "\\\"\\n\";\n";
    }
#endif

    h_out << "// " << template_file_name << " line " << in_line_no << "\n"
          << "// must define patterns:\n";

    map<string,string>::iterator it;
    for (it = unique_patterns.begin();
         it != unique_patterns.end();
         it++)
    {
        h_out << "//     " << it->first << "\n";
    }

    h_out << "void output_" << name << "(\n"
          << "    std::ostream &out, \n"
          << "    const pattern_value_map &pattern_values);\n"
          << "\n";

    string piece;
    int last_pos = 0;
    int last_lineno = -1;
    for (int ind = 0; ind < (int) patterns.size(); ind++)
    {
        pattern_info &pi = patterns[ind];
        piece = body.substr(last_pos, pi.start_pos - last_pos);
        cc_out << "    out << \"" << escape(piece, last_lineno) << "\";\n";
        cc_out << "    out << find_pattern(pattern_values, \""
               << pi.pattern << "\", \"" << name << "\");\n";
        last_pos = pi.end_pos + 2;
        last_lineno = pi.line_no;
    }
    piece = body.substr(last_pos);
    cc_out << "    out << \"" << escape(piece, -1) << "\";\n"
           << "}\n";

    ats.clear();
    patterns.clear();
    unique_patterns.clear();
}

void
process_line(ostream &cc_out, ostream &h_out,
             const string &line, int line_no)
{
    if (in_template == false)
    {
        if (memcmp(line.c_str(), OPTION_LINE, OPTION_LENGTH) == 0)
        {
            if (memcmp(line.c_str() + OPTION_LENGTH + 1,
                       "disable_lineno", 14) == 0)
            {
                enable_line_numbers = false;
            }
        }
        else if (memcmp(line.c_str(), BOUNDARY, BOUNDARY_LEN) == 0)
        {
            in_template = true;
            template_body.clear();
            template_name = line.substr(BOUNDARY_LEN + 1);
            template_line_no = line_no + 1;
        }
        return;
    }
    // else
    if (memcmp(line.c_str(), BOUNDARY, BOUNDARY_LEN) == 0)
    {
        in_template = false;
        if (line.substr(BOUNDARY_LEN + 1, 3) != "END")
        {
            cerr << "ERROR 'END' != '"
                 << line.substr(BOUNDARY_LEN + 1)
                 << "'\n";
            exit(1);
        }
        if (line.substr(BOUNDARY_LEN + 1, 5) == "ENDNL")
        {
            template_body.resize(template_body.size()-1);
            // disable line numbers for this template,
            // because if they're using ENDNL, the #line directive
            // would just screw up the text.
            template_line_no = -1;
        }
        add_template(cc_out, h_out, template_name, template_body,
                     template_line_no);
        return;
    }
    template_body += line;
    template_body += "\n";
}

void print_line(const string &line)
{
    fprintf(stderr, "length : %d\n", (int) line.length());
    unsigned char * ptr = (unsigned char *) line.c_str();
    fprintf(stderr, "contents: ");
    for (int ind = 0; ind < (int) line.length(); ind++)
        fprintf(stderr, "%02x ", ptr[ind]);
    fprintf(stderr, "\n");
}

int
main(int argc, char ** argv)
{
    if (argc != 4)
    {
        fprintf(stderr, "usage: template_to_c templ file.cc file.h\n");
        return 1;
    }

    template_file_name = argv[1];

    ifstream tmpl_in(argv[1]);
    ofstream cc_out(argv[2]);
    ofstream  h_out(argv[3]);

    if (tmpl_in.fail())
    {
        fprintf(stderr, "FAIL : cannot open template '%s'\n",
                argv[1]);
        return 1;
    }

    cc_out << "\n"
           << "#include <iostream>\n"
           << "#include \"" << argv[3] << "\"\n"
           << "\n";

    h_out << "\n"
          << "#include <map>\n"
          << "#include <iostream>\n"
          << "#include \"../template_patterns.h\"\n"
          << "\n";

    int line_no = 1;
    string  line;
    while (1)
    {
        line.resize(300);
        tmpl_in.getline((char*)line.c_str(), line.length());
        if (!tmpl_in.good())
            break;
        line.resize(tmpl_in.gcount()-1);
        process_line(cc_out, h_out, line, line_no);
//        print_line(line);
        line_no ++;
    }

    return 0;
}
