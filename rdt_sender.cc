/*
 * FILE: rdt_sender.cc
 * DESCRIPTION: Reliable data transfer sender.
 * NOTE: This implementation assumes there is no packet loss, corruption, or 
 *       reordering.  You will need to enhance it to deal with all these 
 *       situations.  In this implementation, the packet format is laid out as 
 *       the following:
 *       
 *       |<- 10 byte  ->|<-             the rest            ->|
 *       |    Header    |<-             payload             ->|
 *
 *       The first byte of each packet indicates the size of the payload
 *       (excluding this single-byte header)
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <queue>

#include "rdt_struct.h"
#include "rdt_sender.h"

#define PKT_SIZE 128

#define WINDOW_SIZE 5
#define TIME_OUT 0.3
#define CORRUPTED_INDEX 150000

 using namespace std;


int inddex=0;
int window_tail_inddex=0;
int sending_allowed_inddex=0;
int curr_total_num=0;
int total_chara=0;
queue<packet>buffer;
queue<packet>window;
queue<double>expected_time;

/* sender initialization, called once at the very beginning */
void Sender_Init()
{
    fprintf(stdout, "At %.2fs: sender initializing ...\n", GetSimulationTime());
}

/* sender finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to take this opportunity to release some 
   memory you allocated in Sender_init(). */
void Sender_Final()
{

    fprintf(stdout, "At %.2fs: sender finalizing ...\n", GetSimulationTime());
}

void set_checksum(packet& pkt, int size)
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
    pkt.data[8]=(~Cksum) & 0xFF;
    pkt.data[9]=((~Cksum) >> 8) & 0xFF;
}

bool check_sum(struct packet* pkt, int size)
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
    return (Cksum) == 0xFFFF;
}

/* event handler, called when a message is passed from the upper layer at the 
   sender */
void Sender_FromUpperLayer(struct message *msg)
{
    /* 1-byte header indicating the size of the payload */
    int header_size = 10;

    static int msgNum = 0;

    int seqNum = 0;
    /* maximum payload size */
    int maxpayload_size = RDT_PKTSIZE - header_size;

    curr_total_num+=(msg->size+maxpayload_size-1)/maxpayload_size;

    total_chara+=msg->size;

    /* split the message if it is too big */

    /* reuse the same packet data structure */
    packet pkt;

    /* the cursor always points to the first unsent byte in the message */
    int cursor = 0;

    while (msg->size-cursor > maxpayload_size) {
	/* fill in the packet */
	pkt.data[0] = maxpayload_size;
	pkt.data[1] = msgNum & 0xFF;
	pkt.data[2] = seqNum & 0xFF;
	pkt.data[3] = 0;
	pkt.data[4] = inddex & 0xFF;
	pkt.data[5] = (inddex>>8) & 0xFF;
	pkt.data[6] = (inddex>>16) & 0xFF;
	pkt.data[7] = (inddex>>24) & 0xFF;
	pkt.data[8] = 0;
	pkt.data[9] = 0;
	memcpy(pkt.data+header_size, msg->data+cursor, maxpayload_size);

	set_checksum(pkt,PKT_SIZE);
	//printf("check_sum: %d\n",check_sum(pkt,PKT_SIZE))
	/* send it out through the lower layer */
	if (buffer.empty())
	{
		if (inddex==sending_allowed_inddex && sending_allowed_inddex-window_tail_inddex < WINDOW_SIZE)
		{
			window.push(pkt);
			sending_allowed_inddex++;
			Sender_ToLowerLayer(&pkt);
			expected_time.push(GetSimulationTime()+TIME_OUT);
		}
		else
		{
			buffer.push(pkt);
		}
	}
	else 
	{
		buffer.push(pkt);
		pkt=buffer.front();
		int* cursor=(int*)(pkt.data);
		int id=*(cursor+1);
		while (id==sending_allowed_inddex && sending_allowed_inddex-window_tail_inddex < WINDOW_SIZE)
		{
			//printf("BUFFER ID: %d\n",id);
			buffer.pop();
			window.push(pkt);
			sending_allowed_inddex++;
			Sender_ToLowerLayer(&pkt);
			expected_time.push(GetSimulationTime()+TIME_OUT);
			if (buffer.empty()) break;
			pkt=buffer.front();
			id=*((int*)(pkt.data)+1);
		}
	}

	/* move the cursor */
	cursor += maxpayload_size;

	seqNum++;
	inddex++;
    }

    /* send out the last packet */
    if (msg->size > cursor) 
    {
		/* fill in the packet */
		pkt.data[0] = msg->size-cursor;
		pkt.data[1] = msgNum & 0xFF;
		pkt.data[2] = seqNum & 0xFF;
		pkt.data[3] = 0;
		pkt.data[4] = inddex & 0xFF;
		pkt.data[5] = (inddex>>8) & 0xFF;
		pkt.data[6] = (inddex>>16) & 0xFF;
		pkt.data[7] = (inddex>>24) & 0xFF;
		pkt.data[8] = 0;
		pkt.data[9] = 0;
		memcpy(pkt.data+header_size, msg->data+cursor, pkt.data[0]);
		set_checksum(pkt,PKT_SIZE);
		/* send it out through the lower layer */
		if (buffer.empty())
		{
			if (window.empty()) window_tail_inddex=inddex;
			if (inddex==sending_allowed_inddex && sending_allowed_inddex-window_tail_inddex < WINDOW_SIZE)
			{
				window.push(pkt);
				sending_allowed_inddex++;
				Sender_ToLowerLayer(&pkt);
				expected_time.push(GetSimulationTime()+TIME_OUT);
			}
			else
			{
				buffer.push(pkt);
			}
			inddex++;
		}
		else 
		{
			buffer.push(pkt);
			pkt=buffer.front();
			int* cursor=(int*)(pkt.data);
			int id=*(cursor+1);
			while (id==sending_allowed_inddex && sending_allowed_inddex-window_tail_inddex < WINDOW_SIZE)
			{
				//printf("BUFFER ID: %d\n",id);
				if (window.empty()) window_tail_inddex=id;
				buffer.pop();
				window.push(pkt);
				sending_allowed_inddex++;
				Sender_ToLowerLayer(&pkt);
				expected_time.push(GetSimulationTime()+TIME_OUT);
				if (buffer.empty()) break;
				pkt=buffer.front();
				id=*((int*)(pkt.data)+1);
			}
			inddex++;
	    }
	}
//printf("SENDING window SIZE: %lu\n",window.size());
	//printf("TOTAL CHARA: %d TOTAL NUM: %d  CURR_ID: %d\n",total_chara,curr_total_num,inddex);
	//printf("BUFFER SIZE %lu\n",buffer.size());

    if(!Sender_isTimerSet()) 
	{
		Sender_StartTimer(TIME_OUT);
	}
    msgNum++;
}

/* event handler, called when a packet is passed from the lower layer at the 
   sender */
void Sender_FromLowerLayer(struct packet *pkt)
{
	int acknum=*(int*)(pkt->data);
	int size=8;
	//int wsize=window.size();
	//printf("acknum: %d tail: %d\n",acknum,window_tail_inddex);
	//printf("checksum: %d\n",check_sum(pkt,size));
	if (!check_sum(pkt,size) || acknum>CORRUPTED_INDEX) return;
	//printf("BEFORE SIZE: %lu acknum: %d window_TAIL: %d\n",window.size(),acknum ,window_tail_inddex);
	if (acknum >= window_tail_inddex)
	{
		for (int i = window_tail_inddex; i <= acknum; i++)
		{
			if (window.empty()) break;
			window.pop();
			expected_time.pop();
		}
		if (!expected_time.empty()) Sender_StartTimer(expected_time.front()-GetSimulationTime());
		else Sender_StopTimer();
		window_tail_inddex=acknum+1;
	}
	if (!buffer.empty())
	{
		packet ppkt=buffer.front();
		int* cursor=(int*)(ppkt.data);
		int id=*(cursor+1);
		while (id==sending_allowed_inddex && sending_allowed_inddex-window_tail_inddex < WINDOW_SIZE)
		{
			//printf("BUFFER ID: %d\n",id);
			if (window.empty()) window_tail_inddex=id;
			buffer.pop();
			window.push(ppkt);
			sending_allowed_inddex++;
			Sender_ToLowerLayer(&ppkt);
			expected_time.push(GetSimulationTime()+TIME_OUT);
			if (buffer.empty()) break;
			ppkt=buffer.front();
			id=*((int*)(ppkt.data)+1);
		}
		if (!Sender_isTimerSet()) Sender_StartTimer(TIME_OUT);
	}
	
	if (window.empty())return;
	packet pppkt=window.front();
	int* cursor=(int*)(pppkt.data);
	assert(*(cursor+1)==window_tail_inddex);
	//printf("SIZE: %lu acknum: %d ACTUAL TAIL:%d  window_TAIL: %d\n",window.size(),acknum, *(cursor+1),window_tail_inddex);
    
}

/* event handler, called when the timer expires */
void Sender_Timeout()
{
	int window_size=window.size();
	packet pkt;
	while (!expected_time.empty()) expected_time.pop();

	for (int i=0;i<window_size;i++)
	{
		pkt=window.front();
		Sender_ToLowerLayer(&pkt);
		if (i==0) Sender_StartTimer(TIME_OUT);
		window.pop();
		expected_time.push(GetSimulationTime()+TIME_OUT);
		window.push(pkt);
	}
	assert(window_size==window.size());
}
