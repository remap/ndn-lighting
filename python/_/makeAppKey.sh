#!/bin/sh
# borrowed heavily from 
# lib/ccn_initkeystore.sh, yet simplified to just PEM
#
#
# usage: makeApplicationKey.sh <applicationName>

if [ $# -ne 1 ]
then
    echo "Invalid Argument Count"
    echo "Syntax: $0 <applicationName>"
    exit
fi

# Create a ccn keystore without relying on java
: ${RSA_KEYSIZE:=1024}
: ${APP_NAME:="$1"}
: ${LIFETIME:=365}

Fail () {
  echo '*** Failed' "$*"
  exit 1
}
#test -d .ccnx && rm -rf .ccnx
test $RSA_KEYSIZE -ge 512 || Fail \$RSA_KEYSIZE too small to sign CCN content

openssl req \
  -x509 -nodes -days $LIFETIME \
  -subj '/C=US/ST=California/L=Los Angeles/OU=UCLA/CN=ccnuser'\
  -newkey rsa:$RSA_KEYSIZE -keyout $APP_NAME.pem -out $APP_NAME.pem \
  || Fail openssl req
