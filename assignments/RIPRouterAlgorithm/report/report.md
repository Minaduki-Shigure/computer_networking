# Programming Assignment: RIP Router Algorithm

Copyright (c) 2020 Minaduki Shigure.  
南京大学 电子科学与工程学院 吴康正 171180571

## 实验环境

Microsoft Windows 10 Version 2004  
Microsoft Hyper-V Manager Version 10.0.19041.1  
Ubuntu 16.04 xenial w/ x86_64 Linux 4.4.0-142-generic  
mininet 2.3.0d6

## 实现方式

prog3.c中间exit函数调用需要标准库和提供一个参数
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
minaduki@mininet-vm:~/computer_networking/assignments/RIPRouterAlgorithm$ ./prog3 
Enter TRACE:2
Time = 0.000. rtinit0() has been called.
Time = 0.000. Node 0 sent packet to Node 1 : 0 1 3 7
Time = 0.000. Node 0 sent packet to Node 2 : 0 1 3 7
Time = 0.000. Node 0 sent packet to Node 3 : 0 1 3 7
Time = 0.000. rtinit1() has been called.
Time = 0.000. Node 1 sent packet to Node 0 : 1 0 1 999
Time = 0.000. Node 1 sent packet to Node 2 : 1 0 1 999
Time = 0.000. rtinit2() has been called.
Time = 0.000. Node 2 sent packet to Node 0 : 3 1 0 2
Time = 0.000. Node 2 sent packet to Node 1 : 3 1 0 2
Time = 0.000. Node 2 sent packet to Node 3 : 3 1 0 2
Time = 0.000. rtinit3() has been called.
Time = 0.000. Node 3 sent packet to Node 0 : 7 999 2 0
Time = 0.000. Node 3 sent packet to Node 2 : 7 999 2 0
MAIN: rcv event, t=0.947, at 3 src: 0, dest: 3, contents:   0   1   3   7
Time = 0.947. rtupdate3() has been called.
Time = 0.947. Node 3 received packet from Node 0.
Time = 0.947. Distance Table for Node 3 has been modified!
  dest|     via     
   D3 |    0    2 
  ----|----------
     0|    7  999
     1|    8  999
     2|   10    2
Time = 0.947. Node 3 sent packet to Node 0 : 7 8 2 0
Time = 0.947. Node 3 sent packet to Node 2 : 7 8 2 0
MAIN: rcv event, t=0.992, at 0 src: 1, dest: 0, contents:   1   0   1 999
Time = 0.992. rtupdate0() has been called.
Time = 0.992. Node 0 received packet from Node 1.
Time = 0.992. Distance Table for Node 0 has been modified!
  dest|     via     
   D0 |    1    2    3 
  ----|---------------
     1|    1  999  999
     2|    2    3  999
     3|  1000  999    7
Time = 0.992. Node 0 sent packet to Node 1 : 0 1 2 7
Time = 0.992. Node 0 sent packet to Node 2 : 0 1 2 7
Time = 0.992. Node 0 sent packet to Node 3 : 0 1 2 7
MAIN: rcv event, t=1.209, at 3 src: 2, dest: 3, contents:   3   1   0   2
Time = 1.209. rtupdate3() has been called.
Time = 1.209. Node 3 received packet from Node 2.
Time = 1.209. Distance Table for Node 3 has been modified!
  dest|     via     
   D3 |    0    2 
  ----|----------
     0|    7    5
     1|    8    3
     2|   10    2
Time = 1.209. Node 3 sent packet to Node 0 : 5 3 2 0
Time = 1.209. Node 3 sent packet to Node 2 : 5 3 2 0
MAIN: rcv event, t=1.276, at 3 src: 0, dest: 3, contents:   0   1   2   7
Time = 1.276. rtupdate3() has been called.
Time = 1.276. Node 3 received packet from Node 0.
Time = 1.276. Distance Table for Node 3 has been modified!
  dest|     via     
   D3 |    0    2 
  ----|----------
     0|    7    5
     1|    8    3
     2|    9    2
MAIN: rcv event, t=1.642, at 2 src: 0, dest: 2, contents:   0   1   3   7
Time = 1.642. rtupdate2() has been called.
Time = 1.642. Node 2 received packet from Node 0.
Time = 1.642. Distance Table for Node 2 has been modified!
  dest|     via     
   D2 |    0    1    3 
  ----|---------------
     0|    3  999  999
     1|    4    1  999
     3|   10  999    2
MAIN: rcv event, t=1.871, at 1 src: 0, dest: 1, contents:   0   1   3   7
Time = 1.871. rtupdate1() has been called.
Time = 1.871. Node 1 received packet from Node 0.
Time = 1.871. Distance Table for Node 1 has been modified!
  dest|     via     
   D1 |    0    2 
  ----|----------
     0|    1  999
     2|    4    1
     3|    8  999
Time = 1.871. Node 1 sent packet to Node 0 : 1 0 1 8
Time = 1.871. Node 1 sent packet to Node 2 : 1 0 1 8
MAIN: rcv event, t=2.166, at 2 src: 1, dest: 2, contents:   1   0   1 999
Time = 2.166. rtupdate2() has been called.
Time = 2.166. Node 2 received packet from Node 1.
Time = 2.166. Distance Table for Node 2 has been modified!
  dest|     via     
   D2 |    0    1    3 
  ----|---------------
     0|    3    2  999
     1|    4    1  999
     3|   10  1000    2
Time = 2.166. Node 2 sent packet to Node 0 : 2 1 0 2
Time = 2.166. Node 2 sent packet to Node 1 : 2 1 0 2
Time = 2.166. Node 2 sent packet to Node 3 : 2 1 0 2
MAIN: rcv event, t=2.407, at 0 src: 2, dest: 0, contents:   3   1   0   2
Time = 2.407. rtupdate0() has been called.
Time = 2.407. Node 0 received packet from Node 2.
Time = 2.407. Distance Table for Node 0 has been modified!
  dest|     via     
   D0 |    1    2    3 
  ----|---------------
     1|    1    4  999
     2|    2    3  999
     3|  1000    5    7
Time = 2.407. Node 0 sent packet to Node 1 : 0 1 2 5
Time = 2.407. Node 0 sent packet to Node 2 : 0 1 2 5
Time = 2.407. Node 0 sent packet to Node 3 : 0 1 2 5
MAIN: rcv event, t=2.421, at 2 src: 3, dest: 2, contents:   7 999   2   0
Time = 2.421. rtupdate2() has been called.
Time = 2.421. Node 2 received packet from Node 3.
Time = 2.421. Distance Table for Node 2 has been modified!
  dest|     via     
   D2 |    0    1    3 
  ----|---------------
     0|    3    2    9
     1|    4    1  1001
     3|   10  1000    2
MAIN: rcv event, t=2.811, at 1 src: 2, dest: 1, contents:   3   1   0   2
Time = 2.811. rtupdate1() has been called.
Time = 2.811. Node 1 received packet from Node 2.
Time = 2.811. Distance Table for Node 1 has been modified!
  dest|     via     
   D1 |    0    2 
  ----|----------
     0|    1    4
     2|    4    1
     3|    8    3
Time = 2.811. Node 1 sent packet to Node 0 : 1 0 1 3
Time = 2.811. Node 1 sent packet to Node 2 : 1 0 1 3
MAIN: rcv event, t=3.293, at 2 src: 3, dest: 2, contents:   7   8   2   0
Time = 3.293. rtupdate2() has been called.
Time = 3.293. Node 2 received packet from Node 3.
Time = 3.293. Distance Table for Node 2 has been modified!
  dest|     via     
   D2 |    0    1    3 
  ----|---------------
     0|    3    2    9
     1|    4    1   10
     3|   10  1000    2
MAIN: rcv event, t=3.602, at 3 src: 2, dest: 3, contents:   2   1   0   2
Time = 3.602. rtupdate3() has been called.
Time = 3.602. Node 3 received packet from Node 2.
Time = 3.602. Distance Table for Node 3 has been modified!
  dest|     via     
   D3 |    0    2 
  ----|----------
     0|    7    4
     1|    8    3
     2|    9    2
Time = 3.602. Node 3 sent packet to Node 0 : 4 3 2 0
Time = 3.602. Node 3 sent packet to Node 2 : 4 3 2 0
MAIN: rcv event, t=4.063, at 2 src: 0, dest: 2, contents:   0   1   2   7
Time = 4.063. rtupdate2() has been called.
Time = 4.063. Node 2 received packet from Node 0.
Time = 4.063. Distance Table for Node 2 has been modified!
  dest|     via     
   D2 |    0    1    3 
  ----|---------------
     0|    3    2    9
     1|    4    1   10
     3|   10  1000    2
MAIN: rcv event, t=4.104, at 0 src: 3, dest: 0, contents:   7 999   2   0
Time = 4.104. rtupdate0() has been called.
Time = 4.104. Node 0 received packet from Node 3.
Time = 4.104. Distance Table for Node 0 has been modified!
  dest|     via     
   D0 |    1    2    3 
  ----|---------------
     1|    1    4  1006
     2|    2    3    9
     3|  1000    5    7
MAIN: rcv event, t=4.169, at 2 src: 3, dest: 2, contents:   5   3   2   0
Time = 4.169. rtupdate2() has been called.
Time = 4.169. Node 2 received packet from Node 3.
Time = 4.169. Distance Table for Node 2 has been modified!
  dest|     via     
   D2 |    0    1    3 
  ----|---------------
     0|    3    2    7
     1|    4    1    5
     3|   10  1000    2
MAIN: rcv event, t=4.330, at 0 src: 3, dest: 0, contents:   7   8   2   0
Time = 4.330. rtupdate0() has been called.
Time = 4.330. Node 0 received packet from Node 3.
Time = 4.330. Distance Table for Node 0 has been modified!
  dest|     via     
   D0 |    1    2    3 
  ----|---------------
     1|    1    4   15
     2|    2    3    9
     3|  1000    5    7
MAIN: rcv event, t=4.643, at 1 src: 0, dest: 1, contents:   0   1   2   7
Time = 4.643. rtupdate1() has been called.
Time = 4.643. Node 1 received packet from Node 0.
Time = 4.643. Distance Table for Node 1 has been modified!
  dest|     via     
   D1 |    0    2 
  ----|----------
     0|    1    4
     2|    3    1
     3|    8    3
MAIN: rcv event, t=5.213, at 0 src: 3, dest: 0, contents:   5   3   2   0
Time = 5.213. rtupdate0() has been called.
Time = 5.213. Node 0 received packet from Node 3.
Time = 5.213. Distance Table for Node 0 has been modified!
  dest|     via     
   D0 |    1    2    3 
  ----|---------------
     1|    1    4   10
     2|    2    3    9
     3|  1000    5    7
MAIN: rcv event, t=5.384, at 3 src: 0, dest: 3, contents:   0   1   2   5
Time = 5.384. rtupdate3() has been called.
Time = 5.384. Node 3 received packet from Node 0.
Time = 5.384. Distance Table for Node 3 has been modified!
  dest|     via     
   D3 |    0    2 
  ----|----------
     0|    7    4
     1|    8    3
     2|    9    2
MAIN: rcv event, t=5.820, at 1 src: 2, dest: 1, contents:   2   1   0   2
Time = 5.820. rtupdate1() has been called.
Time = 5.820. Node 1 received packet from Node 2.
Time = 5.820. Distance Table for Node 1 has been modified!
  dest|     via     
   D1 |    0    2 
  ----|----------
     0|    1    3
     2|    3    1
     3|    8    3
MAIN: rcv event, t=6.042, at 2 src: 1, dest: 2, contents:   1   0   1   8
Time = 6.042. rtupdate2() has been called.
Time = 6.042. Node 2 received packet from Node 1.
Time = 6.042. Distance Table for Node 2 has been modified!
  dest|     via     
   D2 |    0    1    3 
  ----|---------------
     0|    3    2    7
     1|    4    1    5
     3|   10    9    2
MAIN: rcv event, t=6.071, at 0 src: 1, dest: 0, contents:   1   0   1   8
Time = 6.071. rtupdate0() has been called.
Time = 6.071. Node 0 received packet from Node 1.
Time = 6.071. Distance Table for Node 0 has been modified!
  dest|     via     
   D0 |    1    2    3 
  ----|---------------
     1|    1    4   10
     2|    2    3    9
     3|    9    5    7
MAIN: rcv event, t=6.532, at 1 src: 0, dest: 1, contents:   0   1   2   5
Time = 6.532. rtupdate1() has been called.
Time = 6.532. Node 1 received packet from Node 0.
Time = 6.532. Distance Table for Node 1 has been modified!
  dest|     via     
   D1 |    0    2 
  ----|----------
     0|    1    3
     2|    3    1
     3|    6    3
MAIN: rcv event, t=7.021, at 0 src: 2, dest: 0, contents:   2   1   0   2
Time = 7.021. rtupdate0() has been called.
Time = 7.021. Node 0 received packet from Node 2.
Time = 7.021. Distance Table for Node 0 has been modified!
  dest|     via     
   D0 |    1    2    3 
  ----|---------------
     1|    1    4   10
     2|    2    3    9
     3|    9    5    7
MAIN: rcv event, t=7.160, at 2 src: 0, dest: 2, contents:   0   1   2   5
Time = 7.160. rtupdate2() has been called.
Time = 7.160. Node 2 received packet from Node 0.
Time = 7.160. Distance Table for Node 2 has been modified!
  dest|     via     
   D2 |    0    1    3 
  ----|---------------
     0|    3    2    7
     1|    4    1    5
     3|    8    9    2
MAIN: rcv event, t=7.405, at 0 src: 1, dest: 0, contents:   1   0   1   3
Time = 7.405. rtupdate0() has been called.
Time = 7.405. Node 0 received packet from Node 1.
Time = 7.405. Distance Table for Node 0 has been modified!
  dest|     via     
   D0 |    1    2    3 
  ----|---------------
     1|    1    4   10
     2|    2    3    9
     3|    4    5    7
Time = 7.405. Node 0 sent packet to Node 1 : 0 1 2 4
Time = 7.405. Node 0 sent packet to Node 2 : 0 1 2 4
Time = 7.405. Node 0 sent packet to Node 3 : 0 1 2 4
MAIN: rcv event, t=7.579, at 3 src: 0, dest: 3, contents:   0   1   2   4
Time = 7.579. rtupdate3() has been called.
Time = 7.579. Node 3 received packet from Node 0.
Time = 7.579. Distance Table for Node 3 has been modified!
  dest|     via     
   D3 |    0    2 
  ----|----------
     0|    7    4
     1|    8    3
     2|    9    2
MAIN: rcv event, t=7.941, at 1 src: 0, dest: 1, contents:   0   1   2   4
Time = 7.941. rtupdate1() has been called.
Time = 7.941. Node 1 received packet from Node 0.
Time = 7.941. Distance Table for Node 1 has been modified!
  dest|     via     
   D1 |    0    2 
  ----|----------
     0|    1    3
     2|    3    1
     3|    5    3
MAIN: rcv event, t=8.086, at 0 src: 3, dest: 0, contents:   4   3   2   0
Time = 8.086. rtupdate0() has been called.
Time = 8.086. Node 0 received packet from Node 3.
Time = 8.086. Distance Table for Node 0 has been modified!
  dest|     via     
   D0 |    1    2    3 
  ----|---------------
     1|    1    4   10
     2|    2    3    9
     3|    4    5    7
MAIN: rcv event, t=8.639, at 2 src: 1, dest: 2, contents:   1   0   1   3
Time = 8.639. rtupdate2() has been called.
Time = 8.639. Node 2 received packet from Node 1.
Time = 8.639. Distance Table for Node 2 has been modified!
  dest|     via     
   D2 |    0    1    3 
  ----|---------------
     0|    3    2    7
     1|    4    1    5
     3|    8    4    2
MAIN: rcv event, t=8.943, at 2 src: 3, dest: 2, contents:   4   3   2   0
Time = 8.943. rtupdate2() has been called.
Time = 8.943. Node 2 received packet from Node 3.
Time = 8.943. Distance Table for Node 2 has been modified!
  dest|     via     
   D2 |    0    1    3 
  ----|---------------
     0|    3    2    6
     1|    4    1    5
     3|    8    4    2
MAIN: rcv event, t=9.960, at 2 src: 0, dest: 2, contents:   0   1   2   4
Time = 9.960. rtupdate2() has been called.
Time = 9.960. Node 2 received packet from Node 0.
Time = 9.960. Distance Table for Node 2 has been modified!
  dest|     via     
   D2 |    0    1    3 
  ----|---------------
     0|    3    2    6
     1|    4    1    5
     3|    7    4    2

Simulator terminated at t=9.959651, no packets in medium
```

## 小结

通过此次试验，充分了解了可靠传输协议的基本实现方式，对网络传输层有了基本的认识。
