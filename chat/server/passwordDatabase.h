/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*-  */

#ifndef __PASSWORD_DATABASE_H__
#define __PASSWORD_DATABASE_H__ 1

#include <time.h>
#include <string>
#include <list>

#define DB_FILE "/home/nginx/pfkchat_password_database"

struct PasswordEntry {
    int id;
    std::string username;
    std::string password;
    std::string token;
//     time_t expire_time; not yet supported
};

class PasswordDatabase {
    static const int tokenLength = 32;
    std::list<PasswordEntry*> database;
    int uniqueIdentifier(void);
public:
    PasswordDatabase(void);
    ~PasswordDatabase(void);
    void sync(void);
    PasswordEntry * lookupUser( std::string username );
    void newToken(PasswordEntry *);
    PasswordEntry * addUser( std::string username, std::string password );
};

#endif /* __PASSWORD_DATABASE_H__ */
