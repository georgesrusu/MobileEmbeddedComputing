/*
 * Authors : George Rusu, Jeremy Bricq, Ilias Boulif
 * Mobile Embedded Computing Project
 * Sensor Nodes Code
 */


#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include <stdio.h>
#include "../Common/messageTypes.h"

#define MAX_TRANSMISSION_PACKET 4
#define MAX_BROADCAST_PACKET_WITHOUT_PARENT 5

/*-------------------------------Variable Definition -------------------------------------*/
static struct broadcast_conn broadcastConnection;
static struct runicast_conn runicastConnection;
static uint16_t rank=0; //when rank is 0, means that it's a sensor and need for a parent
static uint16_t parentRank;
static linkaddr_t parentAddr;
static int broadcastPacketWithoutParentCounter=0;

/*-------------------------------Processes Definition -------------------------------------*/
PROCESS(broadcastProcess, "Broadcast communications");
PROCESS(runicastProcess, "Runicast communications");
AUTOSTART_PROCESSES(&broadcastProcess,&runicastProcess);
/*-------------------------------BroadCast Thread Definition --------------------------------------------*/

static void broadcastReceiver(struct broadcast_conn *c, const linkaddr_t *from){
    struct discovery_packet *pkt;
    pkt=packetbuf_dataptr();

    if (rank>0 && pkt->type == DISCOVERY_HELLO){
        printf("broadcast DISCOVERY_HELLO message received from %d.%d\n",from->u8[0], from->u8[1]);
        //DISCOVERY RESPONSE en UNICAST
        struct discovery_packet pkt_response;
        pkt_response.type=DISCOVERY_RESPONSE;
        pkt_response.rank=rank;
        packetbuf_copyfrom(&pkt_response, sizeof(struct discovery_packet));
        runicast_send(&runicastConnection, from,MAX_TRANSMISSION_PACKET);
    }
}

static const struct broadcast_callbacks broadcastCallback = {broadcastReceiver};

PROCESS_THREAD(broadcastProcess, ev, data){
    static struct etimer et;
    PROCESS_EXITHANDLER(broadcast_close(&broadcastConnection);)
    PROCESS_BEGIN();
    broadcast_open(&broadcastConnection, 129, &broadcastCallback);
    while(1) {
        /* Delay 2-4 seconds */
        etimer_set(&et, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
        //Parent Lost
        //TODO CHANGE TJAT
        if (broadcastPacketWithoutParentCounter>=MAX_BROADCAST_PACKET_WITHOUT_PARENT){
            printf("RESET PARENT");
            rank=0;
            parentAddr.u8[0]=0;
        }



        //BoradCast DISCOVERY-HELLO to discover neighbours and found parent
            struct discovery_packet pkt;
            pkt.type= DISCOVERY_HELLO;
            pkt.rank= rank;
            packetbuf_copyfrom(&pkt, sizeof(struct discovery_packet));
            broadcast_send(&broadcastConnection);
            printf("broadcast message sent\n");
        
        }
    PROCESS_END();
}

/*-------------------------------Runicast Thread Definition --------------------------------------------*/
static void runicastReceiver(struct runicast_conn *c, const linkaddr_t *from, uint8_t seqno){
    struct discovery_packet *pkt;
    pkt=packetbuf_dataptr();
    if (pkt->type ==DISCOVERY_RESPONSE){
        printf("RUNICAST DISCOVERY_RESPONSE message received from %d.%d WITH RANK %d\n",from->u8[0], from->u8[1],(int)pkt->rank);
        //If Motes is moved
         //TODO CHANGE TJAT
        if (from->u8[0] != parentAddr.u8[0]){
            printf("NOT FROM PARENT\n");
            broadcastPacketWithoutParentCounter++;
        }
        else{
            printf("FROM PARENT\n");
            broadcastPacketWithoutParentCounter=0;
        } 

        if (rank==0){
            printf("New Parent found with RANK %d, Parent is %d,%d\n",(int)pkt->rank,from->u8[0], from->u8[1]);
            rank=pkt->rank;
            parentAddr.u8[0]=from->u8[0];
            parentRank=rank;
            rank++;
        }
        else if ((pkt->rank)+1 < rank){
            printf("Parent Changed found with RANK %d, Parent is %d,%d\n",(int)pkt->rank,from->u8[0], from->u8[1]);
            rank=pkt->rank;
            parentAddr.u8[0]=from->u8[0];
            parentRank=rank;
            rank++;
        }
    }
}

static void runicastSender(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions){
     printf("runicast message sent to %d.%d: retransmissions %d\n", to->u8[0], to->u8[1], retransmissions);
}

static void runicastTimeOut(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions){
    printf("runicast message sent to %d.%d: timeout %d\n", to->u8[0], to->u8[1], retransmissions);
}
static const struct runicast_callbacks runicastCallback = {runicastReceiver, runicastSender,runicastTimeOut};

PROCESS_THREAD(runicastProcess, ev, data){
    PROCESS_EXITHANDLER(runicast_close(&runicastConnection);)
    PROCESS_BEGIN();
    runicast_open(&runicastConnection, 146, &runicastCallback);

    while(1) {
        //TODO Parent presence check
        PROCESS_YIELD();
    }

    PROCESS_END();
}