#!/usr/bin/python3.2
import http.client
import json
import hashlib
import argparse
import urllib.parse
import sys
import os

ServerAddress = "10.154.100.101"
#ServerAddress = "172.17.100.1"
ChallengeWordPage = "/fred/dev/api.php?p=authAsk"
logsPage = "/fred/dev/api.php?p=logs&key="
DBTagsVersionNumber = 0;
logFilePath = "logFile.txt"
Password = ""
Timeout = 10
Port = 80

#Connection to server
conn = http.client.HTTPConnection(ServerAddress, Port, Timeout)

def saltAndHash(word):
	h = hashlib.sha256()
	temp = Password + word + Password
	h.update(temp.encode('utf-8'))
	return h.hexdigest()
	
conn.request("GET", ChallengeWordPage)
r1 = conn.getresponse()
	
#If return code is 200, the page was correctly loaded
if r1.status is 200:
	data1 = r1.read()  # This will return entire content.
		
	try:
		table = json.loads(data1.decode("utf-8")) #Convert data in string and decode JSON
	except ValueError:
		#print("Failed to decode json: " + data1)
		sys.exit(0)

	key = saltAndHash(table['mot'])
	logsPage += key
	try:
		logFile = open(logFilePath, "r")
	except IOError:
		sys.exit(2)
		
	
	fileContent = logFile.read()
	logFile.close()
	params = urllib.parse.urlencode({'logs': fileContent})
	headers = {"Content-type": "application/x-www-form-urlencoded", "Accept": "text/plain"}
	conn.request("POST", logsPage, params, headers)
	r2 = conn.getresponse()
	if r2.status is 200:
		data2 = r2.read()
		data2 = data2.decode("utf-8")
		if data2 == 'ok':
			os.remove(logFilePath)
			sys.exit(0)
		else:
			sys.exit(4)
	else:
		sys.exit(3)	

else:
	sys.exit(1)
sys.exit(0)
	
