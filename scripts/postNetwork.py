import httplib, urllib
import os
import commands

server = "bigriver.remap.ucla.edu:80" 
path = "/lighting/devices/devices.py/postData"
command = "ifconfig"

result = commands.getoutput(command)

params = urllib.urlencode({'systemOutput':result})
headers = {"Content-type": "application/x-www-form-urlencoded", "Accept": "text/plain"}
conn = httplib.HTTPConnection(server)
conn.request("POST",path,params,headers)
response = conn.getresponse()
print response.status, response.reason
data = response.read()
print data
conn.close()
