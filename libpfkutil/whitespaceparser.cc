
vector<string> parseString(string input, char delimiter)
{
    stringstream parser(input);
    string token;
    vector<string> tokens;
    while (getline(parser,token,delimiter))
        tokens.push_back(token);
    return tokens;
}
