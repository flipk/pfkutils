
//
// sample CSV data that we should support parsing someday:
//
// this is a test,“this is a test”,one,1.4,1.5,2.9
// "this is a multiline
// Cell","this is a multiline
// Cell with “quotes” in it",“quotes”,1,1,
// first cell,"cell with ""straight quotes"" in it",,,,
//
// it appears multiline cells need straight quotes (ascii 0x22)
// and if the cell contains straight quotes, they are double quoted.
// but normally a cell with quotes use the rounded quotes which
// don't require straight quotes.
//

#include <string>
#include <vector>
#include <fstream>

int LineParser(std::vector<std::string> &fields,
                const std::string &line,
                char delimiter)
{
    fields.clear();
    size_t pos = 0, spacepos;
    while (1)
    {
        if (line[pos] == '(') // todo : handle quotes
        {
            size_t firstcharpos = line.find_first_not_of('(',pos);
            if (firstcharpos == std::string::npos)
                // buh? this line sucks.
                return -1;
            size_t parenpos = line.find_first_of(')',firstcharpos);
            if (parenpos == std::string::npos)
                // this line sucks some other way.
                return -1;
            spacepos = line.find_first_of(delimiter,parenpos+1);
            fields.push_back(line.substr(firstcharpos,
                                         parenpos-firstcharpos));
        }
        else
        {
            spacepos = line.find_first_of(delimiter,pos);
            if (spacepos == std::string::npos)
                fields.push_back(line.substr(pos));
            else
                fields.push_back(line.substr(pos,spacepos-pos));
        }
        if (spacepos == std::string::npos)
            break;
        pos = spacepos+1;
    }
    return fields.size();
}

int FileParser(std::vector<std::vector<std::string> > &lines,
               const std::string &filename,
               char delimiter)
{
    std::ifstream  ifs;

    ifs.open(filename.c_str());
    if (ifs.fail())
        return -1;

    std::string  l;

    while (ifs.good())
    {
        std::getline(ifs, l);
        if (!ifs.good())
            break;
        std::vector<std::string>  fields;
        if (LineParser(fields, l, delimiter) < 0)
            return -1;
        lines.push_back(fields);
    }
}

#ifdef __TEST_LINEPARSER__
#include <stdio.h>

int
main()
{
    std::vector<std::vector<std::string> > lines;

    if (FileParser(lines, "junk.csv", ',') < 0)
        printf("ERROR\n");

    for (int ind = 0; ind < lines.size(); ind++)
    {
        std::vector<std::string> &line = lines[ind];
        printf("line %d :", ind);
        for (int ind2 = 0; ind2 < line.size(); ind2++)
        {
            printf(" %d(%s)", ind2, line[ind2].c_str());
        }
        printf("\n");
    }

    return 0;
}
#endif
