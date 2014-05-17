/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include "passwordDatabase.h"

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

PasswordDatabase :: PasswordDatabase(void)
{
    ifstream  ifs(DB_FILE, ios::in);
    if (ifs.is_open())
    {
        string  id, uname, passwd, tok;
        while (1)
        {
            ifs >> id >> uname >> passwd >> tok;
            if (!ifs.good())
                break;
            PasswordEntry * ent = new PasswordEntry;
            istringstream( id ) >> ent->id;
            ent->username = uname;
            ent->password = passwd;
            ent->token = tok;
            database.push_back(ent);
        }
    }
    sync();
}

PasswordDatabase :: ~PasswordDatabase(void)
{
    sync();
}

PasswordEntry *
PasswordDatabase :: lookupUser( std::string username )
{
    list<PasswordEntry*>::iterator   it;
    for (it = database.begin(); it != database.end(); it++)
    {
        PasswordEntry * ent = *it;
        if (ent->username == username)
            return ent;
    }
    return NULL;
}

static const char tokenSet[] = 
    "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

void
PasswordDatabase :: newToken(PasswordEntry *ent)
{
    string token;
    int c;

    for (c = 0; c < tokenLength; c++)
        token += tokenSet[random() % (sizeof(tokenSet)-1)];
    ent->token = token;
    sync();
}

int
PasswordDatabase :: uniqueIdentifier(void)
{
    int newid;
    bool found = true;

    while (found == true)
    {
        found = false;
        newid = random() % 16384;
        list<PasswordEntry*>::iterator  it;
        for (it = database.begin(); it != database.end(); it++)
        {
            PasswordEntry * ent = *it;
            if (ent->id == newid)
            {
                found = true;
                break;
            }
        }
    }

    return newid;
}

PasswordEntry *
PasswordDatabase :: addUser( std::string username, std::string password )
{
    PasswordEntry * ent;

    ent = new PasswordEntry;
    ent->id = uniqueIdentifier();
    ent->username = username;
    ent->password = password;
    database.push_back(ent);
    newToken(ent);

    return ent;
}

void
PasswordDatabase :: sync(void)
{
    ofstream  ofs(DB_FILE, ios::out | ios::trunc);
    list<PasswordEntry*>::iterator  it;
    system("rm -f /home/web/cookies/*");
    for (it = database.begin(); it != database.end(); it++)
    {
        PasswordEntry * ent = *it;
        ofs
            << ent->id << " "
            << ent->username << " "
            << ent->password << " "
            << ent->token    << endl;
        if (ent->token != "__INVALID__")
        {
            string  cmdstring;
            cmdstring = "touch /home/web/cookies/";
            cmdstring += ent->token;
            system(cmdstring.c_str());
        }
    }
}
