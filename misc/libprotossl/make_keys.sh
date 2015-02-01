#!/bin/sh

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

exit 0
