
#include "libprotossl.h"
#include <mbedtls/version.h>

using namespace ProtoSSL;

#define ssl_personality "ProtoSSLDRBG"

static void
debug_print(void *ptr, int level,
            const char *file, int line,
            const char *s)
{
    const char * slashpos = strrchr(file, '/');
    const char * filename = file;
    if (slashpos != NULL)
        filename = slashpos + 1;
    //_ProtoSSLConn * x = (_ProtoSSLConn *) ptr;
    fprintf(stderr, "%s:%d: %s", filename, line, s);
}

ProtoSSLMsgs :: ProtoSSLMsgs(bool _nonBlockingMode,
                             bool _debugFlag/* = false*/,
                             uint32_t read_timeout /*= 0*/,
                             bool _use_tcp /*= true*/,
                             uint32_t dtls_timeout_min /*= 500*/,
                             uint32_t dtls_timeout_max /*= 10000*/)
    : nonBlockingMode(_nonBlockingMode), debugFlag(_debugFlag),
      use_tcp(_use_tcp)
{
    mbedtls_entropy_init( &entropy );
    memset( &mycert, 0, sizeof( mbedtls_x509_crt ) );
    memset( &cacert, 0, sizeof( mbedtls_x509_crt ) );
    mbedtls_pk_init( &mykey ); // rsa_init( &mykey, RSA_PKCS_V15, 0 );
    mbedtls_ssl_config_init( &sslcfg );
    mbedtls_ssl_config_defaults( &sslcfg, MBEDTLS_SSL_IS_SERVER,
                                 use_tcp ?
                                 MBEDTLS_SSL_TRANSPORT_STREAM :
                                 MBEDTLS_SSL_TRANSPORT_DATAGRAM,
                                 MBEDTLS_SSL_PRESET_DEFAULT );
    mbedtls_ssl_cookie_init( &cookie_ctx );

    //mbedtls_ssl_conf_read_timeout( &sslcfg, 0 );

    mbedtls_ssl_conf_handshake_timeout( &sslcfg,
                                        dtls_timeout_min, dtls_timeout_max );

    mbedtls_ctr_drbg_init( &ctr_drbg );
    mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy,
                           (const unsigned char *) ssl_personality,
                           strlen( ssl_personality ) );
    mbedtls_ssl_conf_rng( &sslcfg, &mbedtls_ctr_drbg_random, &ctr_drbg );

    mbedtls_ssl_conf_authmode( &sslcfg,
                               MBEDTLS_SSL_VERIFY_REQUIRED );

    if (debugFlag)
    {
        mbedtls_ssl_conf_dbg( &sslcfg, &debug_print, (void*) this);
        mbedtls_debug_set_threshold(999);
    }

    if (read_timeout != 0)
    {
        mbedtls_ssl_conf_read_timeout( &sslcfg, read_timeout );
    }

    int ret;
    ret = mbedtls_ssl_cookie_setup( &cookie_ctx,
                                    mbedtls_ctr_drbg_random, &ctr_drbg );

    if (ret != 0 )
        printf( "mbedtls_ssl_cookie_setup returned %d\n\n", ret );

    mbedtls_ssl_conf_dtls_cookies( &sslcfg,
                                   mbedtls_ssl_cookie_write,
                                   mbedtls_ssl_cookie_check,
                                   &cookie_ctx );

    // doesn't appear to be needed?
    //mbedtls_ssl_conf_verify( &sslcfg, f_vrfy, p_vrfy )
}

ProtoSSLMsgs :: ~ProtoSSLMsgs(void)
{
    while (serverList.get_cnt() > 0)
    {
        WaitUtil::Lock lck(&serverList);
        ProtoSSLConnServer * svr = serverList.get_head();
        lck.unlock();
        // server unregisters itself from the list
        delete svr;
    }
    while (clientList.get_cnt() > 0)
    {
        WaitUtil::Lock lck(&clientList);
        ProtoSSLConnClient * client = clientList.get_head();
        lck.unlock();
        // client unregisters itself from the list
        delete client;
    }
    mbedtls_pk_free(&mykey);
    mbedtls_x509_crt_free(&mycert);
    mbedtls_x509_crt_free(&cacert);
    mbedtls_ssl_config_free(&sslcfg);
    mbedtls_ssl_cookie_free( &cookie_ctx );
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);
}

void
ProtoSSLMsgs :: registerServer(ProtoSSLConnServer * svr)
{
    WaitUtil::Lock lck1(&serverList);
    serverList.add_tail(svr);
}

void
ProtoSSLMsgs :: unregisterServer(ProtoSSLConnServer * svr)
{
    WaitUtil::Lock lck1(&serverList);
    serverList.remove(svr);
}

void
ProtoSSLMsgs :: registerClient(ProtoSSLConnClient * clnt)
{
    WaitUtil::Lock lck1(&clientList);
    clientList.add_tail(clnt);
}

void
ProtoSSLMsgs :: unregisterClient(ProtoSSLConnClient * clnt)
{
    WaitUtil::Lock lck1(&clientList);
    clientList.remove(clnt);
}

// returns true if it could load, false if error
bool
ProtoSSLMsgs :: loadCertificates(const ProtoSSLCertParams &params)
{
    int ret;
    char strbuf[200];

    if (debugFlag)
        fprintf(stderr,"loading caCert from %s\n", params.caCert.c_str());
    if (params.caCert.compare(0,5,"file:") == 0)
        ret = mbedtls_x509_crt_parse_file( &cacert,
                                   params.caCert.c_str() + 5);
    else
        ret = mbedtls_x509_crt_parse( &cacert,
                              (const unsigned char *) params.caCert.c_str(),
                              params.caCert.size()+1);
    if (ret != 0)
    {
        mbedtls_strerror( ret, strbuf, sizeof(strbuf));
        fprintf(stderr, " 1 x509parse_crt returned 0x%x: %s\n\n", -ret, strbuf );
        return false;
    }

    if (debugFlag)
        fprintf(stderr,"loading my cert from %s\n", params.myCert.c_str());
    if (params.myCert.compare(0,5,"file:") == 0)
        ret = mbedtls_x509_crt_parse_file( &mycert,
                                   params.myCert.c_str()+5);
    else
        ret = mbedtls_x509_crt_parse( &mycert,
                              (const unsigned char *) params.myCert.c_str(),
                              params.myCert.size()+1);
    if (ret != 0)
    {
        mbedtls_strerror( ret, strbuf, sizeof(strbuf));
        fprintf(stderr, " 2 x509parse_crt returned 0x%x: %s\n\n", -ret, strbuf );
        return false;
    }

    const char *keyPassword = NULL;
    int keyPasswordLen = 0;
    if (params.myKeyPassword != "")
    {
        keyPassword = params.myKeyPassword.c_str();
        keyPasswordLen = params.myKeyPassword.size();
    }

    if (debugFlag)
        fprintf(stderr,"loading my cert key from %s\n", params.myKey.c_str());
    if (params.myKey.compare(0,5,"file:") == 0)
    {
#if defined(MBEDTLS_VERSION_NUMBER) && (MBEDTLS_VERSION_NUMBER < 0x03000000)
        ret = mbedtls_pk_parse_keyfile( &mykey,
                                params.myKey.c_str() + 5,
                                keyPassword);
#else
    // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
    // the args changed!
    // this no longer compiles on mbed 3.6.3 (from fedora 43)
    //   two more args are needed:
    //   int (*f_rng)(void *, unsigned char *, size_t)
    //   void *p_rng
    // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
#endif
    }
    else
    {
#if defined(MBEDTLS_VERSION_NUMBER) && (MBEDTLS_VERSION_NUMBER < 0x03000000)
        ret = mbedtls_pk_parse_key( &mykey,
                            (const unsigned char *) params.myKey.c_str(),
                            params.myKey.size()+1,
                            (const unsigned char *) keyPassword,
                            keyPasswordLen);
#else
    // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
    // the args changed!
    // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
#endif
    }
    if (ret != 0)
    {
        mbedtls_strerror( ret, strbuf, sizeof(strbuf));
        fprintf(stderr, " 3 pk_parse_keyfile returned 0x%x: %s\n\n", -ret, strbuf );
        return false;
    }

    mbedtls_ssl_conf_ca_chain( &sslcfg, &cacert, NULL );
    mbedtls_ssl_conf_own_cert( &sslcfg, &mycert, &mykey );

    return true;
}


static time_t x509_time_to_linux_time(const mbedtls_x509_time &xt)
{
    struct tm   t;

    memset(&t, 0, sizeof(t));

    t.tm_year = xt.year - 1900;  // tm_year is 1900-based, x509.year is 0-based
    t.tm_mon = xt.mon - 1; // tm_mon is 0-based, x509.mon is 1-based
    t.tm_mday = xt.day;
    t.tm_hour = xt.hour;
    t.tm_min = xt.min;
    t.tm_sec = xt.sec;

    return mktime(&t);
}

bool
ProtoSSLMsgs :: validateCertificates(void)
{
    // validate that mycert is signed by cacert chain; then, validate that mycert
    // and mykey match.

    std::string  pubkey, myCertPubkey;

    pubkey.resize(5000);
    int parseRet =
        mbedtls_pk_write_pubkey_pem( &mykey, (unsigned char *)pubkey.c_str(),
                                     pubkey.size());
    if (parseRet == 0)
        pubkey.resize(strlen(pubkey.c_str()));
    else
        pubkey.resize(0);

    myCertPubkey.resize(5000);
    parseRet =
        mbedtls_pk_write_pubkey_pem( &mycert.pk,
                                     (unsigned char *)myCertPubkey.c_str(),
                                     myCertPubkey.size());
    if (parseRet == 0)
        myCertPubkey.resize(strlen(myCertPubkey.c_str()));
    else
        myCertPubkey.resize(0);

    if (pubkey.size() == 0 || pubkey != myCertPubkey)
    {
        fprintf(stderr,
                "ProtoSSLMsgs::validateCertificates : ERROR : "
                "MY CERT and PRIVATE KEY MISMATCH\n");
        return false;
    }

    // next validate that the signature chain actually works.

    time_t t, now = time(NULL);
    time_t latest_start_date = now;
    struct tm broken; // broken-down time
    char broken_string[100];

    t = x509_time_to_linux_time( mycert.valid_from );
#if 0 // debug prints
    gmtime_r(&t, &broken);
    strftime(broken_string, sizeof(broken_string), "%F %T", &broken);
    fprintf(stderr,
            "ProtoSSLMsgs::validateCertificates : "
            "devCert time %u (%s)\n", (uint32_t) t, broken_string);
#endif

    if (t > latest_start_date)
        latest_start_date = t;

    for (mbedtls_x509_crt *crt = &cacert; crt; crt = crt->next)
    {
        t = x509_time_to_linux_time( crt->valid_from );
#if 0 // debug prints
        gmtime_r(&t, &broken);
        strftime(broken_string, sizeof(broken_string), "%F %T", &broken);
        fprintf(stderr,
                "ProtoSSLMsgs::validateCertificates : "
                "caCert time %u (%s)\n", (uint32_t) t, broken_string);
#endif
        if (t > latest_start_date)
            latest_start_date = t;

#if 0   // actually, don't have a need for the root cert just now. leaving
        // this code here for documentation purposes.
        // look for the root cert: you know by whether
        // issuer and subject are the same.
        if (crt->issuer_raw.len == crt->subject_raw.len &&
            memcmp(crt->issuer_raw.p, crt->subject_raw.p,
                   crt->issuer_raw.len) == 0)
        {
            rootCert = crt;
        }
#endif
    }

    if (latest_start_date > now)
    {
        gmtime_r(&now, &broken);
        strftime(broken_string, sizeof(broken_string), "%F %T", &broken);
        fprintf(stderr,
                "ProtoSSLMsgs::validateCertificates: ERROR: "
                "time now (%u %s) is outside certificate valid times\n",
                (uint32_t) now, broken_string);
        return false;
    }

    uint32_t flags = 0;
    parseRet =
        mbedtls_x509_crt_verify(
            &mycert, &cacert, /*ca_crl*/ NULL,
            /*cn*/ NULL, &flags,
            /*f_vrfy*/ NULL, /*p_vrfy*/ NULL );

    if (parseRet != 0)
    {
        std::string info;
        info.resize(200);
        parseRet = mbedtls_x509_crt_verify_info(
            (char*) info.c_str(), info.size(),
            /*prefix*/ "", flags);

        if (parseRet >= 0)
        {
            info.resize(parseRet);
            fprintf(stderr,
                    "ProtoSSLMsgs::validateCertificates : ERROR: "
                    "CERTIFICATE VALIDATION FAILED: %s\n", info.c_str());
        }

        return false;
    }
    return true;
}

ProtoSSLConnServer *
ProtoSSLMsgs :: startServer(int listeningPort)
{
    ProtoSSLConnServer * svr = new ProtoSSLConnServer(this, listeningPort, use_tcp);
    if (svr->ok() == false)
    {
        delete svr;
        svr = NULL;
    }
    return svr;
}

ProtoSSLConnClient *
ProtoSSLMsgs :: startClient(const std::string &remoteHost,
                            int remotePort)
{
    mbedtls_ssl_conf_endpoint( &sslcfg, MBEDTLS_SSL_IS_CLIENT );
    ProtoSSLConnClient * client =
        new ProtoSSLConnClient(this, remoteHost, remotePort, use_tcp);
    if (client->ok() == false)
    {
        delete client;
        client = NULL;
    }
    return client;
}
