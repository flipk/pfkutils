
#include "curl/curl.h"
#include "simple_json.h"
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define SERVER_PORT "8080"

void usage(void)
{
    printf("usage : test_http_client <ipaddr> <test_number>\n");
    printf("      test 0 : get status\n");
}

class curlWriteStorage
{
    std::string &store;
public:
    curlWriteStorage(std::string &_store) : store(_store)
    {
        store.resize(0);
    }
    static size_t writer(const void *ptr, size_t size,
                       size_t nmemb, void *userptr)
    {
        size = size * nmemb;
        struct curlWriteStorage *cws = (struct curlWriteStorage *)userptr;
        size_t oldsize = cws->store.size();
        cws->store.resize(oldsize + size);
        char * dest = (char*) cws->store.c_str() + oldsize;
        memcpy(dest, ptr, size);
        return size;
    }
};

bool
do_rest_get (const std::string &url, std::string &data)
{
    bool ret = false;
    CURL * easy = curl_easy_init();
    curl_easy_setopt(easy, CURLOPT_VERBOSE, 1L);
//    curl_easy_setopt(easy, CURLOPT_DEBUGFUNCTION, my_trace);
    curl_easy_setopt(easy, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(easy, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(easy, CURLOPT_URL, url.c_str());

    curlWriteStorage  writeStore(data);

    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, &curlWriteStorage::writer);
    curl_easy_setopt(easy, CURLOPT_WRITEDATA, (void*) &writeStore);

    CURLcode res = curl_easy_perform(easy);
    if (res != CURLE_OK)
    {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
    }
    else
    {
        // writeStore leaves the data it collected in the
        // "data" arg that was passed at construction.
        ret = true;
    }

    curl_easy_cleanup(easy);
    return ret;
}

bool
do_rest_post(const std::string &url, const std::string &data)
{
    bool ret = false;
    CURL * easy = curl_easy_init();
    curl_easy_setopt(easy, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(easy, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(easy, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(easy, CURLOPT_URL, url.c_str());

// according to some standards thingies, updating something is PUT
// and creating something is POST, but this API uses POST for everything.

// when using PUT, curl does Content-Length.
// when using POST, curl does Transfer-Encoding: chunked.
// odd, that.

//  curl_easy_setopt(easy, CURLOPT_UPLOAD, 1L); // UPDATE is PUT
    curl_easy_setopt(easy, CURLOPT_POST, 1L); // CREATE is POST

    FILE * f = fmemopen((void*)data.c_str(), data.size(), "r");

    curl_easy_setopt(easy, CURLOPT_READDATA, (void*) f);
    curl_easy_setopt(easy, CURLOPT_INFILESIZE_LARGE, data.size());

    CURLcode res = curl_easy_perform(easy);
    fclose(f);

    if (res != CURLE_OK)
    {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
    }
    else
    {
        ret = true;
    }

    curl_easy_cleanup(easy);
    return ret;
}

int
main(int argc, char ** argv)
{
    if (argc != 3)
    {
        usage();
        return 1;
    }

    std::string ipaddr(argv[1]);

    int test_number = atoi(argv[2]);
    if (test_number < 0 || test_number > 4)
    {
        usage();
        return 1;
    }

    curl_global_init(CURL_GLOBAL_ALL);

    if (test_number == 0)
    {
        std::string data;
        if (do_rest_get("http://" + ipaddr + ":" + SERVER_PORT + "/status",data))
            printf("GOT STATUS : %s\n", data.c_str());
        else
            printf("FAILED to get status\n");
    }

    curl_global_cleanup();

    return 0;
}
