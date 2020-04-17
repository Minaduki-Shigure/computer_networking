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
首先在收到包的时候，会对包的内容进行校验，并且将seqnum与接收器内部存储的expecting_seq进行比较，已确定是否为所需要的包。由于链路不会打乱包的顺序，因此接收器不设缓冲区，直接判断。如果收到的包完全正确，则将接收器的ACK包中acknum改为收到包的SEQ并发回，然后在终端上打印收到的内容，并更新接收方期望的seqnum值；如果收到的包重复，则直接丢弃不管，继续接收；如果包校验和不对或者seqnum超前，则认为发生了损毁或者丢包，发回的ACK仍为上一次的acknum，以等待发送端超时重发。

### 接口函数

接口函数即为测试函数会调用的函数，如A_input()等。  
由于采用了对象的方式，因此接口函数的篇幅不大，直接调用对应对象函数进行初始化、接收、重传等操作。  
唯一需要在接口函数中进行操作的是在实体接收到一个包时，判断包是数据包还是ACK包的函数，这里举A_input()为例，使用acknum判断包的类型，然后分别传入接收端或者发送端的input函数记性对应的处理，从而完成数据包的接收。

```c
void A_input(struct pkt packet)
{
    if (-1 == packet.acknum)
    {
        receiver_input(0, &receiver_A, packet);
    }
    {
        sender_input(0, &sender_A, packet);
    }
}
```

### Bonus实现

由于程序面向的是发送端对象而非接收端对象，因此对于不同的实体函数内容基本一样，只需要更改传入的对象即可。

## 实现效果

提供了Makefile文件用于编译和测试，也可以直接使用如下的指令进行测试：

```bash
./prog2 100 0.1 0.3 1000 2
```

即丢包率为0.1，损毁率为0.3，发送10个包（两个实体总共10个），结果（部分）如下：

```plain
minaduki@mininet-vm:~/computer_networking/assignments/ReliableTransportProtocol$ ./prog2 10 0.1 0.3 1000 2
-----  Stop and Wait Network Simulator Version 1.1 --------

the number of messages to simulate: 10
packet loss probability: 0.100000
packet corruption probability: 0.300000
average time between messages from sender's layer5: 1000.000000
TRACE: 2

EVENT time: 1870.573975,  type: 1, fromlayer5  entity: 1
B_sender_output: Added message. Allocated SEQ = 1.
window_send: Sending SEQ = 1.

EVENT time: 1877.939087,  type: 2, fromlayer3  entity: 0
A_receiver_input: Received packet SEQ = 1. Sending ACK = 1.
A_receiver_input: Content: aaaaaaaaaaaaaaaaaaa
          TOLAYER3: packet being corrupted
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 1886.575195,  type: 2, fromlayer3  entity: 1
B_sender_input: Packet corrupted. Dropping packet.

EVENT time: 1890.573975,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.

EVENT time: 1899.818481,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 1 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 1920.573975,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.
          TOLAYER3: packet being lost

EVENT time: 1960.573975,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.

EVENT time: 1962.052368,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 1 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 2010.573975,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.

EVENT time: 2015.846191,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 1 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 2070.573975,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.

EVENT time: 2074.776611,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 1 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 2140.573975,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.

EVENT time: 2143.304199,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 1 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 2220.573975,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.
          TOLAYER3: packet being corrupted

EVENT time: 2222.940430,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 1 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 2310.573975,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.
          TOLAYER3: packet being lost

EVENT time: 2410.573975,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.
          TOLAYER3: packet being lost

EVENT time: 2520.573975,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.

EVENT time: 2526.822998,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 1 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 2640.573975,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.

EVENT time: 2642.054688,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 1 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 2770.573975,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.

EVENT time: 2773.204590,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 1 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 2817.214111,  type: 1, fromlayer5  entity: 0
A_sender_output: Added message. Allocated SEQ = 1.
window_send: Sending SEQ = 1.

EVENT time: 2818.272461,  type: 2, fromlayer3  entity: 1
B_receiver_input: Received packet SEQ = 1. Sending ACK = 1.
B_receiver_input: Content: bbbbbbbbbbbbbbbbbbb
          TOLAYER3: packet being lost
B_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 2837.214111,  type: 0, timerinterrupt   entity: 0
A_timerinterrupt: Timeout for SEQ = 1. Resending.

EVENT time: 2842.488281,  type: 2, fromlayer3  entity: 1
B_receiver_input: Duplicated SEQ = 1 received. Dropped.
B_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 2867.214111,  type: 0, timerinterrupt   entity: 0
A_timerinterrupt: Timeout for SEQ = 1. Resending.
          TOLAYER3: packet being lost

EVENT time: 2907.214111,  type: 0, timerinterrupt   entity: 0
A_timerinterrupt: Timeout for SEQ = 1. Resending.

EVENT time: 2910.573975,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.
          TOLAYER3: packet being lost

EVENT time: 2915.995361,  type: 2, fromlayer3  entity: 1
B_receiver_input: Duplicated SEQ = 1 received. Dropped.
B_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 2957.214111,  type: 0, timerinterrupt   entity: 0
A_timerinterrupt: Timeout for SEQ = 1. Resending.
          TOLAYER3: packet being lost

EVENT time: 3017.214111,  type: 0, timerinterrupt   entity: 0
A_timerinterrupt: Timeout for SEQ = 1. Resending.
          TOLAYER3: packet being corrupted

EVENT time: 3022.033447,  type: 2, fromlayer3  entity: 1
B_receiver_input: Duplicated SEQ = 1 received. Dropped.
B_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 3060.573975,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.

EVENT time: 3064.235840,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 1 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 3087.214111,  type: 0, timerinterrupt   entity: 0
A_timerinterrupt: Timeout for SEQ = 1. Resending.
          TOLAYER3: packet being corrupted

EVENT time: 3096.125000,  type: 2, fromlayer3  entity: 1
B_receiver_input: Duplicated SEQ = 1 received. Dropped.
B_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 3167.214111,  type: 0, timerinterrupt   entity: 0
A_timerinterrupt: Timeout for SEQ = 1. Resending.
          TOLAYER3: packet being corrupted

EVENT time: 3172.831787,  type: 2, fromlayer3  entity: 1
B_receiver_input: Duplicated SEQ = 1 received. Dropped.
B_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 3220.573975,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.
          TOLAYER3: packet being corrupted

EVENT time: 3222.605713,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 1 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 3257.214111,  type: 0, timerinterrupt   entity: 0
A_timerinterrupt: Timeout for SEQ = 1. Resending.
          TOLAYER3: packet being corrupted

EVENT time: 3264.957031,  type: 2, fromlayer3  entity: 1
B_receiver_input: Duplicated SEQ = 1 received. Dropped.
B_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 3357.214111,  type: 0, timerinterrupt   entity: 0
A_timerinterrupt: Timeout for SEQ = 1. Resending.

EVENT time: 3363.905518,  type: 2, fromlayer3  entity: 1
B_receiver_input: Duplicated SEQ = 1 received. Dropped.
B_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 3390.573975,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.
          TOLAYER3: packet being corrupted

EVENT time: 3396.046631,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 1 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 3467.214111,  type: 0, timerinterrupt   entity: 0
A_timerinterrupt: Timeout for SEQ = 1. Resending.

EVENT time: 3472.601074,  type: 2, fromlayer3  entity: 1
B_receiver_input: Duplicated SEQ = 1 received. Dropped.
B_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 3570.573975,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.

EVENT time: 3572.278320,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 1 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 3587.214111,  type: 0, timerinterrupt   entity: 0
A_timerinterrupt: Timeout for SEQ = 1. Resending.
          TOLAYER3: packet being lost

EVENT time: 3717.214111,  type: 0, timerinterrupt   entity: 0
A_timerinterrupt: Timeout for SEQ = 1. Resending.

EVENT time: 3725.171875,  type: 2, fromlayer3  entity: 1
B_receiver_input: Duplicated SEQ = 1 received. Dropped.
B_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 3760.573975,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.

EVENT time: 3764.966553,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 1 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 3857.214111,  type: 0, timerinterrupt   entity: 0
A_timerinterrupt: Timeout for SEQ = 1. Resending.

EVENT time: 3862.879150,  type: 2, fromlayer3  entity: 1
B_receiver_input: Duplicated SEQ = 1 received. Dropped.
B_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 3960.573975,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.
          TOLAYER3: packet being corrupted

EVENT time: 3967.270752,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 1 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 4007.214111,  type: 0, timerinterrupt   entity: 0
A_timerinterrupt: Timeout for SEQ = 1. Resending.

EVENT time: 4016.702393,  type: 2, fromlayer3  entity: 1
B_receiver_input: Duplicated SEQ = 1 received. Dropped.
B_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 4167.213867,  type: 0, timerinterrupt   entity: 0
A_timerinterrupt: Timeout for SEQ = 1. Resending.
          TOLAYER3: packet being corrupted

EVENT time: 4169.719238,  type: 2, fromlayer3  entity: 1
B_sender_input: Packet corrupted. Dropping packet.

EVENT time: 4170.574219,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.
          TOLAYER3: packet being corrupted

EVENT time: 4178.354004,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 1 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 4337.213867,  type: 0, timerinterrupt   entity: 0
A_timerinterrupt: Timeout for SEQ = 1. Resending.
          TOLAYER3: packet being corrupted

EVENT time: 4343.114746,  type: 2, fromlayer3  entity: 1
B_receiver_input: Duplicated SEQ = 1 received. Dropped.
B_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 4390.574219,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.
          TOLAYER3: packet being corrupted

EVENT time: 4399.397461,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 1 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 4517.213867,  type: 0, timerinterrupt   entity: 0
A_timerinterrupt: Timeout for SEQ = 1. Resending.
          TOLAYER3: packet being corrupted

EVENT time: 4518.797852,  type: 2, fromlayer3  entity: 1
B_receiver_input: Duplicated SEQ = 1 received. Dropped.
B_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 4620.574219,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.

EVENT time: 4625.821289,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 1 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 4707.213867,  type: 0, timerinterrupt   entity: 0
A_timerinterrupt: Timeout for SEQ = 1. Resending.
          TOLAYER3: packet being corrupted

EVENT time: 4716.157715,  type: 2, fromlayer3  entity: 1
B_receiver_input: Duplicated SEQ = 1 received. Dropped.
B_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 4763.713867,  type: 1, fromlayer5  entity: 0
A_sender_output: Added message. Allocated SEQ = 2.
window_send: Sending SEQ = 2.
          TOLAYER3: packet being lost

EVENT time: 4860.574219,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.
          TOLAYER3: packet being lost

EVENT time: 4907.213867,  type: 0, timerinterrupt   entity: 0
A_timerinterrupt: Timeout for SEQ = 1. Resending.
          TOLAYER3: packet being corrupted
A_timerinterrupt: Timeout for SEQ = 2. Resending.
          TOLAYER3: packet being corrupted

EVENT time: 4910.210449,  type: 2, fromlayer3  entity: 1
B_receiver_input: Duplicated SEQ = 1 received. Dropped.
B_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 4911.359375,  type: 2, fromlayer3  entity: 1
B_sender_input: Packet corrupted. Dropping packet.

EVENT time: 5110.574219,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.

EVENT time: 5117.213867,  type: 0, timerinterrupt   entity: 0
A_timerinterrupt: Timeout for SEQ = 1. Resending.
A_timerinterrupt: Timeout for SEQ = 2. Resending.

EVENT time: 5118.142578,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 1 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 5121.387695,  type: 2, fromlayer3  entity: 1
B_receiver_input: Duplicated SEQ = 1 received. Dropped.
B_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 5128.989746,  type: 2, fromlayer3  entity: 1
B_receiver_input: Received packet SEQ = 2. Sending ACK = 2.
B_receiver_input: Content: ccccccccccccccccccc
B_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 5138.120605,  type: 2, fromlayer3  entity: 0
A_sender_input: Got ACK = 2.

EVENT time: 5370.574219,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.

EVENT time: 5374.952148,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 1 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 5529.646484,  type: 1, fromlayer5  entity: 1
B_sender_output: Added message. Allocated SEQ = 2.
window_send: Sending SEQ = 2.

EVENT time: 5535.588867,  type: 2, fromlayer3  entity: 0
A_receiver_input: Received packet SEQ = 2. Sending ACK = 2.
A_receiver_input: Content: ddddddddddddddddddd
          TOLAYER3: packet being lost
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 5640.574219,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.
B_timerinterrupt: Timeout for SEQ = 2. Resending.

EVENT time: 5647.405273,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 1 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 5656.605469,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 2 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 5920.574219,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.
B_timerinterrupt: Timeout for SEQ = 2. Resending.

EVENT time: 5929.923340,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 1 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 5933.185547,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 2 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 6210.574219,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.
B_timerinterrupt: Timeout for SEQ = 2. Resending.

EVENT time: 6217.374512,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 1 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 6219.862793,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 2 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 6405.025879,  type: 1, fromlayer5  entity: 0
A_sender_output: Added message. Allocated SEQ = 3.
window_send: Sending SEQ = 3.
          TOLAYER3: packet being corrupted

EVENT time: 6406.431641,  type: 2, fromlayer3  entity: 1
B_receiver_input: Received packet SEQ = 3. Sending ACK = 3.
B_receiver_input: Content: Zeeeeeeeeeeeeeeeeee
          TOLAYER3: packet being lost
B_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 6510.574219,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.
          TOLAYER3: packet being corrupted
B_timerinterrupt: Timeout for SEQ = 2. Resending.

EVENT time: 6520.288574,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 1 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 6523.303223,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 2 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 6625.025879,  type: 0, timerinterrupt   entity: 0
A_timerinterrupt: Timeout for SEQ = 3. Resending.

EVENT time: 6630.758789,  type: 2, fromlayer3  entity: 1
B_receiver_input: Duplicated SEQ = 3 received. Dropped.
B_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 6820.574219,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.
B_timerinterrupt: Timeout for SEQ = 2. Resending.
          TOLAYER3: packet being lost

EVENT time: 6830.178711,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 1 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 6855.025879,  type: 0, timerinterrupt   entity: 0
A_timerinterrupt: Timeout for SEQ = 3. Resending.

EVENT time: 6863.607422,  type: 2, fromlayer3  entity: 1
B_receiver_input: Duplicated SEQ = 3 received. Dropped.
B_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 7095.025879,  type: 0, timerinterrupt   entity: 0
A_timerinterrupt: Timeout for SEQ = 3. Resending.

EVENT time: 7098.439941,  type: 2, fromlayer3  entity: 1
B_receiver_input: Duplicated SEQ = 3 received. Dropped.
B_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 7140.574219,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.
          TOLAYER3: packet being corrupted
B_timerinterrupt: Timeout for SEQ = 2. Resending.
          TOLAYER3: packet being lost

EVENT time: 7145.767090,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 1 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 7345.025879,  type: 0, timerinterrupt   entity: 0
A_timerinterrupt: Timeout for SEQ = 3. Resending.
          TOLAYER3: packet being corrupted

EVENT time: 7349.457031,  type: 2, fromlayer3  entity: 1
B_receiver_input: Duplicated SEQ = 3 received. Dropped.
B_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 7470.574219,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.
          TOLAYER3: packet being lost
B_timerinterrupt: Timeout for SEQ = 2. Resending.
          TOLAYER3: packet being corrupted

EVENT time: 7479.569824,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 2 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 7605.025879,  type: 0, timerinterrupt   entity: 0
A_timerinterrupt: Timeout for SEQ = 3. Resending.

EVENT time: 7612.045898,  type: 2, fromlayer3  entity: 1
B_receiver_input: Duplicated SEQ = 3 received. Dropped.
B_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 7628.443359,  type: 1, fromlayer5  entity: 1
B_sender_output: Added message. Allocated SEQ = 3.
window_send: Sending SEQ = 3.
          TOLAYER3: packet being lost

EVENT time: 7645.067383,  type: 1, fromlayer5  entity: 1
B_sender_output: Added message. Allocated SEQ = 4.
window_send: Sending SEQ = 4.
          TOLAYER3: packet being lost

EVENT time: 7810.574219,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 1. Resending.
          TOLAYER3: packet being lost
B_timerinterrupt: Timeout for SEQ = 2. Resending.
B_timerinterrupt: Timeout for SEQ = 3. Resending.
          TOLAYER3: packet being corrupted
B_timerinterrupt: Timeout for SEQ = 4. Resending.

EVENT time: 7816.737305,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 2 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 7822.151855,  type: 2, fromlayer3  entity: 0
A_receiver_input: Packet corrupted. Sending last ACK = 2.
A_sender_input: Packet corrupted. Dropping packet.

EVENT time: 7828.472168,  type: 2, fromlayer3  entity: 0
A_receiver_input: Unexpected SEQ = 4 received. Sending last ACK = 2.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 7829.170410,  type: 2, fromlayer3  entity: 1
B_sender_input: Got ACK = 2.

EVENT time: 7837.292969,  type: 2, fromlayer3  entity: 1
B_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 7875.025879,  type: 0, timerinterrupt   entity: 0
A_timerinterrupt: Timeout for SEQ = 3. Resending.

EVENT time: 7884.396484,  type: 2, fromlayer3  entity: 1
B_receiver_input: Duplicated SEQ = 3 received. Dropped.
B_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 8155.025879,  type: 0, timerinterrupt   entity: 0
A_timerinterrupt: Timeout for SEQ = 3. Resending.

EVENT time: 8158.062988,  type: 2, fromlayer3  entity: 1
B_receiver_input: Duplicated SEQ = 3 received. Dropped.
B_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 8160.574219,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 3. Resending.
B_timerinterrupt: Timeout for SEQ = 4. Resending.

EVENT time: 8168.292480,  type: 2, fromlayer3  entity: 0
A_receiver_input: Received packet SEQ = 3. Sending ACK = 3.
A_receiver_input: Content: fffffffffffffffffff
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 8176.425293,  type: 2, fromlayer3  entity: 0
A_receiver_input: Received packet SEQ = 4. Sending ACK = 4.
A_receiver_input: Content: ggggggggggggggggggg
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 8176.444336,  type: 2, fromlayer3  entity: 1
B_sender_input: Got ACK = 3.

EVENT time: 8180.759277,  type: 2, fromlayer3  entity: 1
B_sender_input: Got ACK = 4.

EVENT time: 8445.025391,  type: 0, timerinterrupt   entity: 0
A_timerinterrupt: Timeout for SEQ = 3. Resending.

EVENT time: 8453.754883,  type: 2, fromlayer3  entity: 1
B_receiver_input: Duplicated SEQ = 3 received. Dropped.
B_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 8745.025391,  type: 0, timerinterrupt   entity: 0
A_timerinterrupt: Timeout for SEQ = 3. Resending.
          TOLAYER3: packet being corrupted

EVENT time: 8754.703125,  type: 2, fromlayer3  entity: 1
B_receiver_input: Duplicated SEQ = 3 received. Dropped.
B_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 9036.344727,  type: 1, fromlayer5  entity: 0
A_sender_output: Added message. Allocated SEQ = 4.
window_send: Sending SEQ = 4.

EVENT time: 9043.182617,  type: 2, fromlayer3  entity: 1
B_receiver_input: Received packet SEQ = 4. Sending ACK = 4.
B_receiver_input: Content: hhhhhhhhhhhhhhhhhhh
B_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 9046.638672,  type: 2, fromlayer3  entity: 0
A_sender_input: Got ACK = 4.

EVENT time: 9267.890625,  type: 1, fromlayer5  entity: 1
B_sender_output: Added message. Allocated SEQ = 5.
window_send: Sending SEQ = 5.

EVENT time: 9270.275391,  type: 2, fromlayer3  entity: 0
A_receiver_input: Received packet SEQ = 5. Sending ACK = 5.
A_receiver_input: Content: iiiiiiiiiiiiiiiiiii
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 9280.012695,  type: 2, fromlayer3  entity: 1
B_sender_input: Got ACK = 5.

EVENT time: 9958.363281,  type: 1, fromlayer5  entity: 1
B_sender_output: Added message. Allocated SEQ = 6.
window_send: Sending SEQ = 6.

EVENT time: 9963.712891,  type: 2, fromlayer3  entity: 0
A_receiver_input: Received packet SEQ = 6. Sending ACK = 6.
A_receiver_input: Content: jjjjjjjjjjjjjjjjjjj
          TOLAYER3: packet being lost
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 10318.363281,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 6. Resending.
          TOLAYER3: packet being lost

EVENT time: 10688.363281,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 6. Resending.

EVENT time: 10693.135742,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 6 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 11068.363281,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 6. Resending.

EVENT time: 11071.865234,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 6 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 11458.363281,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 6. Resending.
          TOLAYER3: packet being corrupted

EVENT time: 11461.542969,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 6 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 11858.363281,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 6. Resending.

EVENT time: 11866.335938,  type: 2, fromlayer3  entity: 0
A_receiver_input: Duplicated SEQ = 6 received. Dropped.
A_sender_input: Received duplicate ACK. Packet ignored.

EVENT time: 12268.363281,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 6. Resending.
          TOLAYER3: packet being lost

EVENT time: 12688.363281,  type: 0, timerinterrupt   entity: 1
B_timerinterrupt: Timeout for SEQ = 6. Resending.
          TOLAYER3: packet being corrupted

EVENT time: 12690.823242,  type: 2, fromlayer3  entity: 0
A_receiver_input: Packet corrupted. Sending last ACK = 6.
A_sender_input: Packet corrupted. Dropping packet.

EVENT time: 12693.951172,  type: 2, fromlayer3  entity: 1
B_sender_input: Got ACK = 6.
 Simulator terminated at time 12693.951172
 after sending 10 msgs from layer5

```

## 小结

通过此次试验，充分了解了可靠传输协议的基本实现方式，对网络传输层有了基本的认识。