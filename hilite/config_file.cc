
#include "config_file.h"
#include <stdio.h>
#include <fstream>

using namespace std;

// each line of the config file should obey this pattern.
static const char * config_parse_expr =
    // matches[0]: wholestring

#define LINE_OF_BLANKS 1
    // matches[1]: a line of blanks
    "(^[ \t]*$)|"

#define COMMENT_LINE 2
    // matches[2]: a comment line
    "(^[ \t]*#.*$)|"

    // matches[3]: a regex rule (whole line)
    "(^"
    /**/"[ \t]*"
#define REGEXP_PATTERN 4
    // matches[4]: the regex part
    /**/"\"(.+)\""
    /**/"[ \t]+"
#define FIRST_COLOR_WORD 5
    // matches[5]: first color word
    /**/"([a-z]+)"
#define SECOND_COLOR_WORD 7
    // matches[6]: space plus next color word (ignore)
    // matches[7]: color word
    /**/"([ \t]+([a-z]+))?"
#define THIRD_COLOR_WORD 9
    // matches[8]: space plus next color word (ignore)
    // matches[9]: color word
    /**/"([ \t]+([a-z]+))?"
#define FOURTH_COLOR_WORD 11
    // matches[10]: space plus next color word (ignore)
    // matches[11]: color word
    /**/"([ \t]+([a-z]+))?"
    "[ \t]*$)";
;


Hilite_config :: Hilite_config(void)
{
    int regerr = regcomp( &config_file_expr, config_parse_expr, REG_EXTENDED );
    if ( regerr != 0 )
    {
        char errbuf[80];
        regerror( regerr, &config_file_expr, errbuf, sizeof(errbuf));
    }
}

Hilite_config :: ~Hilite_config(void)
{
    regfree(&config_file_expr);
    while (!patterns.empty())
    {
        pattern * p = patterns.front();
        patterns.pop_front();
        delete p;
    }
}

bool
Hilite_config :: parse_file(const std::string &fname)
{
    ifstream inf(fname.c_str());
    if (inf.is_open() == false)
    {
        printf("unable to open file %s\n", fname.c_str());
        return false;
    }
    string line;
    int lineno = 1;
    while (1)
    {
        getline(inf, line);
        if (!inf.good())
            break;
        if (parse_line(line,lineno) == false)
            return false;
        lineno++;
    }

    return true;
}

bool
Hilite_config :: parse_line(const std::string &line, int lineno)
{
    regmatch_t matches[ MAX_MATCHES ];
    int regerr;

    regerr = regexec( &config_file_expr, line.c_str(),
                      MAX_MATCHES, matches, 0 );
    if ( regerr != 0 )
    {
        char errbuf[80];
        regerror( regerr, &config_file_expr, errbuf, sizeof(errbuf));
        return false;
    }

#define MATCH(n) (matches[(n)].rm_so != -1)
#define MATCHSTR(str,n) \
    str.substr(matches[(n)].rm_so, \
               matches[(n)].rm_eo - matches[(n)].rm_so)

    if (MATCH(LINE_OF_BLANKS))
    {
        // blank, skip
    }
    else if (MATCH(COMMENT_LINE))
    {
        // comments
    }
    else if (MATCH(REGEXP_PATTERN))
    {
        pattern * p = new pattern;
        p->regex = MATCHSTR(line,REGEXP_PATTERN);
        bool fail = false;
        if (!p->set_expr(lineno, p->regex))
            fail = true;
        if (!fail && !p->color.set_color(MATCHSTR(line,FIRST_COLOR_WORD)))
            fail = true;
        if (!fail && MATCH(SECOND_COLOR_WORD) &&
            !p->color.set_modifier(MATCHSTR(line,SECOND_COLOR_WORD)))
            fail = true;
        if (!fail && MATCH(THIRD_COLOR_WORD) &&
            !p->color.set_modifier(MATCHSTR(line,THIRD_COLOR_WORD)))
            fail = true;
        if (!fail && MATCH(FOURTH_COLOR_WORD) &&
            !p->color.set_modifier(MATCHSTR(line,FOURTH_COLOR_WORD)))
            fail = true;
        if (fail)
            delete p;
        else
        {
            p->color.finalize();
//            p->print();
            patterns.push_back(p);
        }
    }
    return true;
}

Hilite_config :: pattern :: pattern( void )
{
    config_lineno = -1;
}

Hilite_config :: pattern :: ~pattern( void )
{
    if (config_lineno != -1)
        regfree(&expr);
}

bool
Hilite_config :: pattern :: set_expr( int _lineno, const std::string &regex )
{
    config_lineno = _lineno;
    int regerr = regcomp(&expr, regex.c_str(), REG_EXTENDED);
    if (regerr != 0)
    {
        char errbuf[80];
        regerror(regerr, &expr, errbuf, sizeof(errbuf));
        printf("config line %d : regcomp : %s\n", config_lineno, errbuf);
        config_lineno = -1;
        return false;
    }
    return true;
}

void
Hilite_config :: pattern :: print(void)
{
    printf("%d : %s\"%s\"%s\n", config_lineno,
           color.color_string.c_str(),
           regex.c_str(),
           color.normal_color_string.c_str());
}

void
Hilite_config :: colorize_line(std::string &line, int lineno)
{
    std::list<pattern*>::iterator it;
    for (it = patterns.begin(); it != patterns.end(); it++)
    {
        pattern * p = *it;
        p->colorize_line(line, lineno);
    }
}

bool
Hilite_config :: pattern :: colorize_line(std::string &line, int file_lineno)
{
    regmatch_t matches[ MAX_MATCHES ];
    int regerr = regexec(&expr, line.c_str(), MAX_MATCHES, matches, 0);
    if (regerr == REG_NOMATCH)
        return false;
    if (regerr != 0)
    {
        char errbuf[80];
        regerror( regerr, &expr, errbuf, sizeof(errbuf));
        printf("regexec fail on pattern %d line %d : %s\n",
               config_lineno, file_lineno, errbuf);
        return false;
    }
    int so = matches[0].rm_so;
    int eo = matches[0].rm_eo;
    if (so == -1)
        return false; // no match
//    printf("pattern %d matches on line %d, %d->%d\n",
//           config_lineno, file_lineno, so, eo );
    line.insert(so, color.color_string);
    eo += color.color_string.length();
    line.insert(eo, color.normal_color_string);
    return true;
}
