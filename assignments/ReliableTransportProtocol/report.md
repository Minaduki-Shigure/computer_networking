# Programming Assignment: Reliable Transport Protocol

Copyright (c) 2020 Minaduki Shigure.  
南京大学 电子科学与工程学院 吴康正 171180571

## 实验环境

Microsoft Windows 10 Version 2004  
Microsoft Hyper-V Manager Version 10.0.19041.1  
Ubuntu 16.04 xenial w/ x86_64 Linux 4.4.0-142-generic  
mininet 2.3.0d6

## 实现方式

代码整体上使用了面向对象的思路构建发送和接收程序。使用typedef的方式定义了两个类，分别为发送者和接受者，对于每一个实体，都会各生成一个发送对象和一个接收对象，通过对象分别实现发送与接收的功能。

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

代码总共实现了2个初始化函数、2个辅助函数和5个关键函数。  

### 初始化函数

sender_init()和receiver_init()分别负责对发送方对象和接收方对象的初始化，将对象中的元素赋予初值。

### 辅助函数

1. 辅助函数cal_checksum()负责计算校验位的值，Checksum的计算采用了标准算法，即每16位相加然后取反。  
2. 辅助函数highlight_printf()负责部分终端输出的高亮，使用简单的ANSI控制码实现，不支持重定向检测。

### 关键函数

关键函数中，window_send()、sender_input()、sender_output()和sender_timerinterrupt()函数为发送类的函数，receiver_input()为接收类的函数。

要求中定义的packet格式中并没有标志位，而修改packet的结构体定义是一件很有风险的事情，因此程序使用了一个比较取巧的方法：如果包不是一个ACK包，就将其acknum置为-1，以此作为依据区分发出的数据包和ACK包。

#### sender_output()

收到从上层的消息后，发送方首先判断缓冲区是否还有空间，如果缓冲区已满，则直接丢弃，否则就将消息填入缓冲区顶部的包中，分配对应的seqnum，计算其校验位，然后将缓冲区顶部指针右移，调用窗口函数决定是否发送。

#### window_send()

每一次加入新的包后，发送方都会调用窗口函数，如果满足可以发送的条件，则会开始发送，发送前会将acknum置为-1以表示包不是ACK包，同时重新计算校验位，发送开始的同时启动定时器进行计时。

#### sender_input()

此函数负责处理收到的ACK包，首先在收到包的时候，会对包的内容进行校验，然后确认收到的ACK包是否为期望的ACK包，如果有任何一项不符合，则直接丢弃该包，认为是重发中产生的多余ACK包。如果收到的ACK包确为期望的ACK包，则认为其acknum对应的包已经被成功接收，窗口左侧指针（*事实上并不是个指针变量*）base向右滑动，如果所有已经发出的包都被收到了，则结束定时器，此次传输成功完成。

#### sender_timerinterrupt()

如果发送端的定时器超时，则可以判定可能有部分包损毁/丢失/因网络延时太高而无法及时到达，其中前两种情况肯定需要重发，但是实际上是无法区分这三种情况的，因此如果超时发生，则当前窗口内的所有包全部重新进行发送，同时为了避免由于网络延时导致的误判，当超时发生后，将estimatedRTT即定时器超时的值+10（最高不超过10000），以防止因为网络延时原因而反复发送。

> 这一段理论来说是可以复用window_send()函数的，但是实际运行中有一些莫名其妙的bug，所以索性重写了窗口发送。

#### receiver_input()

这个函数是用于接收并发送ACK的函数，处于对函数名整齐可读的考虑没有使用send_ack()作为函数名。  
首先在收到包的时候，会对包的内容进行校验，并且将seqnum与接收器内部存储的expecting_seq进行比较，已确定是否为所需要的包。由于链路不会打乱包的顺序，因此接收器不设缓冲区，直接判断。如果收到的包损毁或者seqnum不对

## 小结

