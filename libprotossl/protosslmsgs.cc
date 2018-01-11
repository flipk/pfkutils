
#include "libprotossl.h"

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

ProtoSSLMsgs :: ProtoSSLMsgs(bool _nonBlockingMode, bool _debugFlag/* = false*/)
    : nonBlockingMode(_nonBlockingMode), debugFlag(_debugFlag)
{
    mbedtls_entropy_init( &entropy );
    memset( &mycert, 0, sizeof( mbedtls_x509_crt ) );
    memset( &cacert, 0, sizeof( mbedtls_x509_crt ) );
    mbedtls_pk_init( &mykey ); // rsa_init( &mykey, RSA_PKCS_V15, 0 );
    mbedtls_ssl_config_init( &sslcfg );
    mbedtls_ssl_config_defaults( &sslcfg, MBEDTLS_SSL_IS_SERVER,
                                 MBEDTLS_SSL_TRANSPORT_STREAM,
                                 MBEDTLS_SSL_PRESET_DEFAULT );

    //mbedtls_ssl_conf_read_timeout( &sslcfg, 0 );

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
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);
}

void
ProtoSSLMsgs :: registerServer(ProtoSSLConnServer * svr)
{
    WaitUtil::Lock lck1(&serverList);
    WaitUtil::Lock lck2(&serverHash);
    serverList.add_tail(svr);
    serverHash.add(svr);
}

void
ProtoSSLMsgs :: unregisterServer(ProtoSSLConnServer * svr)
{
    WaitUtil::Lock lck1(&serverList);
    WaitUtil::Lock lck2(&serverHash);
    serverList.remove(svr);
    serverHash.remove(svr);
}

void
ProtoSSLMsgs :: registerClient(ProtoSSLConnClient * clnt)
{
    WaitUtil::Lock lck1(&clientList);
    WaitUtil::Lock lck2(&clientHash);
    clientList.add_tail(clnt);
    clientHash.add(clnt);
}

void
ProtoSSLMsgs :: unregisterClient(ProtoSSLConnClient * clnt)
{
    WaitUtil::Lock lck1(&clientList);
    WaitUtil::Lock lck2(&clientHash);
    clientList.remove(clnt);
    clientHash.remove(clnt);
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
        ret = mbedtls_pk_parse_keyfile( &mykey,
                                params.myKey.c_str() + 5,
                                keyPassword);
    else
        ret = mbedtls_pk_parse_key( &mykey,
                            (const unsigned char *) params.myKey.c_str(),
                            params.myKey.size()+1,
                            (const unsigned char *) keyPassword,
                            keyPasswordLen);
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

ProtoSSLConnServer *
ProtoSSLMsgs :: startServer(int listeningPort)
{
    ProtoSSLConnServer * svr = new ProtoSSLConnServer(this, listeningPort);
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
        new ProtoSSLConnClient(this, remoteHost, remotePort);
    if (client->ok() == false)
    {
        delete client;
        client = NULL;
    }
    return client;
}
