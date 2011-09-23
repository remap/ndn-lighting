
## human configuration for application signing:

appName = "TV1Sequencer"
appPrefix = "ccnx:/ndn/ucla.edu/apps/lighting"
keyFile = "sequencer.pem"

appDescription = "Sequencer for TV1."
capabilities = {"setRGB", "readRGB"}
appDeviceNames = {"living-room-front","living-room-right","window-left"}

appControlNameSpace = {
"ccnx:/ndn/ucla.edu/apps/living-room-right/readRGB",
"ccnx:/ndn/ucla.edu/apps/window-left/readRGB",
"ccnx:/ndn/ucla.edu/apps/living-room-front/readRGB",
"ccnx:/ndn/ucla.edu/apps/living-room-right/setRGB",
"ccnx:/ndn/ucla.edu/apps/window-left/setRGB",
"ccnx:/ndn/ucla.edu/apps/living-room-front/setRGB"}

# simulation of burned in names
#
# these could be pulled in from external server via https or ccnx
# but that is in itself a hack / temporary, so for now we put them here & can extend python in future

deviceList = (
'00:1c:42:00:00:08', '169.192.0.13', 'phillips/ColorBlast/',
'00:1c:42:00:00:04', '169.192.0.14', 'phillips/ColorBlaze',
'00:1c:42:00:00:06', '169.192.0.12', 'gumstix/overo/fire'
)
# to allow expansion of list if needed
# change if change above list schema
numValPerKey = 3
# right now just MAC, IP, MfrTypeComponent


# linkage of deviceList to application aliases
#
#not used in this file (yet), but in web/config.py (for UAG / sequencer / analyzer)
# application device name (alias) mapping to default/config device name

deviceNames = {
"living-room-front":"ColorBlast/1",
"living-room-right":"ColorBlast/1",
"entrance-door":"ColorBlast/2",
"window-right":"ColorBlast/3",
"stairs":"ColorBlast/4",
"bedroom":"ColorBlast/2",
"kitchen":"ColorBlast/2",
"window-left":"ColorBlast/3",
"incandescent":"ArtNet"}


# this is for TV1 - not used @ mo, just for notes
kinetDeviceList = (
'192.168.3.52', 'colorBlast',
'192.168.3.51', 'colorBlaze1',
'192.168.3.50', 'colorBlaze2',
'131.179.141.17', 'ArtNet'
)
# also note IP address is for Kinet and is to be auto-detected and written *after* ccnx cfg handshake
# the only thing humans should enter to this file is the serial of the devices & the typeComponent
# the serial field could be populated with MAC address for now.
# as well as anything necessary for application signing


# this is for TV1 - 



# x509 cert for this application

-----BEGIN RSA PRIVATE KEY-----
MIICWwIBAAKBgQDYjl6fJ2LfNPypTfDTY5f5INjXjsLM8nlwpVv/+FhfITJ5aXWO
uTEM+VfTU5yeCsZ8feJPrvMyFUW8Y+Gzw3iF3Y4LN6vW3A2tNX8odlxFgZi87TxT
bf33W18Lsw1hY9yyquQ/0wmsdKtwnGc5g1FAc3bCrDqIn3Mw/eJ6JBg/KwIDAQAB
AoGAOImC3p5Ty95zkGgO1cGMrbgLpMtaxFMRrX0edceKmtt56ATGckqbKb33Ve7g
VsqNY3ciHJeaWpr/J94T0PDZh+5DkK0+5ITnHKcQkjh2cmMfvF6arihz8YZW+Wxn
4SeUcAvK31j/sQiY74pJfoRTkmEBgGiB11bUbRbbeC9azfECQQD8BgzBeQPkDazl
2Qe4fsOb869DJHLU9Jrl1xWBzyjZ4gLHymipABvCeCqiydvfTWAmtDOQqCgQI1G8
ndaTKqZHAkEA2/kQEygOEl5bufNfW29newYLcekBQgQYSbcBe6UENb/NW9xMtSsj
tV9owiZn4JwmIkUJWOHjOlx4p/0qLpU9/QI/R91xxbm8YkuEAgbhLLr5DH9werTq
Cc+2W9P5TgrEm1zXbiFJtudRAyNBBSqKiCKISIEaMyXARtnLdP2NMmn3AkButNeB
PJxwZCq32CM6qkOjJ7Sk3IZ0igkLOF43syH+Rwx8WdAFMbPj+SOI6rlG0m9iuent
YKnyWk2jBgevsA6lAkEAnU35Hd/Vz4el4/8AE6L8gPuIvVt9DsbDCrLdh/TmPaHk
JuHPG63CJw1FXKytY05giOSrSBEljsDoYaSYmobH7Q==
-----END RSA PRIVATE KEY-----
-----BEGIN CERTIFICATE-----
MIICszCCAhygAwIBAgIJAMasciUxywF/MA0GCSqGSIb3DQEBBQUAMEYxCzAJBgNV
BAYTAlVTMQswCQYDVQQIEwJDQTELMAkGA1UEBxMCTEExDTALBgNVBAoTBFVDTEEx
DjAMBgNVBAsTBVJFTUFQMB4XDTExMDkxNjE3MzAxNloXDTEyMDkxNTE3MzAxNlow
RjELMAkGA1UEBhMCVVMxCzAJBgNVBAgTAkNBMQswCQYDVQQHEwJMQTENMAsGA1UE
ChMEVUNMQTEOMAwGA1UECxMFUkVNQVAwgZ8wDQYJKoZIhvcNAQEBBQADgY0AMIGJ
AoGBANiOXp8nYt80/KlN8NNjl/kg2NeOwszyeXClW//4WF8hMnlpdY65MQz5V9NT
nJ4Kxnx94k+u8zIVRbxj4bPDeIXdjgs3q9bcDa01fyh2XEWBmLztPFNt/fdbXwuz
DWFj3LKq5D/TCax0q3CcZzmDUUBzdsKsOoifczD94nokGD8rAgMBAAGjgagwgaUw
HQYDVR0OBBYEFFGzeveYgH6vUlMNQgcdTGFNzMDCMHYGA1UdIwRvMG2AFFGzeveY
gH6vUlMNQgcdTGFNzMDCoUqkSDBGMQswCQYDVQQGEwJVUzELMAkGA1UECBMCQ0Ex
CzAJBgNVBAcTAkxBMQ0wCwYDVQQKEwRVQ0xBMQ4wDAYDVQQLEwVSRU1BUIIJAMas
ciUxywF/MAwGA1UdEwQFMAMBAf8wDQYJKoZIhvcNAQEFBQADgYEApaz/MQ4z6fUE
5xCOqPizMdbwN8RW4Faz0rxLkpoxr7KpozUTEA8IcUw53ecJMrWH0igGQy/rToon
IShZ0dBTDUTVl2enCh/yvpjvBDiEXoD86EXQaKRnReZfL7xJVPdFkBC859i7X00x
ngTrC5Qra1y7wIdXdBMDBpp1Bo+Tmgo=
-----END CERTIFICATE-----