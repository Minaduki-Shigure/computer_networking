# Programming Assignment: Reliable Transport Protocol

Copyright (c) 2020 Minaduki Shigure.  
南京大学 电子科学与工程学院 吴康正 171180571

## 实验环境

Microsoft Windows 10 Version 2004  
Microsoft Hyper-V Manager Version 10.0.19041.1  
Ubuntu 16.04 xenial w/ x86_64 Linux 4.4.0-142-generic  
mininet 2.3.0d6

## 实现方式

使用typedef的方式定义了两个类，分别为发送者和接受者，对于每一个实体，都会各生成一个发送对象和一个接收对象，通过对象分别实现发送与接收的功能。

各个类的结构如下：  

```c
typedef struct {
    int base;
    int next_seq;
    // int message_num;
    int window_size;
    int buffer_ptr; // Top of the buffer.
    struct pkt buffer[BUFFER_SIZE];
    double estimatedRTT;
} sender_class;
```

```c
typedef struct {
    int expecting_seq;
    struct pkt ack_buffer;
} receiver_class;
```

发送方的对象包含了当前发送的Seq，下个发送的Seq、当前的窗口大小（此实验中为固定值16）、当前缓冲区顶部位置、缓冲区和估计的RTT值，由发送方对象进行管理。接收方的对象包含了当前正在等待接收的包的Seq和一个存在缓冲区的ACK包，ACK包将会在初始化时被构建，而其包含的seqnum和acknum则会由接收方对象进行管理。

> 发送类和接收类的函数并没有在类中实现，这是因为C语言的伪类并没有访问控制的功能和this指针，使用函数指针的方式管理类函数也会加大代码的复杂度，没有必要多此一举。

Checksum的计算采用了标准算法，即每16位相加然后取反。使用cal_checksum()函数可以进行计算，以备发送或校验。


## 小结

