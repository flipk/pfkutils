#!/bin/sh

#cd /etc/ssl/certs
#openssl dhparam -out dhparam.pem 4096

# nginx.conf:
#ssl_ciphers 'AES128+EECDH:AES128+EDH';
#ssl_prefer_server_ciphers on;
#ssl_session_cache shared:SSL:10m;

# 1 : name (generic, Thingy-Root-CA, etc)
# 2 : identity (Some CA, Thingy Root CA, etc)
# 3 : email (somewho@somewhere.com)
# 4 : optional root ca name or ""
genkey() {
    plain_key_file=$1-plain.key
    params_file=$1-params.txt
    request_file=$1.csr
    cert_file=$1.crt
    ident=$2
    email=$3
    rootkey=$4-plain.key
    rootcert=$4.crt

    openssl ecparam -name secp384r1 -genkey -noout -out $plain_key_file
    cat > $params_file <<EOF
US
SomeState
SomeCity
$ident
$ident
$ident
$email


EOF

    openssl req -sha512 -utf8 -new -key $plain_key_file -out $request_file < $params_file

    if [ x$4 = x ] ; then
        openssl x509 -in $request_file -out $cert_file \
            -req -sha512 -signkey $plain_key_file -days 3650
    else
        openssl x509 -in $request_file -out $cert_file \
            -req -sha512 -CA $rootcert -CAkey $rootkey \
            -CAcreateserial -days 3650
    fi

#    rm -f $params_file $request_file
#    chmod 400 $encrypted_key_file $plain_key_file $cert_file $pwd_file
}

genkey Root-CA 'Root CA' ca@example.com ''
genkey Server-Cert 'Server Cert' server@example.com Root-CA
genkey Client-Cert 'Client Cert' client@example.com Root-CA

exit 0
