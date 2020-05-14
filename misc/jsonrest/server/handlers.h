
#include "test_http_server.h"

class myHandlers : public TestHttpGetHandler,
                   public TestHttpPostHandler
{
    xxx;

    // get handler
    /*virtual*/ bool generate_data(const std::string &path,
                                   const std::string &rest,
                                   std::string &data);
    // post handler
    /*virtual*/ bool handle_data(const std::string &path,
                                 const std::string &rest,
                                 const std::string &data);
public:
    myHandlers( xxxx );
    ~myHandlers(void);
    void register_handlers(TestHttpServer &svr);
};
