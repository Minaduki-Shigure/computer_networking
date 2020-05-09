# Programming Assignment: RIP Router Algorithm

Copyright (c) 2020 Minaduki Shigure.  
南京大学 电子科学与工程学院 吴康正 171180571

## 实验环境

Microsoft Windows 10 Version 2004  
Microsoft Hyper-V Manager Version 10.0.19041.1  
Ubuntu 16.04 xenial w/ x86_64 Linux 4.4.0-142-generic

## 文件结构

```plain
├── Makefile
├── obj
├── report
│   ├── out.txt
│   ├── report.md
│   └── report.pdf
├── prog3
└── src
    ├── config.c
    ├── node0.c
    ├── node1.c
    ├── node2.c
    ├── node3.c
    ├── node.c
    ├── node.h
    └── prog3.c
```

实验包含了数个目录，其中src目录存放源码，obj目录存放编译时产生的目标文件，report目录存放实验报告和终端输出结果out.txt，提供Makefile可以用于快速编译和清理。  
node.c与node.h包含了所有节点共有的函数和定义，config.c文件包含了每个节点的初始化参数，如果需要修改节点关系和初始代价，可以在config.c文件中完成。

## 实现方式

> 对提供的prog3.c文件进行了两处修改：  
> 增加了对标准库头文件的包含，同时修改了exit()函数调用增加了一个参数，因为我的编译器提示使用的exit()函数没有声明，而实验也没有要求我来实现这个函数，因此做出了此处修改，**这不会影响程序的执行**。  
> 在程序的printf语句内容末尾增加了一个换行符，在开启了bonus部分后程序对链路代价事件的输出不会换行，导致输出界面较乱，为了美观考虑做出了此处修改，**这会导致部分程序输出排版由于增加的换行符而与原先不一样，但是对程序的运行没有任何影响**

代码使用typedef的方式定义了每个节点的模版，节点的功能使用node.c文件中的函数具体实现，每一个节点的函数会直接调用node.c中的函数完成功能。

节点结构如下：  

```c
typedef struct
{
    int id;
    struct distance_table dt;
    int mincost[4];
    int neighbour[4];
} node_class;
```

每个节点的对象包含了节点id，节点的距离表、当前到每个节点的最小代价和当前节点到其他节点的相邻关系。节点id和相邻关系在节点初始化时写入节点，其中相邻关系为一个数组，如果当前节点与节点i相邻，则数组中对应的i为1，否则为0，特别的，为了提升泛用性考虑，节点与自身身的相邻关系也为0。

### 初始化函数

在每个节点的初始化过程中，先后调用node_constructor()函数rtinit()函数对当前节点进行初始化

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
MAIN: rcv event, t=0.947, at 3
 src: 0, dest: 3, contents:   0   1   3   7
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
MAIN: rcv event, t=0.992, at 0
 src: 1, dest: 0, contents:   1   0   1 999
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
MAIN: rcv event, t=1.209, at 3
 src: 2, dest: 3, contents:   3   1   0   2
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
MAIN: rcv event, t=1.276, at 3
 src: 0, dest: 3, contents:   0   1   2   7
Time = 1.276. rtupdate3() has been called.
Time = 1.276. Node 3 received packet from Node 0.
Time = 1.276. Distance Table for Node 3 has been modified!
  dest|     via     
   D3 |    0    2 
  ----|----------
     0|    7    5
     1|    8    3
     2|    9    2
MAIN: rcv event, t=1.642, at 2
 src: 0, dest: 2, contents:   0   1   3   7
Time = 1.642. rtupdate2() has been called.
Time = 1.642. Node 2 received packet from Node 0.
Time = 1.642. Distance Table for Node 2 has been modified!
  dest|     via     
   D2 |    0    1    3 
  ----|---------------
     0|    3  999  999
     1|    4    1  999
     3|   10  999    2
MAIN: rcv event, t=1.871, at 1
 src: 0, dest: 1, contents:   0   1   3   7
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
MAIN: rcv event, t=2.166, at 2
 src: 1, dest: 2, contents:   1   0   1 999
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
MAIN: rcv event, t=2.407, at 0
 src: 2, dest: 0, contents:   3   1   0   2
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
MAIN: rcv event, t=2.421, at 2
 src: 3, dest: 2, contents:   7 999   2   0
Time = 2.421. rtupdate2() has been called.
Time = 2.421. Node 2 received packet from Node 3.
Time = 2.421. Distance Table for Node 2 has been modified!
  dest|     via     
   D2 |    0    1    3 
  ----|---------------
     0|    3    2    9
     1|    4    1  1001
     3|   10  1000    2
MAIN: rcv event, t=2.811, at 1
 src: 2, dest: 1, contents:   3   1   0   2
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
MAIN: rcv event, t=3.293, at 2
 src: 3, dest: 2, contents:   7   8   2   0
Time = 3.293. rtupdate2() has been called.
Time = 3.293. Node 2 received packet from Node 3.
Time = 3.293. Distance Table for Node 2 has been modified!
  dest|     via     
   D2 |    0    1    3 
  ----|---------------
     0|    3    2    9
     1|    4    1   10
     3|   10  1000    2
MAIN: rcv event, t=3.602, at 3
 src: 2, dest: 3, contents:   2   1   0   2
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
MAIN: rcv event, t=4.063, at 2
 src: 0, dest: 2, contents:   0   1   2   7
Time = 4.063. rtupdate2() has been called.
Time = 4.063. Node 2 received packet from Node 0.
Time = 4.063. Distance Table for Node 2 has been modified!
  dest|     via     
   D2 |    0    1    3 
  ----|---------------
     0|    3    2    9
     1|    4    1   10
     3|   10  1000    2
MAIN: rcv event, t=4.104, at 0
 src: 3, dest: 0, contents:   7 999   2   0
Time = 4.104. rtupdate0() has been called.
Time = 4.104. Node 0 received packet from Node 3.
Time = 4.104. Distance Table for Node 0 has been modified!
  dest|     via     
   D0 |    1    2    3 
  ----|---------------
     1|    1    4  1006
     2|    2    3    9
     3|  1000    5    7
MAIN: rcv event, t=4.169, at 2
 src: 3, dest: 2, contents:   5   3   2   0
Time = 4.169. rtupdate2() has been called.
Time = 4.169. Node 2 received packet from Node 3.
Time = 4.169. Distance Table for Node 2 has been modified!
  dest|     via     
   D2 |    0    1    3 
  ----|---------------
     0|    3    2    7
     1|    4    1    5
     3|   10  1000    2
MAIN: rcv event, t=4.330, at 0
 src: 3, dest: 0, contents:   7   8   2   0
Time = 4.330. rtupdate0() has been called.
Time = 4.330. Node 0 received packet from Node 3.
Time = 4.330. Distance Table for Node 0 has been modified!
  dest|     via     
   D0 |    1    2    3 
  ----|---------------
     1|    1    4   15
     2|    2    3    9
     3|  1000    5    7
MAIN: rcv event, t=4.643, at 1
 src: 0, dest: 1, contents:   0   1   2   7
Time = 4.643. rtupdate1() has been called.
Time = 4.643. Node 1 received packet from Node 0.
Time = 4.643. Distance Table for Node 1 has been modified!
  dest|     via     
   D1 |    0    2 
  ----|----------
     0|    1    4
     2|    3    1
     3|    8    3
MAIN: rcv event, t=5.213, at 0
 src: 3, dest: 0, contents:   5   3   2   0
Time = 5.213. rtupdate0() has been called.
Time = 5.213. Node 0 received packet from Node 3.
Time = 5.213. Distance Table for Node 0 has been modified!
  dest|     via     
   D0 |    1    2    3 
  ----|---------------
     1|    1    4   10
     2|    2    3    9
     3|  1000    5    7
MAIN: rcv event, t=5.384, at 3
 src: 0, dest: 3, contents:   0   1   2   5
Time = 5.384. rtupdate3() has been called.
Time = 5.384. Node 3 received packet from Node 0.
Time = 5.384. Distance Table for Node 3 has been modified!
  dest|     via     
   D3 |    0    2 
  ----|----------
     0|    7    4
     1|    8    3
     2|    9    2
MAIN: rcv event, t=5.820, at 1
 src: 2, dest: 1, contents:   2   1   0   2
Time = 5.820. rtupdate1() has been called.
Time = 5.820. Node 1 received packet from Node 2.
Time = 5.820. Distance Table for Node 1 has been modified!
  dest|     via     
   D1 |    0    2 
  ----|----------
     0|    1    3
     2|    3    1
     3|    8    3
MAIN: rcv event, t=6.042, at 2
 src: 1, dest: 2, contents:   1   0   1   8
Time = 6.042. rtupdate2() has been called.
Time = 6.042. Node 2 received packet from Node 1.
Time = 6.042. Distance Table for Node 2 has been modified!
  dest|     via     
   D2 |    0    1    3 
  ----|---------------
     0|    3    2    7
     1|    4    1    5
     3|   10    9    2
MAIN: rcv event, t=6.071, at 0
 src: 1, dest: 0, contents:   1   0   1   8
Time = 6.071. rtupdate0() has been called.
Time = 6.071. Node 0 received packet from Node 1.
Time = 6.071. Distance Table for Node 0 has been modified!
  dest|     via     
   D0 |    1    2    3 
  ----|---------------
     1|    1    4   10
     2|    2    3    9
     3|    9    5    7
MAIN: rcv event, t=6.532, at 1
 src: 0, dest: 1, contents:   0   1   2   5
Time = 6.532. rtupdate1() has been called.
Time = 6.532. Node 1 received packet from Node 0.
Time = 6.532. Distance Table for Node 1 has been modified!
  dest|     via     
   D1 |    0    2 
  ----|----------
     0|    1    3
     2|    3    1
     3|    6    3
MAIN: rcv event, t=7.021, at 0
 src: 2, dest: 0, contents:   2   1   0   2
Time = 7.021. rtupdate0() has been called.
Time = 7.021. Node 0 received packet from Node 2.
Time = 7.021. Distance Table for Node 0 has been modified!
  dest|     via     
   D0 |    1    2    3 
  ----|---------------
     1|    1    4   10
     2|    2    3    9
     3|    9    5    7
MAIN: rcv event, t=7.160, at 2
 src: 0, dest: 2, contents:   0   1   2   5
Time = 7.160. rtupdate2() has been called.
Time = 7.160. Node 2 received packet from Node 0.
Time = 7.160. Distance Table for Node 2 has been modified!
  dest|     via     
   D2 |    0    1    3 
  ----|---------------
     0|    3    2    7
     1|    4    1    5
     3|    8    9    2
MAIN: rcv event, t=7.405, at 0
 src: 1, dest: 0, contents:   1   0   1   3
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
MAIN: rcv event, t=7.579, at 3
 src: 0, dest: 3, contents:   0   1   2   4
Time = 7.579. rtupdate3() has been called.
Time = 7.579. Node 3 received packet from Node 0.
Time = 7.579. Distance Table for Node 3 has been modified!
  dest|     via     
   D3 |    0    2 
  ----|----------
     0|    7    4
     1|    8    3
     2|    9    2
MAIN: rcv event, t=7.941, at 1
 src: 0, dest: 1, contents:   0   1   2   4
Time = 7.941. rtupdate1() has been called.
Time = 7.941. Node 1 received packet from Node 0.
Time = 7.941. Distance Table for Node 1 has been modified!
  dest|     via     
   D1 |    0    2 
  ----|----------
     0|    1    3
     2|    3    1
     3|    5    3
MAIN: rcv event, t=8.086, at 0
 src: 3, dest: 0, contents:   4   3   2   0
Time = 8.086. rtupdate0() has been called.
Time = 8.086. Node 0 received packet from Node 3.
Time = 8.086. Distance Table for Node 0 has been modified!
  dest|     via     
   D0 |    1    2    3 
  ----|---------------
     1|    1    4   10
     2|    2    3    9
     3|    4    5    7
MAIN: rcv event, t=8.639, at 2
 src: 1, dest: 2, contents:   1   0   1   3
Time = 8.639. rtupdate2() has been called.
Time = 8.639. Node 2 received packet from Node 1.
Time = 8.639. Distance Table for Node 2 has been modified!
  dest|     via     
   D2 |    0    1    3 
  ----|---------------
     0|    3    2    7
     1|    4    1    5
     3|    8    4    2
MAIN: rcv event, t=8.943, at 2
 src: 3, dest: 2, contents:   4   3   2   0
Time = 8.943. rtupdate2() has been called.
Time = 8.943. Node 2 received packet from Node 3.
Time = 8.943. Distance Table for Node 2 has been modified!
  dest|     via     
   D2 |    0    1    3 
  ----|---------------
     0|    3    2    6
     1|    4    1    5
     3|    8    4    2
MAIN: rcv event, t=9.960, at 2
 src: 0, dest: 2, contents:   0   1   2   4
Time = 9.960. rtupdate2() has been called.
Time = 9.960. Node 2 received packet from Node 0.
Time = 9.960. Distance Table for Node 2 has been modified!
  dest|     via     
   D2 |    0    1    3 
  ----|---------------
     0|    3    2    6
     1|    4    1    5
     3|    7    4    2
MAIN: rcv event, t=10000.000, at -1
Time = 10000.000. linkhandler0() has been called.
Time = 10000.000. Node 0 detected the new cost of link to Node 1 is now 20.
Time = 10000.000. Distance Table for Node 0 has been modified!
  dest|     via     
   D0 |    1    2    3 
  ----|---------------
     1|   20    4   10
     2|   21    3    9
     3|   23    5    7
Time = 10000.000. Node 0 sent packet to Node 1 : 0 4 3 5
Time = 10000.000. Node 0 sent packet to Node 2 : 0 4 3 5
Time = 10000.000. Node 0 sent packet to Node 3 : 0 4 3 5
Time = 10000.000. linkhandler1() has been called.
Time = 10000.000. Node 1 detected the new cost of link to Node 0 is now 20.
Time = 10000.000. Distance Table for Node 1 has been modified!
  dest|     via     
   D1 |    0    2 
  ----|----------
     0|   20    3
     2|   22    1
     3|   24    3
Time = 10000.000. Node 1 sent packet to Node 0 : 3 0 1 3
Time = 10000.000. Node 1 sent packet to Node 2 : 3 0 1 3
MAIN: rcv event, t=10000.178, at 1
 src: 0, dest: 1, contents:   0   4   3   5
Time = 10000.178. rtupdate1() has been called.
Time = 10000.178. Node 1 received packet from Node 0.
Time = 10000.178. Distance Table for Node 1 has been modified!
  dest|     via     
   D1 |    0    2 
  ----|----------
     0|   20    3
     2|   23    1
     3|   25    3
MAIN: rcv event, t=10000.702, at 0
 src: 1, dest: 0, contents:   3   0   1   3
Time = 10000.702. rtupdate0() has been called.
Time = 10000.702. Node 0 received packet from Node 1.
Time = 10000.702. Distance Table for Node 0 has been modified!
  dest|     via     
   D0 |    1    2    3 
  ----|---------------
     1|   20    4   10
     2|   21    3    9
     3|   23    5    7
MAIN: rcv event, t=10001.166, at 3
 src: 0, dest: 3, contents:   0   4   3   5
Time = 10001.166. rtupdate3() has been called.
Time = 10001.166. Node 3 received packet from Node 0.
Time = 10001.166. Distance Table for Node 3 has been modified!
  dest|     via     
   D3 |    0    2 
  ----|----------
     0|    7    4
     1|   11    3
     2|   10    2
MAIN: rcv event, t=10001.964, at 2
 src: 0, dest: 2, contents:   0   4   3   5
Time = 10001.964. rtupdate2() has been called.
Time = 10001.964. Node 2 received packet from Node 0.
Time = 10001.964. Distance Table for Node 2 has been modified!
  dest|     via     
   D2 |    0    1    3 
  ----|---------------
     0|    3    2    6
     1|    7    1    5
     3|    8    4    2
MAIN: rcv event, t=10003.342, at 2
 src: 1, dest: 2, contents:   3   0   1   3
Time = 10003.342. rtupdate2() has been called.
Time = 10003.342. Node 2 received packet from Node 1.
Time = 10003.342. Distance Table for Node 2 has been modified!
  dest|     via     
   D2 |    0    1    3 
  ----|---------------
     0|    3    4    6
     1|    7    1    5
     3|    8    4    2
Time = 10003.342. Node 2 sent packet to Node 0 : 3 1 0 2
Time = 10003.342. Node 2 sent packet to Node 1 : 3 1 0 2
Time = 10003.342. Node 2 sent packet to Node 3 : 3 1 0 2
MAIN: rcv event, t=10003.448, at 0
 src: 2, dest: 0, contents:   3   1   0   2
Time = 10003.448. rtupdate0() has been called.
Time = 10003.448. Node 0 received packet from Node 2.
Time = 10003.448. Distance Table for Node 0 has been modified!
  dest|     via     
   D0 |    1    2    3 
  ----|---------------
     1|   20    4   10
     2|   21    3    9
     3|   23    5    7
MAIN: rcv event, t=10004.307, at 1
 src: 2, dest: 1, contents:   3   1   0   2
Time = 10004.307. rtupdate1() has been called.
Time = 10004.307. Node 1 received packet from Node 2.
Time = 10004.307. Distance Table for Node 1 has been modified!
  dest|     via     
   D1 |    0    2 
  ----|----------
     0|   20    4
     2|   23    1
     3|   25    3
Time = 10004.307. Node 1 sent packet to Node 0 : 4 0 1 3
Time = 10004.307. Node 1 sent packet to Node 2 : 4 0 1 3
MAIN: rcv event, t=10004.417, at 3
 src: 2, dest: 3, contents:   3   1   0   2
Time = 10004.417. rtupdate3() has been called.
Time = 10004.417. Node 3 received packet from Node 2.
Time = 10004.417. Distance Table for Node 3 has been modified!
  dest|     via     
   D3 |    0    2 
  ----|----------
     0|    7    5
     1|   11    3
     2|   10    2
Time = 10004.417. Node 3 sent packet to Node 0 : 5 3 2 0
Time = 10004.417. Node 3 sent packet to Node 2 : 5 3 2 0
MAIN: rcv event, t=10004.669, at 0
 src: 1, dest: 0, contents:   4   0   1   3
Time = 10004.669. rtupdate0() has been called.
Time = 10004.669. Node 0 received packet from Node 1.
Time = 10004.669. Distance Table for Node 0 has been modified!
  dest|     via     
   D0 |    1    2    3 
  ----|---------------
     1|   20    4   10
     2|   21    3    9
     3|   23    5    7
MAIN: rcv event, t=10005.498, at 2
 src: 1, dest: 2, contents:   4   0   1   3
Time = 10005.498. rtupdate2() has been called.
Time = 10005.498. Node 2 received packet from Node 1.
Time = 10005.498. Distance Table for Node 2 has been modified!
  dest|     via     
   D2 |    0    1    3 
  ----|---------------
     0|    3    5    6
     1|    7    1    5
     3|    8    4    2
MAIN: rcv event, t=10005.692, at 2
 src: 3, dest: 2, contents:   5   3   2   0
Time = 10005.692. rtupdate2() has been called.
Time = 10005.692. Node 2 received packet from Node 3.
Time = 10005.692. Distance Table for Node 2 has been modified!
  dest|     via     
   D2 |    0    1    3 
  ----|---------------
     0|    3    5    7
     1|    7    1    5
     3|    8    4    2
MAIN: rcv event, t=10006.615, at 0
 src: 3, dest: 0, contents:   5   3   2   0
Time = 10006.615. rtupdate0() has been called.
Time = 10006.615. Node 0 received packet from Node 3.
Time = 10006.615. Distance Table for Node 0 has been modified!
  dest|     via     
   D0 |    1    2    3 
  ----|---------------
     1|   20    4   10
     2|   21    3    9
     3|   23    5    7
MAIN: rcv event, t=20000.000, at 0
Time = 20000.000. linkhandler0() has been called.
Time = 20000.000. Node 0 detected the new cost of link to Node 1 is now 1.
Time = 20000.000. Distance Table for Node 0 has been modified!
  dest|     via     
   D0 |    1    2    3 
  ----|---------------
     1|    1    4   10
     2|    2    3    9
     3|    4    5    7
Time = 20000.000. Node 0 sent packet to Node 1 : 0 1 2 4
Time = 20000.000. Node 0 sent packet to Node 2 : 0 1 2 4
Time = 20000.000. Node 0 sent packet to Node 3 : 0 1 2 4
Time = 20000.000. linkhandler1() has been called.
Time = 20000.000. Node 1 detected the new cost of link to Node 0 is now 1.
Time = 20000.000. Distance Table for Node 1 has been modified!
  dest|     via     
   D1 |    0    2 
  ----|----------
     0|    1    4
     2|    4    1
     3|    6    3
Time = 20000.000. Node 1 sent packet to Node 0 : 1 0 1 3
Time = 20000.000. Node 1 sent packet to Node 2 : 1 0 1 3
MAIN: rcv event, t=20000.014, at 2
 src: 0, dest: 2, contents:   0   1   2   4
Time = 20000.014. rtupdate2() has been called.
Time = 20000.014. Node 2 received packet from Node 0.
Time = 20000.014. Distance Table for Node 2 has been modified!
  dest|     via     
   D2 |    0    1    3 
  ----|---------------
     0|    3    5    7
     1|    4    1    5
     3|    7    4    2
MAIN: rcv event, t=20000.068, at 0
 src: 1, dest: 0, contents:   1   0   1   3
Time = 20000.068. rtupdate0() has been called.
Time = 20000.068. Node 0 received packet from Node 1.
Time = 20000.068. Distance Table for Node 0 has been modified!
  dest|     via     
   D0 |    1    2    3 
  ----|---------------
     1|    1    4   10
     2|    2    3    9
     3|    4    5    7
MAIN: rcv event, t=20000.885, at 2
 src: 1, dest: 2, contents:   1   0   1   3
Time = 20000.885. rtupdate2() has been called.
Time = 20000.885. Node 2 received packet from Node 1.
Time = 20000.885. Distance Table for Node 2 has been modified!
  dest|     via     
   D2 |    0    1    3 
  ----|---------------
     0|    3    2    7
     1|    4    1    5
     3|    7    4    2
Time = 20000.885. Node 2 sent packet to Node 0 : 2 1 0 2
Time = 20000.885. Node 2 sent packet to Node 1 : 2 1 0 2
Time = 20000.885. Node 2 sent packet to Node 3 : 2 1 0 2
MAIN: rcv event, t=20001.076, at 3
 src: 0, dest: 3, contents:   0   1   2   4
Time = 20001.076. rtupdate3() has been called.
Time = 20001.076. Node 3 received packet from Node 0.
Time = 20001.076. Distance Table for Node 3 has been modified!
  dest|     via     
   D3 |    0    2 
  ----|----------
     0|    7    5
     1|    8    3
     2|    9    2
MAIN: rcv event, t=20001.125, at 3
 src: 2, dest: 3, contents:   2   1   0   2
Time = 20001.125. rtupdate3() has been called.
Time = 20001.125. Node 3 received packet from Node 2.
Time = 20001.125. Distance Table for Node 3 has been modified!
  dest|     via     
   D3 |    0    2 
  ----|----------
     0|    7    4
     1|    8    3
     2|    9    2
Time = 20001.125. Node 3 sent packet to Node 0 : 4 3 2 0
Time = 20001.125. Node 3 sent packet to Node 2 : 4 3 2 0
MAIN: rcv event, t=20001.834, at 0
 src: 2, dest: 0, contents:   2   1   0   2
Time = 20001.834. rtupdate0() has been called.
Time = 20001.834. Node 0 received packet from Node 2.
Time = 20001.834. Distance Table for Node 0 has been modified!
  dest|     via     
   D0 |    1    2    3 
  ----|---------------
     1|    1    4   10
     2|    2    3    9
     3|    4    5    7
MAIN: rcv event, t=20001.961, at 1
 src: 0, dest: 1, contents:   0   1   2   4
Time = 20001.961. rtupdate1() has been called.
Time = 20001.961. Node 1 received packet from Node 0.
Time = 20001.961. Distance Table for Node 1 has been modified!
  dest|     via     
   D1 |    0    2 
  ----|----------
     0|    1    4
     2|    3    1
     3|    5    3
MAIN: rcv event, t=20002.221, at 0
 src: 3, dest: 0, contents:   4   3   2   0
Time = 20002.221. rtupdate0() has been called.
Time = 20002.221. Node 0 received packet from Node 3.
Time = 20002.221. Distance Table for Node 0 has been modified!
  dest|     via     
   D0 |    1    2    3 
  ----|---------------
     1|    1    4   10
     2|    2    3    9
     3|    4    5    7
MAIN: rcv event, t=20002.854, at 2
 src: 3, dest: 2, contents:   4   3   2   0
Time = 20002.854. rtupdate2() has been called.
Time = 20002.854. Node 2 received packet from Node 3.
Time = 20002.854. Distance Table for Node 2 has been modified!
  dest|     via     
   D2 |    0    1    3 
  ----|---------------
     0|    3    2    6
     1|    4    1    5
     3|    7    4    2
MAIN: rcv event, t=20002.979, at 1
 src: 2, dest: 1, contents:   2   1   0   2
Time = 20002.979. rtupdate1() has been called.
Time = 20002.979. Node 1 received packet from Node 2.
Time = 20002.979. Distance Table for Node 1 has been modified!
  dest|     via     
   D1 |    0    2 
  ----|----------
     0|    1    3
     2|    3    1
     3|    5    3

Simulator terminated at t=20002.978516, no packets in medium
```

## 小结

本实验实现了一个简单的路由协议，不过实验整体更倾向于验证，可拓展性和部署可行性不是很强，主要是受限于测试代码中写死了节点的个数，导致对节点数量的拓展性较弱。

如果涉及到节点数量的可变性，我想到了两种方案，一种是将节点数使用宏定义的方法定义，然后在部署前修改宏定义的值和对应的config文件的节点配置，也就是实验中使用的方法，这种方法简单易行，但是其节点数量的“可变性”是静态的，也就是说一旦部署，就无法改变节点的数量，并不算是真正的可变。而且由于投鼠忌器，我没有修改prog3.c文件中的节点配置为宏定义模式，也就没有办法验证这种实现在其他节点数量时的可行性。

另一种方案则是使用动态分配的方式对各个节点的属性进行调整，但是这需要额外的代码来处理新的节点的问题，比如新节点需要发送一个“包”来通知其他节点，而如果要实现这个包势必要对原有的测试代码进行修改，从而实现对包的区分，同时节点的properties比如邻居信息等可能需要大量更改数据结构以实现动态性，需要考虑的各种情况会带来相当大的工作量，这对于一个普通的验证性作业有些overkill了（~~其实就是懒~~），因此对于这个方案并没有去实现任何除了空想之外的工作。

通过此次实验，对路由算法有了更深入的理解，也对程序结构的规划有了新的认识。
