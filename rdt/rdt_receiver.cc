/*
 * FILE: rdt_receiver.cc
 * DESCRIPTION: Reliable data transfer receiver.
 * NOTE: This implementation assumes there is no packet loss, corruption, or 
 *       reordering.  You will need to enhance it to deal with all these 
 *       situations.  In this implementation, the packet format is laid out as 
 *       the following:
 *       
 *       |<-  1 byte  ->|<-             the rest            ->|
 *       | payload size |<-             payload             ->|
 *
 *       The first byte of each packet indicates the size of the payload
 *       (excluding this single-byte header)
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rdt_struct.h"
#include "rdt_receiver.h"

int expected_index=0;
int countt=0;

/* receiver initialization, called once at the very beginning */
void Receiver_Init()
{
    fprintf(stdout, "At %.2fs: receiver initializing ...\n", GetSimulationTime());
}

/* receiver finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to use this opportunity to release some 
   memory you allocated in Receiver_init(). */
void Receiver_Final()
{
    fprintf(stdout, "At %.2fs: receiver finalizing ...\n", GetSimulationTime());
}

void set_cchecksum(packet& pkt, int size)
{
    unsigned short* cursor = (unsigned short*)(pkt.data);
    unsigned long Cksum=0;
    while (size>1)
    {
        Cksum +=*(cursor++);
        size -=sizeof(unsigned short);
    }
    if (size)
    {
        Cksum +=*(unsigned char *) cursor;                          
    }
    while (Cksum>>16)  Cksum = (Cksum>>16) + (Cksum & 0xffff);
    pkt.data[6]=(~Cksum) & 0xFF;
    pkt.data[7]=((~Cksum) >> 8) & 0xFF;
}

bool ccheck_sum(struct packet* pkt, int size)
{
    unsigned short* cursor = (unsigned short*)(pkt->data);
    unsigned long Cksum=0;
    while (size>1)
    {
        Cksum +=*(cursor++);
        size -=sizeof(unsigned short);
    }
    if (size)
    {
        Cksum +=*(unsigned char *) cursor;
    }
    while (Cksum>>16)  Cksum = (Cksum>>16) + (Cksum & 0xffff);
    //printf("CHECKSUM: %lx\n",Cksum);
    return (Cksum) == 0xFFFF;
}

/* event handler, called when a packet is passed from the lower layer at the 
   receiver */
void Receiver_FromLowerLayer(struct packet *pkt)
{
    /* 1-byte header indicating the size of the payload */
    countt++;
    int header_size = 10;
    int *cursor=(int*)(pkt->data);
    int index=*(cursor+1);
    //printf("INDEX: %d Expected: %d  RIGHT: %d\n",index,expected_index,ccheck_sum(pkt,RDT_PKTSIZE));

    if (index!=expected_index && index!=expected_index-1) return;
    if (!ccheck_sum(pkt,RDT_PKTSIZE)) return;
    /* construct a message and deliver to the upper layer */
    packet ppkt;

    ppkt.data[0]=index & 0xFF;
    ppkt.data[1]=(index >> 8) & 0xFF;
    ppkt.data[2]=(index >> 16) & 0xFF;
    ppkt.data[3]=(index >> 24) & 0xFF;
    ppkt.data[4]=ppkt.data[5]=ppkt.data[6]=ppkt.data[7]=0;
    set_cchecksum(ppkt,8);
    Receiver_ToLowerLayer(&ppkt);

    if (index!=expected_index) return;
    expected_index++;

    struct message *msg = (struct message*) malloc(sizeof(struct message));
    ASSERT(msg!=NULL);

    msg->size = pkt->data[0];

    /* sanity check in case the packet is corrupted */
    if (msg->size<0) msg->size=0;
    if (msg->size>RDT_PKTSIZE-header_size) msg->size=RDT_PKTSIZE-header_size;

    msg->data = (char*) malloc(msg->size);
    ASSERT(msg->data!=NULL);
    memcpy(msg->data, pkt->data+header_size, msg->size);
    Receiver_ToUpperLayer(msg);

    /* don't forget to free the space */
    if (msg->data!=NULL) free(msg->data);
    if (msg!=NULL) free(msg);


}
