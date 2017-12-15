
#include <stdio.h>

#include "options.h"
#include "libprotossl.h"

extern "C" int
i3_main(int argc, char ** argv)
{
    i3_options opts(argc, argv);

    if (opts.ok == false)
    {
        printf("option parsing failure\n");
        return 1;
    }

    ProtoSSL::ProtoSSLMsgs  msgs(opts.debug_flag);

    ProtoSSL::ProtoSSLCertParams  certs(opts.ca_cert_path,
                              opts.my_cert_path,
                              opts.my_key_path,
                              opts.my_key_password,
                              I3_OPTIONS_OTHERCOMMONNAME);

    if (msgs.loadCertificates(certs) == false)
    {
        printf("certificate loading failed, try with -d\n");
        return 1;
    }

#if 0
        myFactoryServer fact;
        msgs.startServer(fact, 2005);
        while (msgs.run())
            ;
        myFactoryClient fact;
        msgs.startClient(fact, argv[2], 2005);
        while (msgs.run())
            ;
#endif

    return 0;
}
