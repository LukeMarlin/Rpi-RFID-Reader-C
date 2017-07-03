#!/usr/bin/python3.2
import pymssql
import sys
import os

logFilePath = "logFile.txt"

logLines = []
try:
    with open(logFilePath, 'r') as logFile:
        logLines = logFile.read().split(';')
except:
    sys.exit(1)

login = ""
password = ""

try:
    login = os.environ['tcpdblogin']
    password = os.environ['tcpdbpassword']
except:
    sys.exit(4)

try:
    with pymssql.connect(server="213.246.49.130", user=login, password=password, database="tcp_db") as conn:
        cur = conn.cursor()
        entries = [tuple(l.split(',')) for l in logLines if l != '']
        for entry in entries:
            cur.callproc("addLogEntry", entry)
        conn.commit()
except:
    sys.exit(2)

try:
    os.remove(logFilePath)
except:
    sys.exit(3)

sys.exit(0)
