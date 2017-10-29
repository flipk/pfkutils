#!/bin/sh

#cd /etc/ssl/certs
#openssl dhparam -out dhparam.pem 4096

# nginx.conf:
#ssl_ciphers 'AES128+EECDH:AES128+EDH';
#ssl_prefer_server_ciphers on;
#ssl_session_cache shared:SSL:10m;
#ssl_dhparam /etc/ssl/certs/dhparam.pem;

# 1 : name (generic, PFK-Root-CA, www.pfk.org, etc)
# 2 : identity (Some CA, PFK Root CA, www.pfk.org, etc)
# 3 : email (somewho@somewhere.com, pfk@pfk.org)
# 4 : password to protect generated private key
# 5 : optional root ca name or ""
genkey() {
    encrypted_key_file=$1-encrypted.key
    plain_key_file=$1-plain.key
    params_file=$1-params.txt
    request_file=$1.csr
    cert_file=$1.crt
    pwd_file=$1-password.txt
    ident=$2
    email=$3
    pass=$4
    rootkey=$5-plain.key
    rootcert=$5.crt

    echo $pass > $pwd_file
    openssl genrsa -out $encrypted_key_file -passout pass:$pass -aes256 4096
    openssl rsa -in $encrypted_key_file -passin pass:$pass -out $plain_key_file
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

    if [ x$5 == x ] ; then
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

genkey Root-CA 'Root CA' ca@example.com `random_text 40` ''

server_pwd="0KZ7QMalU75s0IXoWnhm3BXEtswirfwrXwwNiF6c"
genkey Server-Cert 'Server Cert' server@example.com $server_pwd Root-CA

client_pwd="IgiLNFWx3fTMioJycI8qXCep8j091yfHOwsBbo6f"
genkey Client-Cert 'Client Cert' client@example.com $client_pwd Root-CA

exit 0

#display a csr with:
openssl req -in     Root-CA.csr -noout -text
openssl req -in Client-Cert.csr -noout -text
openssl req -in Server-Cert.csr -noout -text

#display a crt with:
openssl x509 -in     Root-CA.crt -noout -text
openssl x509 -in Client-Cert.crt -noout -text
openssl x509 -in Server-Cert.crt -noout -text

#display a key with:
openssl rsa -in     Root-CA-plain.key -noout -text
openssl rsa -in Client-Cert-plain.key -noout -text
openssl rsa -in Server-Cert-plain.key -noout -text

# in windows, run "certmgr.msc"
