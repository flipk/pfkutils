
#include "handlers.h"
#include "simple_json.h"

myHandlers :: myHandlers( xxxx )
{
}

myHandlers :: ~myHandlers()
{
}

void
myHandlers :: register_handlers(TestHttpServer &svr)
{
    svr.register_get_handler ( "/status",       this );
    svr.register_get_handler ( "/xxxx",    this );
    svr.register_post_handler( "/xxxxxx", this );
}

// get handler
/*virtual*/ bool
myHandlers :: generate_data(const std::string &path,
                            const std::string &rest,
                            std::string &data)
{
    bool ret = false;

    if (path == "/status")
    {
        SimpleJson::ObjectProperty  o("");
        o.push_back(new SimpleJson::StringProperty("status","connected"));
        std::ostringstream ostr;
        ostr << &o;
        data = ostr.str();
        ret = true;
    }
    else if (path == "/xxxx")
    {
    }

    printf("GET handler path '%s' rest '%s' data '%s'\n",
           path.c_str(), rest.c_str(), data.c_str());

    return ret;
}

// post handler
/*virtual*/ bool
myHandlers :: handle_data(const std::string &path,
                          const std::string &rest,
                          const std::string &data)
{
    bool ret = false;

    printf("POST handler path '%s' rest '%s' data '%s'\n",
           path.c_str(), rest.c_str(), data.c_str());

    if (data.size() == 0)
    {
        printf("ZERO LENGTH DATA SKIPPING \n");
        return false;
    }

    if (path == "/xxxxx")
    {
        SimpleJson::Property * p = SimpleJson::parseJson(data);
        if (p)
        {
            //xxxxx
            delete p;
        }
        else
            printf("FAILURE parsing JSON\n");
    }

    return ret;
}
