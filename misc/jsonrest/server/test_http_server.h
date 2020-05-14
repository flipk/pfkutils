#ifndef __TEST_HTTP_SERVER_H__
#define __TEST_HTTP_SERVER_H__ 1

#include <string>
#include <map>

class TestHttpGetHandler
{
public:
    TestHttpGetHandler(void) { };
    virtual ~TestHttpGetHandler(void) { }
    virtual bool generate_data(const std::string &path,
                               const std::string &rest,
                               std::string &data) = 0;
};

class TestHttpPostHandler
{
public:
    TestHttpPostHandler(void) { };
    virtual ~TestHttpPostHandler(void) { }
    virtual bool handle_data(const std::string &path,
                             const std::string &rest,
                             const std::string &data) = 0;
};

class TestHttpServer
{
    int fd;
    std::map<std::string,TestHttpGetHandler *> getHandlers;
    std::map<std::string,TestHttpPostHandler *> postHandlers;
    TestHttpGetHandler *findGetHandler(const std::string &path);
    TestHttpPostHandler *findPostHandler(const std::string &path);
public:
    TestHttpServer(int port);
    ~TestHttpServer(void);
    void register_get_handler(const std::string &path,
                              TestHttpGetHandler *hand);
    void register_post_handler(const std::string &path,
                               TestHttpPostHandler *hand);
    void run(void);
};

#endif /* __TEST_HTTP_SERVER_H__ */
