# Socket Programming: UDP Pinger

Copyright (c) 2020 Minaduki Shigure.  
南京大学 电子科学与工程学院 吴康正 171180571

## 实验环境

macOS “Mojave” 10.14.5  
Broadcom BCM4352 Wireless Network Adapter  
Wireshark Version 3.2.2 (v3.2.2-0-ga3efece3d640)  
Safari Version 12.1.1 (14607.2.6.1.1)

## 实验结果

实现了通过UDP的ping操作，同时可以显示ping的统计信息：

```plain
Minadukis-MacBook-Pro:2_UDPPinger minaduki$ python3 client.py 127.0.0.1
from 127.0.0.1: udp_seq=0 rtt=0.000162 s
Request timeout for udp_seq 1
from 127.0.0.1: udp_seq=2 rtt=0.000276 s
Request timeout for udp_seq 3
from 127.0.0.1: udp_seq=4 rtt=0.000547 s
from 127.0.0.1: udp_seq=5 rtt=0.000254 s
Request timeout for udp_seq 6
from 127.0.0.1: udp_seq=7 rtt=0.000393 s
Request timeout for udp_seq 8
from 127.0.0.1: udp_seq=9 rtt=0.000383 s
--- 127.0.0.1 ping statistics ---
10 packets transmitted, 6 packets received, 40.0% packet loss
rtt: min 0.000162 s, average 0.000336 s, max 0.000547 s
```

## 小结

加深了对UDP协议的理解，同时掌握了使用UDP通讯的方法。
