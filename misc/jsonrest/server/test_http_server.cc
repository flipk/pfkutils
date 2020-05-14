
#include "test_http_server.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <vector>

using namespace std;

TestHttpServer :: TestHttpServer(int port)
{
    char err[100];
    int v = 1;
    fd = -1;
    int d = socket(AF_INET, SOCK_STREAM, 0);
    if (d < 0)
    {
        int e = errno;
        printf("unable to create socket: %d: %s\n", e,
               strerror_r(e, err, sizeof(err)));
        goto fail;
    }
    setsockopt( d, SOL_SOCKET, SO_REUSEADDR, (void*) &v, sizeof( v ));
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(d, (struct sockaddr *)&sa, sizeof(sa)) < 0)
    {
        int e = errno;
        printf("unable to bind to port %d: %d: %s\n",
               port, e, strerror_r(e, err, sizeof(err)));
        goto fail;
    }
    listen(d,1);
    fd = d;
    if (0) {
    fail:
        if (d > 0)
            close(d);
    }
}

TestHttpServer :: ~TestHttpServer(void)
{
    if (fd > 0)
        close(fd);
}

TestHttpGetHandler *
TestHttpServer :: findGetHandler(const std::string &path)
{
    std::map<std::string,TestHttpGetHandler *>::iterator it;
    it = getHandlers.find(path);
    if (it == getHandlers.end())
        return NULL;
    return it->second;
}

TestHttpPostHandler *
TestHttpServer :: findPostHandler(const std::string &path)
{
    std::map<std::string,TestHttpPostHandler *>::iterator it;
    it = postHandlers.find(path);
    if (it == postHandlers.end())
        return NULL;
    return it->second;
}

void
TestHttpServer :: register_get_handler(const std::string &path,
                                       TestHttpGetHandler *hand)
{
    getHandlers[path] = hand;
}

void
TestHttpServer :: register_post_handler(const std::string &path,
                                        TestHttpPostHandler *hand)
{
    postHandlers[path] = hand;
}

static void splitString( vector<string> &ret, const string &line )
{
    for (size_t pos = 0; ;)
    {
        size_t found = line.find_first_of("\t ",pos);
        if (found == string::npos)
        {
            if (pos != line.size())
                ret.push_back(line.substr(pos,line.size()-pos));
            break;
        }
        if (found > pos)
            ret.push_back(line.substr(pos,found-pos));
        pos = found+1;
    }
}

void
TestHttpServer :: run(void)
{
    while (1)
    {
        struct sockaddr_in sa;
        socklen_t salen = sizeof(sa);
        printf("waiting for connection.\n");
        int new_fd = accept(fd, (struct sockaddr *)&sa, &salen);
        if (new_fd < 0)
        {
            int e = errno;
            char err[100];
            printf("accept error: %d: %s\n", e,
                   strerror_r(e, err, sizeof(err)));
            continue;
        }

        // destructor guarantees closure if we forget.
        struct fdcloser {
            FILE *f;
            fdcloser(int newfd) { f = fdopen(newfd, "w+"); }
            ~fdcloser(void) { fclose(f); }
        };
        fdcloser fd(new_fd);

        string request;
        request.resize(2000);
        if (fgets((char*)request.c_str(), request.size(), fd.f) == NULL)
            continue;

        size_t nlpos = request.find_first_of("\r\n");
        if (nlpos != string::npos)
            request.resize(nlpos);
        else
            request.resize(strlen(request.c_str()));
        vector<string> reqSplit;
        splitString(reqSplit, request);
        if (reqSplit.size() != 3)
        {
            fprintf(fd.f, "HTTP/1.1 400 ERROR improperly formatted req\r\n");
            continue;
        }

        struct headersType : public map<string,string>
        {
            string *findHeader(const string &name) {
                map<string,string>::iterator it = find(name);
                if (it == end())
                    return NULL;
                return &it->second;
            }
        } headers;

        bool ok = false;
        while (1)
        {
            string line;
            line.resize(2000);
            if (fgets((char*)line.c_str(), line.size(), fd.f) == NULL)
                break;
            size_t nlpos = line.find_first_of("\r\n");
            if (nlpos != string::npos)
                line.resize(nlpos);
            else
                line.resize(strlen(line.c_str()));
            if (line.size() == 0)
            {
                ok = true;
                break;
            }
            size_t colonpos = line.find(": ");
            if (colonpos == string::npos)
            {
                fprintf(fd.f,
                        "HTTP/1.1 400 ERROR improperly "
                        "formatted header line '%s'\r\n",
                        line.c_str());
                break;
            }
            headers[line.substr(0,colonpos)] = line.substr(colonpos+2);
        }
        if (!ok)
        {
            fprintf(fd.f,
                    "HTTP/1.1 400 ERROR improperly formatted headers\r\n");
            continue;
        }

        string * expect_header = headers.findHeader("Expect");
        if (expect_header)
        {
            fprintf(fd.f, "HTTP/1.1 100 Continue\r\n\r\n");
            fflush(fd.f);
        }

        string contents;
        string * content_length = headers.findHeader("Content-Length");
        if (content_length)
        {
            size_t clen = (size_t) atoi(content_length->c_str());
            if (clen < 0 || clen > 50000000)
            {
                fprintf(fd.f,
                        "HTTP/1.1 400 ERROR HUGE\r\n");
                continue;
            }
            contents.resize(clen);
            size_t sz = fread((void*) contents.c_str(), 1, clen, fd.f);
            if (sz < clen)
            {
                fprintf(fd.f,
                        "HTTP/1.1 400 ERROR improperly formatted content\r\n");
                continue;
            }
        }

        string * chunked = headers.findHeader("Transfer-Encoding");
        if (chunked && chunked->compare("chunked") == 0)
        {
            string chunk;

// hex number (not zero)
// \r\n
// N bytes of binary
// \r\n
// to finish blowing chunks, hex number is 0

            char dummy[2]; // for swallowing \r\n
            while (1)
            {
                size_t chunk_size = 0;
                chunk.resize(128);
                if (fgets((char*) chunk.c_str(), chunk.size(), fd.f) == NULL)
                {
                    printf("closed??\n");
                    break;
                }
                printf("fgets chunk size part returns : '%s'\n",
                       chunk.c_str());
                char * endptr = NULL;
                chunk_size = (size_t) strtoul(chunk.c_str(), &endptr,
                                              /*base*/16);
                printf("got chunk size = %d\n", chunk_size);
                if (chunk_size == 0)
                    // done!
                    break;
                if (chunk_size < 0 || chunk_size > 500000)
                    break;
                chunk.resize(chunk_size);
                char * chunk_ptr = (char*) chunk.c_str();
                while (chunk_size > 0)
                {
                    size_t got = fread(chunk_ptr, 1,
                                       chunk_size, fd.f);
                    if (got <= 0)
                        break;
                    chunk_ptr += got;
                    chunk_size -= got;
                }
                contents += chunk;
                // swallow the \r\n after the data chunk.
                fread(dummy, 1, 2, fd.f);
            }
            // swallow the extra \r\n after the 0\r\n
            fread(dummy, 1, 2, fd.f);
        }

        if (reqSplit[0] == "GET")
        {
            // search for exact match
            string rest; // remainder after matching path
            TestHttpGetHandler * thgh = findGetHandler(reqSplit[1]);
            if (thgh == NULL)
            {
                // search for component
                size_t slashpos = reqSplit[1].find_last_of('/');
                if (slashpos != string::npos)
                {
                    rest = reqSplit[1].substr(slashpos+1);
                    reqSplit[1].resize(slashpos);
                    thgh = findGetHandler(reqSplit[1]);
                }
            }
            if (thgh)
            {
                if (thgh->generate_data(reqSplit[1], rest,
                                        contents) == false)
                {
                    fprintf(fd.f, "HTTP/1.1 404 FUBARD ONE\r\n");
                    continue;
                }
                else
                {
                    fprintf(fd.f,
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Length: %u\r\n"
                            "\r\n", (unsigned int) contents.size());
                    fwrite(contents.c_str(), contents.size(), 1, fd.f);
                    fflush(fd.f);
                }
            }
            else
                fprintf(fd.f, "HTTP/1.1 404 NO FOUNDED\r\n");
        }
        else if (reqSplit[0] == "PUT") // CURL UPLOAD / UPDATE
        {
            // search for exact match
            string rest; // remainder after matching path
            TestHttpPostHandler * thph = findPostHandler(reqSplit[1]);
            if (thph == NULL)
            {
                // search for component
                size_t slashpos = reqSplit[1].find_last_of('/');
                if (slashpos != string::npos)
                {
                    rest = reqSplit[1].substr(slashpos+1);
                    reqSplit[1].resize(slashpos);
                    thph = findPostHandler(reqSplit[1]);
                }
            }
            if (thph)
            {
                if (thph->handle_data(reqSplit[1], rest,
                                      contents) == false)
                {
                    fprintf(fd.f, "HTTP/1.1 404 FUBARD TWO\r\n");
                    fflush(fd.f);
                    continue;
                }
                else
                {
                    fprintf(fd.f, "HTTP/1.1 200 OK\r\n");
                    fflush(fd.f);
                }
            }
            else
            {
                fprintf(fd.f, "HTTP/1.1 404 NO FOUNDED TWO\r\n");
                fflush(fd.f);
            }
        }
        else if (reqSplit[0] == "POST") // CURL POST / CREATE
        {
            // search for exact match
            string rest; // remainder after matching path
            TestHttpPostHandler * thph = findPostHandler(reqSplit[1]);
            if (thph == NULL)
            {
                // search for component
                size_t slashpos = reqSplit[1].find_last_of('/');
                if (slashpos != string::npos)
                {
                    rest = reqSplit[1].substr(slashpos+1);
                    reqSplit[1].resize(slashpos);
                    thph = findPostHandler(reqSplit[1]);
                }
            }
            if (thph)
            {
                if (thph->handle_data(reqSplit[1], rest,
                                      contents) == false)
                {
                    fprintf(fd.f, "HTTP/1.1 404 FUBARD THREE\r\n");
                    fflush(fd.f);
                }
                else
                {
                    fprintf(fd.f, "HTTP/1.1 200 OK\r\n");
                    fflush(fd.f);
                }
            }
            else
            {
                fprintf(fd.f, "HTTP/1.1 404 NO FOUNDED TWO\r\n");
                fflush(fd.f);
            }
        }
        else
        {
            fprintf(fd.f,
                    "HTTP/1.1 404 UNSUPPORTED METHOD '%s'\r\n",
                    reqSplit[0].c_str());
            fflush(fd.f);
        }
    }
}
