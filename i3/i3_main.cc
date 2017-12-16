
#include <stdio.h>

#include "i3_options.h"
#include "i3_protossl_factory.h"

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

    i3protoFact  fact(opts);

    if (opts.outbound)
    {
        if (opts.debug_flag)
            printf("starting msgs.client\n");
        msgs.startClient(fact, opts.hostname, opts.port_number);
    }
    else
    {
        if (opts.debug_flag)
            printf("starting msgs.server\n");
        msgs.startServer(fact, opts.port_number);
    }

    while (msgs.run())
        ;

    return 0;
}
