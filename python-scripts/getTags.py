#!/usr/bin/python3.2
import pymssql
from datetime import datetime
import argparse
import sys
import os

userTagsFile = "userTags.txt"
userPlusTagsFile = "userPlusTags.txt"
adminTagsFile = "adminTags.txt"

parser = argparse.ArgumentParser()
parser.add_argument('-t', '--tagBaseVersion', help='Current tags base version. Used to avoid unnecessary loadings. Default=0')
args = parser.parse_args()
baseVersion = args.tagBaseVersion if args.tagBaseVersion else 0
login = "";
pwd = "";

try:
    login = os.environ['tcpdblogin']
    password = os.environ['tcpdbpassword']
except:
    sys.exit(3)


try:
    with pymssql.connect(server="213.246.49.130", user=login, password=password, database="tcp_db") as conn:
        cur = conn.cursor()
        cur.execute("select versionNumber from AuthorizedTagsVersion")
        new_version = cur.fetchone()[0]

        if baseVersion == new_version:
            sys.exit(0)

        cur.execute("""select b.number, p.status_id from BadgeJeu as b
                        JOIN PlayerJeu as p
                        On b.player_ID = p.ID
                        where b.IsEnabled = 1;""")

        try:
            with open(userTagsFile, 'w') as userFile, open(userPlusTagsFile, 'w') as userPlusFile, open(adminTagsFile, 'w') as adminFile:

                for row in cur:
                    if row[1] == 1:
                        userFile.write(str(row[0]) + '\n')
                    if row[1] == 2:
                        userPlusFile.write(str(row[0]) + '\n')
                    if row[1] == 3:
                        adminFile.write(str(row[0]) + '\n')
        except:
            sys.exit(2)
except:
    sys.exit(1)

sys.exit(0)
