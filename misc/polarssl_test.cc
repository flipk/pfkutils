#if 0

if [ x$1 = xk ] ; then

pass=`random_text 10`
echo $pass > ca.password
openssl genrsa -out ca.key.enc -passout pass:$pass -aes256 2048
openssl rsa -in ca.key.enc -passin pass:$pass -out ca.key
openssl req -utf8 -new -key ca.key -out ca.req << EOF
US
CA State
CA City
CA Org
CA Org Unit
CA Common Name
CA Email


EOF
openssl x509 -in ca.req -out ca.crt -req -signkey ca.key -days 3650

echo xxxxxxxxxxxxxxxxxxx making srv key and cert

pass=`random_text 10`
echo $pass > srv.password
openssl genrsa -out srv.key.enc -passout pass:$pass -aes256 2048
openssl rsa -in srv.key.enc -passin pass:$pass -out srv.key
openssl req -utf8 -new -key srv.key -out srv.req << EOF
US
Srv State
Srv City
Srv Org
Srv Org Unit
Srv Common Name
Srv Email


EOF
openssl x509 -in srv.req -out srv.crt -req -CA ca.crt -CAkey ca.key -CAcreateserial -days 3650

echo xxxxxxxxxxxxxxxxxxx making client key and cert

pass=`random_text 10`
echo $pass > clnt.password
openssl genrsa -out clnt.key.enc -passout pass:$pass -aes256 2048
openssl rsa -in clnt.key.enc -passin pass:$pass -out clnt.key
openssl req -utf8 -new -key clnt.key -out clnt.req << EOF
US
Clnt State
Clnt City
Clnt Org
Clnt Org Unit
Clnt Common Name
Clnt Email


EOF
openssl x509 -in clnt.req -out clnt.crt -req -CA ca.crt -CAkey ca.key -CAcreateserial -days 3650

elif [ x$1 = xm ] ; then

set -e -x
g++ -O3 -Wall -Werror test.cc -o t -lpolarssl

else 
    echo "usage: 'sh test.cc m' to make the binary"
    echo "       'sh test.cc k' to make the keys"
fi

exit 0
#endif


#include <polarssl/entropy.h>
#include <polarssl/ctr_drbg.h>
#include <polarssl/ssl.h>
#include <polarssl/net.h>
#include <stdlib.h>
#include <string>
#include <iostream>

#define SRV_COMMON_NAME "Srv Common Name"
#define CLNT_COMMON_NAME "Clnt Common Name"

#define CA_CRT_FILE "ca.crt"
#define SRV_CRT_FILE "srv.crt"
#define SRV_KEY_FILE "srv.key"
#define CLNT_CRT_FILE "clnt.crt"
#define CLNT_KEY_FILE "clnt.key"

using namespace std;

int usage(const char * errmsg = NULL)
{
    if (errmsg)
        cerr << errmsg;
    cerr << "\nusage:\n"
         << "   t -l port\n"
         << "   t -c host port\n";
    return 1;
}

void my_debug( void *ctx, int level, const char *str )
{
    fprintf( (FILE *) ctx, "%s", str );
    fflush(  (FILE *) ctx  );
}

struct Args {
    bool srvclispec;
    bool server;
    bool debug;
    string remoteHost;
    int port;
    Args(void) {
        port = -1;
        srvclispec = false;
        server = false;
        debug = false;
    }
} args;

int
main(int argc, char ** argv)
{
    Args args;
    int ret, fd, len;
    entropy_context entropy;
    ctr_drbg_context ctr_drbg;
    const char *pers = "ssl_server";
    x509_cert cacert, mycert;
    rsa_context mykey;
    ssl_context  sslctx;
    unsigned char buf[512];

    argc--;
    argv++;
    while (argc > 0)
    {
        string arg(argv[0]);

        if (arg == "-l")
        {
            if (args.srvclispec)
                return usage("choose server or client, not both");
            args.srvclispec = true;
            if (argc > 1)
            {
                args.server = true;
                args.port = atoi(argv[1]);
                argc--;
                argv++;
            }
            else
            {
                cerr << "-l requires a port\n";
                return 1;
            }
        }
        else if (arg == "-c")
        {
            if (args.srvclispec)
                return usage("choose server or client, not both");
            args.srvclispec = true;
            if (argc > 2)
            {
                args.server = false;
                args.remoteHost = argv[1];
                args.port = atoi(argv[2]);
                argc -= 2;
                argv += 2;
            }
            else
            {
                cerr << "-c requires a host and port\n";
                return 1;
            }
        }
        else if (arg == "-d")
        {
            args.debug = true;
        }
        argc--;
        argv++;
    }

    if (!args.srvclispec)
        return usage();

    if (args.server)
    {
        if (args.port < 1000)
            return usage("if server, must provide port");
        cout << "server on port " << args.port << endl;
    }
    else
    {
        if (args.port < 1000 || args.remoteHost == "")
            return usage("if client, must provide host and port");
        cout << "connect to client at host " << args.remoteHost
             << " on port " << args.port << endl;
    }

    entropy_init( &entropy );

    if( ( ret = ctr_drbg_init( &ctr_drbg, entropy_func, &entropy,
                               (const unsigned char *) pers,
                               strlen( pers ) ) ) != 0 )
    {
        printf( " ctr_drbg_init returned -0x%x\n", -ret );
        return 1;
    }


    memset( &mycert, 0, sizeof( x509_cert ) );
    memset( &cacert, 0, sizeof( x509_cert ) );
    rsa_init( &mykey, RSA_PKCS_V15, 0 );

    if ((ret = x509parse_crtfile( &cacert, CA_CRT_FILE)) != 0)
    {
        printf( " failed\n  !  x509parse_crt returned -0x%x\n\n", -ret );
        return 1;
    }

    if ((ret = x509parse_crtfile(
             &mycert, args.server ? SRV_CRT_FILE : CLNT_CRT_FILE )) != 0)
    {
        printf( " failed\n  !  x509parse_crt returned -0x%x\n\n", -ret );
        return 1;
    }

    if ((ret = x509parse_keyfile(
             &mykey, args.server ? SRV_KEY_FILE : CLNT_KEY_FILE,
             NULL )) != 0)
    {
        printf( " failed\n  !  x509parse_key returned -0x%x\n\n", -ret );
        return 1;
    }

    if ((ret = ssl_init( &sslctx )) != 0)
    {
        printf("ssl init returned -0x%x\n", -ret);
        return 1;
    }

    ssl_set_endpoint( &sslctx, args.server ? SSL_IS_SERVER : SSL_IS_CLIENT );
    ssl_set_authmode( &sslctx, SSL_VERIFY_REQUIRED );
    ssl_set_rng( &sslctx, ctr_drbg_random, &ctr_drbg );
    if (args.debug)
        ssl_set_dbg( &sslctx, &my_debug, stdout);

    if (args.server)
    {
        ssl_set_ca_chain( &sslctx, &cacert, NULL, CLNT_COMMON_NAME );
        ssl_set_own_cert( &sslctx, &mycert, &mykey );
    }
    else
    {
        // this string below is the srv.crt Common Name field
        ssl_set_ca_chain( &sslctx, &cacert, NULL, SRV_COMMON_NAME );
        ssl_set_own_cert( &sslctx, &mycert, &mykey );
        ssl_set_hostname( &sslctx, SRV_COMMON_NAME );
    }

    if (args.server)
    {
        int listen_fd;
        if ((ret = net_bind(&listen_fd, NULL, args.port)) != 0)
        {
            printf(" net_bind failed error -0x%x\n", -ret);
            return 1;
        }
        if ((ret = net_accept(listen_fd, &fd, NULL)) != 0)
        {
            printf(" net_accept failed error -0x%x\n", -ret);
            return 1;
        }
        net_close(listen_fd);
    }
    else
    {
        if ((ret = net_connect(&fd, args.remoteHost.c_str(), args.port)) != 0)
        {
            printf("net_connect failed with error -0x%x\n", -ret);
            return 1;
        }
    }

    ssl_set_bio( &sslctx, net_recv, &fd, net_send, &fd );

    printf("got connection\n");

    while ((ret = ssl_handshake( &sslctx )) != 0)
    {
        if (ret != POLARSSL_ERR_NET_WANT_READ &&
            ret != POLARSSL_ERR_NET_WANT_WRITE )
        {
            printf( " failed\n  ! ssl_handshake returned -0x%x\n\n", -ret );
            return 1;
        }
    }
    
    printf("SSL connection handshake complete\n");

    printf("verify peer cert: ");
    if( ( ret = ssl_get_verify_result( &sslctx ) ) != 0 )
    {
        printf( " failed\n" );
        if( ( ret & BADCERT_EXPIRED ) != 0 )
            printf( "  ! server certificate has expired\n" );
        if( ( ret & BADCERT_REVOKED ) != 0 )
            printf( "  ! server certificate has been revoked\n" );
        if( ( ret & BADCERT_CN_MISMATCH ) != 0 )
            printf( "  ! CN mismatch (expected CN=%s)\n",
                    "PolarSSL Server 1" );
        if( ( ret & BADCERT_NOT_TRUSTED ) != 0 )
            printf( "  ! self-signed or not signed by a trusted CA\n" );
        printf( "\n" );
    }
    else
        printf( " ok\n" );

    if (args.server)
    {
        while (1)
        {
            len = sizeof(buf)-1;
            ret = ssl_read( &sslctx, buf, len );
            if (ret < 0)
                printf("ssl_read returned -0x%x\n", -ret);
            else
                printf("ssl_read returned 0x%x\n", ret);
            if( ret == POLARSSL_ERR_NET_WANT_READ ||
                ret == POLARSSL_ERR_NET_WANT_WRITE )
                continue;
            if( ret <= 0 )
            {
                switch( ret )
                {
                case POLARSSL_ERR_SSL_PEER_CLOSE_NOTIFY:
                    printf( " connection was closed gracefully\n" );
                    break;
                case POLARSSL_ERR_NET_CONN_RESET:
                    printf( " connection was reset by peer\n" );
                    break;
                default:
                    printf( " ssl_read returned -0x%x\n", -ret );
                    break;
                }
                break;
            }
            if (ret > 0)
            {
                len = ret;
                printf( " %d bytes read\n\n%s", len, (char *) buf );
                do {
                    ret = ssl_write( &sslctx, buf, len);
                } while (ret == POLARSSL_ERR_NET_WANT_READ ||
                         ret == POLARSSL_ERR_NET_WANT_WRITE);
                if (ret < 0)
                    printf("ssl_write returned -0x%x\n", -ret);
                else
                    printf("ssl_write returned 0x%x\n", ret);
                if( ret == POLARSSL_ERR_NET_CONN_RESET )
                {
                    printf( " failed\n  ! peer closed the connection\n\n" );
                    break;
                }
            }
        }
    }
    else /* ! server */
    {
        while ((ret = ssl_write(&sslctx,
                                (unsigned char *) "hello\r\n", 7)) <= 0)
        {
            if( ret != POLARSSL_ERR_NET_WANT_READ &&
                ret != POLARSSL_ERR_NET_WANT_WRITE )
            {
                printf( " failed\n  ! ssl_write returned -0x%x\n\n", -ret );
                return 1;
            }
        }
        do {
            len = sizeof(buf)-1;
            ret = ssl_read( &sslctx, buf, len );
            if( ret == POLARSSL_ERR_NET_WANT_READ ||
                ret == POLARSSL_ERR_NET_WANT_WRITE )
                continue;
            if( ret == POLARSSL_ERR_SSL_PEER_CLOSE_NOTIFY )
                break;
            if( ret < 0 )
            {
                printf( "failed\n  ! ssl_read returned -0x%x\n\n", -ret );
                break;
            }
            if( ret == 0 )
            {
                printf( "\n\nEOF\n\n" );
                break;
            }
            len = ret;
            printf( " %d bytes read\n\n%s", len, (char *) buf );
        } while (1);
    }

    while ((ret = ssl_close_notify( &sslctx )) < 0)
    {
        if( ret != POLARSSL_ERR_NET_WANT_READ &&
            ret != POLARSSL_ERR_NET_WANT_WRITE )
        {
            printf( " ssl_close_notify returned -0x%x\n\n", -ret );
            return 1;
        }
    }

    net_close(fd);
    ssl_free( &sslctx );
    x509_free( &cacert );
    x509_free( &mycert );
    rsa_free( &mykey );

//size_t ssl_get_bytes_avail( const ssl_context *ssl );

    return 0;
}
