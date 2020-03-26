# Socket Programming: SMTP Server

Copyright (c) 2020 Minaduki Shigure.  
南京大学 电子科学与工程学院 吴康正 171180571

## 实验环境

macOS “Mojave” 10.14.5  
Broadcom BCM4352 Wireless Network Adapter  
Wireshark Version 3.2.2 (v3.2.2-0-ga3efece3d640)  
Safari Version 12.1.1 (14607.2.6.1.1)

## 实验结果

实现了使用SMTP协议的电子邮件发送：
```
Minadukis-MacBook-Pro:3_SMTPClient minaduki$ python3 client.py 
220 smtp.qq.com Esmtp QQ Mail Server

250 smtp.qq.com

334 VXNlcm5hbWU6

334 UGFzc3dvcmQ6

235 Authentication successful

250 Ok

250 Ok

354 End data with <CR><LF>.<CR><LF>

250 Ok: queued as 

221 Bye

```

![Mail received](./pic/3-1.png)

## 小结

本次实验很可惜没有完成拓展内容，对于SSL/TLS加密，QQ邮箱只支持SSL，但是我在网上找到的参考资料都是使用TLS的，而我在使用telnet时也没能成功，因此只能放弃。  
对于多媒体内容，似乎我对content-type的理解有误，图片发送出去显示的仍旧是base64编码，也许可以尝试发送一个html格式的邮件？