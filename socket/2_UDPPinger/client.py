from socket import *
import sys
import time

server = sys.argv[1]
serverPort = 12000

clientSocket = socket(AF_INET, SOCK_DGRAM)

clientSocket.settimeout(1)

time_sum = 0.0
alive_sum = 0
max_time = 0.0
min_time = 1.0

for i in range(0, 10):
    startTime = time.time()
    content = ('ping %d %s' % (i, startTime)).encode()

    try:
        clientSocket.sendto(content, (server, serverPort))
        response = clientSocket.recvfrom(1024)
        rtt = time.time() - startTime
        time_sum += rtt
        alive_sum += 1
        if rtt > max_time:
            max_time = rtt
        if rtt < min_time:
            min_time = rtt
        print ('from %s: udp_seq=%d rtt=%f s' % (server, i, rtt))
    except Exception as e:
        print ('Request timeout for udp_seq %d' % i)

average = float(time_sum / alive_sum)
loss = float((10 - alive_sum) * 10)
print ('--- %s ping statistics ---' % server)
print ('10 packets transmitted, {0} packets received, {1}% packet loss'.format(alive_sum, loss))
# print ('10 packets transmitted, %d packets received, %.1f \% packet loss' % (alive_sum, loss))    # This just do not work at all!
print ('rtt: min %f s, average %f s, max %f s' % (min_time, average, max_time))
clientSocket.close() # 关闭套接字