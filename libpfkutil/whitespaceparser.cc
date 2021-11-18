
#include <string>
#include <vector>
#include <sstream>

void parseString(std::vector<std::string> &tokens,
                 std::string &input, char delimiter)
{
    std::istringstream parser(input);
    std::string token;
    tokens.clear();
    while (std::getline(parser,token,delimiter))
        if (token.length() > 0)
            tokens.push_back(token);
}

int main(int argc, char ** argv)
{
    if (argc == 2)
    {
        std::string in(argv[1]);
        std::vector<std::string> tokens;

        parseString(tokens, in, ' ');

        for (auto t : tokens)
            printf("token : %s\n", t.c_str());
    }

    return 0;
}
