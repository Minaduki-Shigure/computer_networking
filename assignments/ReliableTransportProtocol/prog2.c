#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose
   
   This code should be used for PA2, unidirectional or bidirectional
   data transfer protocols (from A to B. Bidirectional transfer of data
   is for extra credit and is not required).  Network properties:
   - one way network delay averages five time units (longer if there
       are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
       or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
       (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 0 /* change to 1 if you're doing extra credit */
                        /* and write a routine called B_output */

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct msg
{
    char data[20];
};

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt
{
    int seqnum;
    int acknum;
    int checksum;
    char payload[20];
};

void starttimer(int AorB, float increment);
void stoptimer(int AorB);
void tolayer3(int AorB, struct pkt packet);
void tolayer5(int AorB, char datasent[20]);

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

#define BUFFER_SIZE 50
#define DEFAULT_WINDOW_SIZE 16

typedef struct {
    int base;
    int next_seq;
    // int message_num;
    int window_size;
    int buffer_ptr; // Top of the buffer.
    struct pkt buffer[BUFFER_SIZE];
    double estimatedRTT;
} sender_class;

typedef struct {
    int expecting_seq;
    struct pkt ack_buffer;
} receiver_class;

// Tried to implement the Window via linked list but failed to realize...
// Forget it! The following type is not used.
typedef struct {
    struct pkt* packet;
    Window* next;
} Window;

sender_class sender_A;
sender_class sender_B;
receiver_class receiver_A;
receiver_class receiver_B;

// The following function was written for the list implement of the window and is deprecated now.
// The packet will now be pre-generated when the sender initialized.
struct pkt* gen_packet(int seq, const char data[20])
{
    struct pkt* new_packet;
    new_packet = (struct ptk*)malloc(sizeof(struct pkt));
    memset(new_packet, 0, sizeof(struct pkt));
    new_packet->seqnum = seq;
    new_packet->acknum = -1;
    memcpy(new_packet->payload, data, 20);
    new_packet->checksum = cal_checksum(new_packet);
    return new_packet;
}

void sender_init(sender_class* sender)
{
    sender->base = 1;
    sender->next_seq = 1;
    // sender->message_num = 0;
    sender->window_size = DEFAULT_WINDOW_SIZE;
    sender->buffer_ptr = 1;
    sender->estimatedRTT = 20;
}

void receiver_init(receiver_class* receiver)
{
    receiver->expecting_seq = 1;
    (receiver->ack_buffer).seqnum = -1;
    (receiver->ack_buffer).acknum = 0;
    memset((receiver->ack_buffer).payload, 0, 20);
    memcpy((receiver->ack_buffer).payload, "ACK", 4);
    (receiver->ack_buffer).checksum = cal_checksum(&(receiver->ack_buffer));
}

void hightlight_printf(const char* content)
{
    printf("\033[31m%s \033[0m", content);
    return;
}

int cal_checksum(struct pkt* packet)
{
    int checksum = 0;
    int i = 0;
    checksum += (packet->seqnum << 8) + packet->acknum;
    for (i = 0; i < 10; ++i)
    {
        checksum += (packet->payload[2 * i] << 8) + packet->payload[2 * i + 1];
    }
    while (checksum > 0xffff)
    {
        checksum = checksum & 0xff + (checksum >> 16);
    }
    checksum = 0xffff - checksum;
    return checksum;
}

void window_send(void)
{
    while ((sender_A.next_seq < sender_A.buffer_ptr) && (sender_A.next_seq < sender_A.base + sender_A.window_size))
    {
        struct pkt* send = sender_A.buffer + (sender_A.next_seq % BUFFER_SIZE);
        hightlight_printf("window_send:");
        printf("Sending SEQ = %d.\n", send->seqnum);
        tolayer3(0, *send);
        if (sender_A.base == sender_A.next_seq)
        {
            starttimer(0, sender_A.estimatedRTT);
        }
        ++sender_A.next_seq;
    }
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
    if (sender_A.buffer_ptr - sender_A.base >= BUFFER_SIZE)
    {
        hightlight_printf("A_output:");
        printf("Buffer overflow. Message dropped.\n");
        return;
    }
    struct pkt* new_packet = buffer + (buffer_ptr % BUFFER_SIZE);
    new_packet->seqnum = sender_A.buffer_ptr;
    memcpy(new_packet->payload, message.data, 20);
    new_packet->checksum = cal_checksum(new_packet);
    hightlight_printf("A_output:");
    printf("Added message. Allocated SEQ = %d.\n", new_packet->seqnum);
    ++sender_A.buffer_ptr;
    window_send();
}

/* need be completed only for extra credit */
void B_output(struct msg message)
{
    printf("  B_output: uni-directional. ignore.\n");
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
    if (cal_checksum(&packet) != packet.checksum)
    {
        hightlight_printf("A_input:");
        printf("Packet corrupted. Dropping packet.\n");
        return;
    }
    if (packet.acknum < sender_A.base)
    {
        hightlight_printf("A_input:");
        printf("Received duplicate ACK. Packet ignored.\n");
        return;
    }
    hightlight_printf("A_input:");
    printf("Got ACK = %d.\n", packet.acknum);
    sender_A.base = packet.acknum + 1;
    
    if (sender_A.base == sender_A.next_seq)
    {
        stoptimer(0);
        window_send();
    }
    else
    {
        starttimer(0, sender_A.estimatedRTT);
    }
}

/* called when A's timer goes off */
void A_timerinterrupt(void)
{
    int i = 0;
    for (i = sender_A.base; i < sender_A.next_seq; ++i)
    {
        struct pkt* resend = sender_A.buffer + (i % BUFFER_SIZE);
        hightlight_printf("A_timerinterrupt:");
        printf("Timeout for SEQ = %d. Resending.\n", resend->seqnum);
        tolayer3(0, *resend);
    }
    estimatedRTT = estimatedRTT + 10;
    starttimer(0, sender_A.estimatedRTT);
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init(void)
{
    sender_init(&sender_A);
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

void send_ack(int AorB, int ack)
{
    // Do nothing. This function is implemented somewhere else.
}

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
    if (cal_checksum(&packet) != packet.checksum)
    {
        hightlight_printf("B_input:");
        printf("Packet corrupted. Sending last ACK = %d.\n", receiver_B.ack_buffer.acknum);
        tolayer3(1, receiver_B.ack_buffer);
        return;
    }
    if (packet.seqnum != receiver_B.expecting_seq)
    {
        hightlight_printf("B_input:");
        printf("Unexpected seqnum received. Sending last ACK = %d.\n", receiver_B.ack_buffer.acknum);
        tolayer3(1, receiver_B.ack_buffer);
        return;
    }
    hightlight_printf("B_input:");
    printf("Received packet SEQ = %d. Sending ACK = %d.", packet.seqnum, packet.seqnum);
    tolayer5(1, packet.payload);
    receiver_B.ack_buffer.acknum = packet.seqnum;
    receiver_B.ack_buffer.checksum = cal_checksum(&(receiver_B.ack_buffer));
    ++receiver_B.expecting_seq;
    tolayer3(1, receiver_B.ack_buffer);
}

/* called when B's timer goes off */
void B_timerinterrupt(void)
{
    printf("  B_timerinterrupt: B doesn't have a timer. ignore.\n");
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init(void)
{
    receiver_init(&receiver_B);
}

/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
    - emulates the tranmission and delivery (possibly with bit-level corruption
        and packet loss) of packets across the layer 3/4 interface
    - handles the starting/stopping of a timer, and generates timer
        interrupts (resulting in calling students timer handler).
    - generates message to be sent (passed from later 5 to 4)
THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
******************************************************************/

struct event
{
    float evtime;       /* event time */
    int evtype;         /* event type code */
    int eventity;       /* entity where event occurs */
    struct pkt *pktptr; /* ptr to packet (if any) assoc w/ this event */
    struct event *prev;
    struct event *next;
};
struct event *evlist = NULL; /* the event list */

/* possible events: */
#define TIMER_INTERRUPT 0
#define FROM_LAYER5 1
#define FROM_LAYER3 2

#define OFF 0
#define ON 1
#define A 0
#define B 1

int TRACE = 1;   /* for my debugging */
int nsim = 0;    /* number of messages from 5 to 4 so far */
int nsimmax = 0; /* number of msgs to generate, then stop */
float time = 0.000;
float lossprob;    /* probability that a packet is dropped  */
float corruptprob; /* probability that one bit is packet is flipped */
float lambda;      /* arrival rate of messages from layer 5 */
int ntolayer3;     /* number sent into layer 3 */
int nlost;         /* number lost in media */
int ncorrupt;      /* number corrupted by media*/

void init(int argc, char **argv);
void generate_next_arrival(void);
void insertevent(struct event *p);

int main(int argc, char **argv)
{
    struct event *eventptr;
    struct msg msg2give;
    struct pkt pkt2give;

    int i, j;
    char c;

    init(argc, argv);
    A_init();
    B_init();

    while (1)
    {
        eventptr = evlist; /* get next event to simulate */
        if (eventptr == NULL)
            goto terminate;
        evlist = evlist->next; /* remove this event from event list */
        if (evlist != NULL)
            evlist->prev = NULL;
        if (TRACE >= 2)
        {
            printf("\nEVENT time: %f,", eventptr->evtime);
            printf("  type: %d", eventptr->evtype);
            if (eventptr->evtype == 0)
                printf(", timerinterrupt  ");
            else if (eventptr->evtype == 1)
                printf(", fromlayer5 ");
            else
                printf(", fromlayer3 ");
            printf(" entity: %d\n", eventptr->eventity);
        }
        time = eventptr->evtime; /* update time to next event time */
        if (eventptr->evtype == FROM_LAYER5)
        {
            if (nsim < nsimmax)
            {
                if (nsim + 1 < nsimmax)
                    generate_next_arrival(); /* set up future arrival */
                /* fill in msg to give with string of same letter */
                j = nsim % 26;
                for (i = 0; i < 20; i++)
                    msg2give.data[i] = 97 + j;
                msg2give.data[19] = 0;
                if (TRACE > 2)
                {
                    printf("          MAINLOOP: data given to student: ");
                    for (i = 0; i < 20; i++)
                        printf("%c", msg2give.data[i]);
                    printf("\n");
                }
                nsim++;
                if (eventptr->eventity == A)
                    A_output(msg2give);
                else
                    B_output(msg2give);
            }
        }
        else if (eventptr->evtype == FROM_LAYER3)
        {
            pkt2give.seqnum = eventptr->pktptr->seqnum;
            pkt2give.acknum = eventptr->pktptr->acknum;
            pkt2give.checksum = eventptr->pktptr->checksum;
            for (i = 0; i < 20; i++)
                pkt2give.payload[i] = eventptr->pktptr->payload[i];
            if (eventptr->eventity == A) /* deliver packet by calling */
                A_input(pkt2give);       /* appropriate entity */
            else
                B_input(pkt2give);
            free(eventptr->pktptr); /* free the memory for packet */
        }
        else if (eventptr->evtype == TIMER_INTERRUPT)
        {
            if (eventptr->eventity == A)
                A_timerinterrupt();
            else
                B_timerinterrupt();
        }
        else
        {
            printf("INTERNAL PANIC: unknown event type \n");
        }
        free(eventptr);
    }

terminate:
    printf(
        " Simulator terminated at time %f\n after sending %d msgs from layer5\n",
        time, nsim);
}

void init(int argc, char **argv) /* initialize the simulator */
{
    int i;
    float sum, avg;
    float jimsrand();

    if (argc != 6)
    {
        printf("usage: %s  num_sim  prob_loss  prob_corrupt  interval  debug_level\n", argv[0]);
        exit(1);
    }

    nsimmax = atoi(argv[1]);
    lossprob = atof(argv[2]);
    corruptprob = atof(argv[3]);
    lambda = atof(argv[4]);
    TRACE = atoi(argv[5]);
    printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
    printf("the number of messages to simulate: %d\n", nsimmax);
    printf("packet loss probability: %f\n", lossprob);
    printf("packet corruption probability: %f\n", corruptprob);
    printf("average time between messages from sender's layer5: %f\n", lambda);
    printf("TRACE: %d\n", TRACE);

    srand(9999); /* init random number generator */
    sum = 0.0;   /* test random number generator for students */
    for (i = 0; i < 1000; i++)
        sum = sum + jimsrand(); /* jimsrand() should be uniform in [0,1] */
    avg = sum / 1000.0;
    if (avg < 0.25 || avg > 0.75)
    {
        printf("It is likely that random number generation on your machine\n");
        printf("is different from what this emulator expects.  Please take\n");
        printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
        exit(1);
    }

    ntolayer3 = 0;
    nlost = 0;
    ncorrupt = 0;

    time = 0.0;              /* initialize time to 0.0 */
    generate_next_arrival(); /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand(void)
{
    double mmm = RAND_MAX;
    float x;          /* individual students may need to change mmm */
    x = rand() / mmm; /* x should be uniform in [0,1] */
    return (x);
}

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

void generate_next_arrival(void)
{
    double x, log(), ceil();
    struct event *evptr;
    float ttime;
    int tempint;

    if (TRACE > 2)
        printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

    x = lambda * jimsrand() * 2; /* x is uniform on [0,2*lambda] */
                                 /* having mean of lambda        */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtime = time + x;
    evptr->evtype = FROM_LAYER5;
    if (BIDIRECTIONAL && (jimsrand() > 0.5))
        evptr->eventity = B;
    else
        evptr->eventity = A;
    insertevent(evptr);
}

void insertevent(struct event *p)
{
    struct event *q, *qold;

    if (TRACE > 2)
    {
        printf("            INSERTEVENT: time is %lf\n", time);
        printf("            INSERTEVENT: future time will be %lf\n", p->evtime);
    }
    q = evlist; /* q points to header of list in which p struct inserted */
    if (q == NULL)
    { /* list is empty */
        evlist = p;
        p->next = NULL;
        p->prev = NULL;
    }
    else
    {
        for (qold = q; q != NULL && p->evtime > q->evtime; q = q->next)
            qold = q;
        if (q == NULL)
        { /* end of list */
            qold->next = p;
            p->prev = qold;
            p->next = NULL;
        }
        else if (q == evlist)
        { /* front of list */
            p->next = evlist;
            p->prev = NULL;
            p->next->prev = p;
            evlist = p;
        }
        else
        { /* middle of list */
            p->next = q;
            p->prev = q->prev;
            q->prev->next = p;
            q->prev = p;
        }
    }
}

void printevlist(void)
{
    struct event *q;
    int i;
    printf("--------------\nEvent List Follows:\n");
    for (q = evlist; q != NULL; q = q->next)
    {
        printf("Event time: %f, type: %d entity: %d\n", q->evtime, q->evtype,
               q->eventity);
    }
    printf("--------------\n");
}

/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
void stoptimer(int AorB /* A or B is trying to stop timer */)
{
    struct event *q, *qold;

    if (TRACE > 2)
        printf("          STOP TIMER: stopping timer at %f\n", time);
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB))
        {
            /* remove this event */
            if (q->next == NULL && q->prev == NULL)
                evlist = NULL;        /* remove first and only event on list */
            else if (q->next == NULL) /* end of list - there is one in front */
                q->prev->next = NULL;
            else if (q == evlist)
            { /* front of list - there must be event after */
                q->next->prev = NULL;
                evlist = q->next;
            }
            else
            { /* middle of list */
                q->next->prev = q->prev;
                q->prev->next = q->next;
            }
            free(q);
            return;
        }
    printf("Warning: unable to cancel your timer. It wasn't running.\n");
}

void starttimer(int AorB /* A or B is trying to stop timer */, float increment)
{
    struct event *q;
    struct event *evptr;

    if (TRACE > 2)
        printf("          START TIMER: starting timer at %f\n", time);
    /* be nice: check to see if timer is already started, if so, then  warn */
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB))
        {
            printf("Warning: attempt to start a timer that is already started\n");
            return;
        }

    /* create future event for when timer goes off */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtime = time + increment;
    evptr->evtype = TIMER_INTERRUPT;
    evptr->eventity = AorB;
    insertevent(evptr);
}

/************************** TOLAYER3 ***************/
void tolayer3(int AorB /* A or B is trying to stop timer */, struct pkt packet)
{
    struct pkt *mypktptr;
    struct event *evptr, *q;
    float lastime, x;
    int i;

    ntolayer3++;

    /* simulate losses: */
    if (jimsrand() < lossprob)
    {
        nlost++;
        if (TRACE > 0)
            printf("          TOLAYER3: packet being lost\n");
        return;
    }

    /* make a copy of the packet student just gave me since he/she may decide */
    /* to do something with the packet after we return back to him/her */
    mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
    mypktptr->seqnum = packet.seqnum;
    mypktptr->acknum = packet.acknum;
    mypktptr->checksum = packet.checksum;
    for (i = 0; i < 20; i++)
        mypktptr->payload[i] = packet.payload[i];
    if (TRACE > 2)
    {
        printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
               mypktptr->acknum, mypktptr->checksum);
        for (i = 0; i < 20; i++)
            printf("%c", mypktptr->payload[i]);
        printf("\n");
    }

    /* create future event for arrival of packet at the other side */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtype = FROM_LAYER3;      /* packet will pop out from layer3 */
    evptr->eventity = (AorB + 1) % 2; /* event occurs at other entity */
    evptr->pktptr = mypktptr;         /* save ptr to my copy of packet */
                                      /* finally, compute the arrival time of packet at the other end.
                                         medium can not reorder, so make sure packet arrives between 1 and 10
                                         time units after the latest arrival time of packets
                                         currently in the medium on their way to the destination */
    lastime = time;
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == FROM_LAYER3 && q->eventity == evptr->eventity))
            lastime = q->evtime;
    evptr->evtime = lastime + 1 + 9 * jimsrand();

    /* simulate corruption: */
    if (jimsrand() < corruptprob)
    {
        ncorrupt++;
        if ((x = jimsrand()) < .75)
            mypktptr->payload[0] = 'Z'; /* corrupt payload */
        else if (x < .875)
            mypktptr->seqnum = 999999;
        else
            mypktptr->acknum = 999999;
        if (TRACE > 0)
            printf("          TOLAYER3: packet being corrupted\n");
    }

    if (TRACE > 2)
        printf("          TOLAYER3: scheduling arrival on other side\n");
    insertevent(evptr);
}

void tolayer5(int AorB, char datasent[20])
{
    int i;
    if (TRACE > 2)
    {
        printf("          TOLAYER5: data received: ");
        for (i = 0; i < 20; i++)
            printf("%c", datasent[i]);
        printf("\n");
    }
}
