import httplib

conn = httplib.HTTPConnection("192.168.178.55")
conn.request("GET", "/?red=255&green=0&blue=0")
