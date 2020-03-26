from socket import *
import credentials
import base64

subject = "From Minaduki"
msg = "\r\n I love computer networks!"
endmsg = "\r\n.\r\n"

imgPath = './Player_Minaduki.jpg'
imgFile = open(imgPath, 'rb').read()
img = base64.b64encode(imgFile)

srcaddr = 'i@minaduki.cn'
dstaddr = '171180571@smail.nju.edu.cn'

username, password = credentials.base64_output()

# Choose a mail server (e.g. Google mail server) and call it mailserver 
mailserver = 'smtp.exmail.qq.com'
# Create socket called clientSocket and establish a TCP connection with mailserver
clientSocket = socket(AF_INET, SOCK_STREAM)
clientSocket.connect((mailserver, 25))

recv = clientSocket.recv(1024).decode() 
print(recv)
if recv[:3] != '220':
    print('220 reply not received from server.')

# Send HELO command and print server response. 
heloCommand = 'HELO Minaduki\r\n'
clientSocket.send(heloCommand.encode())
recv1 = clientSocket.recv(1024).decode()
print(recv1)
if recv1[:3] != '250':
    print('250 reply not received from server.')

# Authorize
authCommand = 'AUTH LOGIN\r\n'
clientSocket.send(authCommand.encode())
recv2 = clientSocket.recv(1024).decode()
print(recv2)
if (recv2[:3] != '334'):
	print('334 reply not received from server')

clientSocket.send((username + '\r\n').encode())
recv3 = clientSocket.recv(1024).decode()
print(recv3)
if (recv3[:3] != '334'):
	print('334 reply not received from server')

clientSocket.send((password + '\r\n').encode())
recv4 = clientSocket.recv(1024).decode()
print(recv4)
if (recv4[:3] != '235'):
	print('235 reply not received from server')

# Send MAIL FROM command and print server response.
mailfrom = 'MAIL FROM: <' + srcaddr + '>\r\n'
clientSocket.send(mailfrom.encode())
recv5 = clientSocket.recv(1024).decode()
print(recv5)
if (recv5[:3] != '250'):
	print('250 reply not received from server')

# Send RCPT TO command and print server response.
rcptto = 'RCPT TO: <' + dstaddr + '>\r\n'
clientSocket.send(rcptto.encode())
recv6 = clientSocket.recv(1024).decode()
print(recv6)
if (recv6[:3] != '250'):
	print('250 reply not received from server')

# Send DATA command and print server response.
dataCommand = 'DATA\r\n'
clientSocket.send(dataCommand.encode())
recv7 = clientSocket.recv(1024).decode()
print(recv7)
if (recv7[:3] != '354'):
	print('354 reply not received from server')

# Send message data.
header = 'FROM:' + srcaddr + '\r\n' + 'TO:' + dstaddr + '\r\n' + 'SUBJECT:' + subject + '\r\n'
message = header + msg;
clientSocket.send(message.encode())

# Send message data.

# header = 'FROM:' + srcaddr + '\r\n' + 'TO:' + dstaddr + '\r\n' + 'SUBJECT:' + subject + '\r\n'
# text_content_header = 'Content-Type:multipart/mixed;boundray="simple"\r\n\n\n--simple\nContent-Type:text/plain\r\n'
# img_content_header = '\n--simple\nContent-Type:image/JPEG\nContent-transfer-encoding:base64\r\n'
# eofflag = '\n--simple\r\n'
# message = header.encode() + text_content_header.encode() + msg.encode() + img_content_header.encode() + img + eofflag.encode();
# clientSocket.send(message)

# Message ends with a single period.
clientSocket.send(endmsg.encode())
recv8 = clientSocket.recv(1024).decode()
print(recv8)
if recv8[:3] != '250':
	print('250 reply not received from server.')

# Send QUIT command and get server response.
quitCommand = 'QUIT\r\n'
clientSocket.send(quitCommand.encode())
recv9 = clientSocket.recv(1024).decode()
print(recv9)
if (recv9[:3] != '221'):
	print('221 reply not received from server')
