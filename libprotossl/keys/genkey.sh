#!/bin/sh

last_modified="2017-12-27.20:59:16"

# 1 : name prefix for files (PFK-Root-CA or i3-client, etc)
# 2 : org name
# 3 : org unit
# 4 : common name (for web, the website hostname)
# 5 : email addr
# 6 : password for private key
# 7 : optional root ca file name prefix (or "" to make a root CA)
genkey() {
    encrypted_key_file=$1-encrypted.key
    plain_key_file=$1.key
    params_file=$1-params.txt
    request_file=$1.csr
    cert_file=$1.crt
    cert_txt=$1.crt.txt
    pwd_file=$1-password.txt
    org_name=$2
    org_unit=$3
    common_name=$4
    email=$5
    pass=$6
    rootkey=$7.key
    rootcert=$7.crt

    echo $pass > $pwd_file
    openssl genrsa -out $encrypted_key_file -passout pass:$pass -aes256 4096
    openssl rsa -in $encrypted_key_file -passin pass:$pass -out $plain_key_file
    cat > $params_file <<EOF
US
PFK-State
PFK-City
$org_name
$org_unit
$common_name
$email


EOF

    openssl req -sha512 -utf8 -new -key $plain_key_file -out $request_file < $params_file

    if [ x$7 = x ] ; then
        openssl x509 -in $request_file -out $cert_file \
            -req -sha512 -signkey $plain_key_file -days 3650
    else
        openssl x509 -in $request_file -out $cert_file \
            -req -sha512 -CA $rootcert -CAkey $rootkey \
            -CAcreateserial -days 3650
    fi

    openssl x509 -in $cert_file -noout -text > $cert_txt

#    rm -f $params_file $request_file
#    chmod 400 $encrypted_key_file $plain_key_file $cert_file $pwd_file
}

# Local Variables:
# mode: shell-script
# indent-tabs-mode: nil
# tab-width: 8
# eval: (add-hook 'write-file-hooks 'time-stamp)
# time-stamp-start: "last_modified="
# time-stamp-format: "\"%:y-%02m-%02d.%02H:%02M:%02S\""
# time-stamp-end: "$"
# End:
