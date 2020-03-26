import sys
import http.client
import urllib, parser

# Usage: python3 client.py <Server> <Port> <Document>
server = sys.argv[1]
port = int(sys.argv[2]) # Only accept integers
filename = sys.argv[3]

connection = http.client.HTTPConnection(server, port)
connection.request("GET", '/' + filename)
response = connection.getresponse()

print (response.status, response.reason)
print (response.headers)

f = open('./client-' + filename, "w+")
print(str(response.read(), encoding = "utf-8"), file = f)

connection.close()