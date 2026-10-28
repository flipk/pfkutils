#!/bin/bash
set -e

# Automatically run under faketime if available to avoid timezone/clock-skew issues
if [ -z "$FAKETIME_ACTIVE" ] && command -v faketime >/dev/null 2>&1; then
    export FAKETIME_ACTIVE=1
    exec faketime '12 hours ago' "$0" "$@"
fi

# 1 : name (Root-CA, Server-Cert, etc)
# 2 : identity (Some CA, Thingy Root CA, etc)
# 3 : email (somewho@somewhere.com)
# 4 : optional root ca name or ""
genkey() {
    name=$1
    ident=$2
    email=$3
    root=$4

    plain_key_file=${name}-plain.key
    params_file=${name}-params.txt
    request_file=${name}.csr
    cert_file=${name}.crt
    ext_file=${name}-ext.cnf

    openssl ecparam -name secp384r1 -genkey -noout -out "$plain_key_file"
    cat > "$params_file" <<EOF
US
SomeState
SomeCity
$ident
$ident
$ident
$email


EOF

    openssl req -sha512 -utf8 -new -key "$plain_key_file" -out "$request_file" < "$params_file"

    if [ -z "$root" ]; then
        cat > "$ext_file" <<EOF
[v3_ca]
basicConstraints = critical,CA:TRUE
keyUsage = critical,keyCertSign,cRLSign
subjectKeyIdentifier = hash
EOF
        openssl x509 -in "$request_file" -out "$cert_file" \
            -req -sha512 -signkey "$plain_key_file" -days 3650 \
            -extfile "$ext_file" -extensions v3_ca
    else
        cat > "$ext_file" <<EOF
[v3_req]
basicConstraints = CA:FALSE
keyUsage = digitalSignature,keyEncipherment
extendedKeyUsage = serverAuth,clientAuth
subjectKeyIdentifier = hash
authorityKeyIdentifier = keyid,issuer
EOF
        openssl x509 -in "$request_file" -out "$cert_file" \
            -req -sha512 -CA "${root}.crt" -CAkey "${root}-plain.key" \
            -CAcreateserial -days 3650 \
            -extfile "$ext_file" -extensions v3_req
    fi

    rm -f "$params_file" "$ext_file"
}

genkey Root-CA 'Root CA' ca@example.com ''
genkey Server-Cert 'Server Cert' server@example.com Root-CA
genkey Client-Cert 'Client Cert' client@example.com Root-CA

exit 0
