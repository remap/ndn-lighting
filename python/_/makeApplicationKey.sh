#!/bin/sh
# borrowed heavily from 
# lib/ccn_initkeystore.sh
#
# yet rather than default (linux user) keys, this creates a seperate *application* keystore
# and saves it in the (same) ~/.ccnx directory

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
Fail () {
  echo '*** Failed' "$*"
  exit 1
}
#test -d .ccnx && rm -rf .ccnx
test $RSA_KEYSIZE -ge 512 || Fail \$RSA_KEYSIZE too small to sign CCN content
#(umask 077 && mkdir .ccnx) || Fail $0 Unable to create .ccnx directory
cd ~/.ccnx
umask 077
# Set a trap to cleanup on the way out
trap 'rm -f *.pem openssl.cnf' 0
cat <<EOF >openssl.cnf
# This is not really relevant because we're not sending cert requests anywhere,
# but openssl req can refuse to go on if it has no config file.
[ req ]
distinguished_name	= req_distinguished_name
[ req_distinguished_name ]
countryName			= Country Name (2 letter code)
countryName_default		= AU
countryName_min			= 2
countryName_max			= 2
EOF
openssl req    -config openssl.cnf      \
               -newkey rsa:$RSA_KEYSIZE \
               -x509                    \
               -keyout private_key.pem  \
               -out certout.pem         \
               -subj /CN="$APP_NAME"    \
               -nodes                                   || Fail openssl req
openssl pkcs12 -export -name "ccnxuser" \
               -out .ccnx_app_"$APP_NAME"_keystore      \
               -in certout.pem          \
               -inkey private_key.pem   \
               -password pass:'Th1s1sn0t8g00dp8ssw0rd.' || Fail openssl pkcs12
