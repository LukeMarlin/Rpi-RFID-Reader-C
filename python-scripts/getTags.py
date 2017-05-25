from datetime import datetime
import http.client
import json
import hashlib
import argparse
import sys

#ServerAddress = "10.154.100.101"
ServerAddress = "172.17.100.1"
TagsVersionNumberPage="/fred/dev/api.php?p=badgeversion"
ChallengeWordPage = "/fred/dev/api.php?p=authAsk"
GetListPage = "/fred/dev/api.php?p=getAuthorizedTags&key="
DBTagsVersionNumber = 0;
userJSONid = "1"
clubJSONid = "2"
adminJSONid = "3"
userTagsFile = "userTags.txt"
clubTagsFile = "clubTags.txt"
adminTagsFile = "adminTags.txt"
Password = ""
Timeout = 10
Port = 80

parser = argparse.ArgumentParser()
parser.add_argument('-t', '--tagBaseVersion', help='Current tags base version. Used to avoid unnecessary loadings. Default=0')
args = parser.parse_args()
baseVersion = args.tagBaseVersion if args.tagBaseVersion else 0

#Connection to server
conn = http.client.HTTPConnection(ServerAddress, Port, Timeout)

#First step, we retrieve the current version number on DB
conn.request("GET", TagsVersionNumberPage)
r0 = conn.getresponse();
if r0.status is 200:
	DBTagsVersionNumber = r0.read().decode("utf-8")
else:
	#print("Failed to retrieve version number")
	sys.exit(0)
if baseVersion == 0 or baseVersion != DBTagsVersionNumber:

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
			sys.exit(4)
		key = saltAndHash(table['mot'])


		#Second request with key to get the RFID tag list
		conn.request("GET", GetListPage + key)
		r2 = conn.getresponse()

		#Return code 200 = OK
		if r2.status is 200:
			data2 = r2.read()
			try:
				RFIDtable = json.loads(data2.decode("utf-8")) #Convert data in string and decode JSON
			except ValueError:
				#print("Failed to decode json: " + data1)
				sys.exit(3)

			#Write each user tag in file
			try:
				f = open(userTagsFile,'w')
			except ValueError:
				sys.exit(5)

			for tag in RFIDtable[userJSONid]:
				f.write(str(tag) + '\n')
			f.close()

			#Write each club tag in file
			try:
				f = open(clubTagsFile,'w')
			except ValueError:
				sys.exit(5)

			for tag in RFIDtable[clubJSONid]:
				f.write(str(tag) + '\n')
			f.close()

			#Write each admin tag in file
			try:
				f = open(adminTagsFile,'w')
			except ValueError:
				sys.exit(5)

			for tag in RFIDtable[adminJSONid]:
				f.write(str(tag) + '\n')
			f.close()


		else:
			sys.exit(1)
			#print("Failed to get the RFID tag list")

	else:
		sys.exit(2)
		#print("Failed to get the challenge word")


conn.close()
sys.exit(0)
