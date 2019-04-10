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
#include "node-id.h"
#define MAX_TRANSMISSION_PACKET 4
#define MAX_PACKET_PARENT_ALIVE 5

/*-------------------------------Variable Definition -------------------------------------*/
static struct broadcast_conn broadcastConnection;
static struct runicast_conn runicastConnection;
static uint16_t rank=0; //when rank is 0, means that it's a sensor and need for a parent
static uint16_t parentRank;
static linkaddr_t parentAddr;
static int ParentAliveCounter=0;
static int randomSensorData=1;

/*-------------------------------Processes Definition -------------------------------------*/
PROCESS(broadcastProcess, "Broadcast communications");
PROCESS(runicastProcess, "Runicast communications");
PROCESS(getDataProcess, "Get sensor data communications");
AUTOSTART_PROCESSES(&broadcastProcess,&runicastProcess,&getDataProcess);
/*-------------------------------BroadCast Thread Definition --------------------------------------------*/

static void broadcastReceiver(struct broadcast_conn *c, const linkaddr_t *from){
    struct packet *pkt;
    pkt=packetbuf_dataptr();

    if (rank>0 && pkt->type == DISCOVERY_REQUEST){
        //printf("broadcast DISCOVERY_REQUEST message received from %d.%d\n",from->u8[0], from->u8[1]);
        //DISCOVERY RESPONSE en UNICAST
        struct packet pkt_response;
        pkt_response.type=DISCOVERY_RESPONSE;
        pkt_response.rank=rank;
        packetbuf_copyfrom(&pkt_response, sizeof(struct packet));
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

        //check if parent is still there
        if (ParentAliveCounter > MAX_PACKET_PARENT_ALIVE){
            printf("RESET PARENT\n");
            rank=0; //if rank=0 parent will be overiden
            ParentAliveCounter=0;
            //parentAddr=null;
        }
       
        //BoradCast DISCOVERY-HELLO to discover neighbours and found parent
        struct packet pkt;
        pkt.type= DISCOVERY_REQUEST;
        pkt.rank= rank;
        packetbuf_copyfrom(&pkt, sizeof(struct packet));
        broadcast_send(&broadcastConnection);
        //printf("broadcast message sent\n");
        
        }
    PROCESS_END();
}

/*-------------------------------Runicast Thread Definition --------------------------------------------*/
static void runicastReceiver(struct runicast_conn *c, const linkaddr_t *from, uint8_t seqno){
    struct packet *pkt;
    struct data_packet *data_pkt;
    pkt=packetbuf_dataptr();
    data_pkt=packetbuf_dataptr();
    if (pkt->type ==DISCOVERY_RESPONSE){
        //printf("RUNICAST DISCOVERY_RESPONSE message received from %d.%d WITH RANK %d\n",from->u8[0], from->u8[1],(int)pkt->rank);
        if (rank==0){
            printf("New Parent found with RANK %d, Parent is %d,%d\n",(int)pkt->rank,from->u8[0], from->u8[1]);
            rank=pkt->rank;
            parentAddr=*from;
            parentRank=rank;
            rank++;
        }
        else if ((pkt->rank)+1 < rank){
            printf("Parent Changed found with RANK %d, Parent is %d,%d\n",(int)pkt->rank,from->u8[0], from->u8[1]);
            rank=pkt->rank;
            parentAddr=*from;
            parentRank=rank;
            rank++;
        }
    }
    else if (pkt->type ==ALIVE_REQUEST){
        printf("RUNICAST ALIVE_REQUEST message received from %d.%d \n",from->u8[0], from->u8[1]);
        struct packet pkt_response;
        pkt_response.type=ALIVE_RESPONSE;
        pkt_response.rank=rank;
        packetbuf_copyfrom(&pkt_response, sizeof(struct packet));
        runicast_send(&runicastConnection, from,MAX_TRANSMISSION_PACKET);
    }
    else if (pkt->type == ALIVE_RESPONSE){
        printf("RUNICAST ALIVE_RESPONSE message received from %d.%d \n",from->u8[0], from->u8[1]);
        if (pkt->rank == 0){
            printf("RESETPARENTFROM ALIVE pACKET\n");
            rank=0;
        }
        ParentAliveCounter=0;
    }
    else if(data_pkt->type == SENSOR_DATA){
        if (rank>1){
            printf("Retransmission\n");
            packetbuf_copyfrom(data_pkt, sizeof(struct data_packet));
            runicast_send(&runicastConnection, &parentAddr,MAX_TRANSMISSION_PACKET);
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
    static struct etimer et;
    PROCESS_EXITHANDLER(runicast_close(&runicastConnection);)
    PROCESS_BEGIN();
    runicast_open(&runicastConnection, 146, &runicastCallback);

    while(1) {
  
        etimer_set(&et, CLOCK_SECOND * 3 + random_rand() % (CLOCK_SECOND * 2));
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
        if (rank>1){
            ParentAliveCounter++;
            struct packet pkt;
            pkt.type= ALIVE_REQUEST;
            packetbuf_copyfrom(&pkt, sizeof(struct packet));
            runicast_send(&runicastConnection, &parentAddr ,MAX_TRANSMISSION_PACKET);
        }
    }

    PROCESS_END();
}

/*-------------------------------Get Data Thread Definition --------------------------------------------*/
PROCESS_THREAD(getDataProcess, ev, data){
    static struct etimer et;
    PROCESS_EXITHANDLER(broadcast_close(&broadcastConnection);)
    PROCESS_BEGIN();
    broadcast_open(&broadcastConnection, 129, &broadcastCallback);
    while(1) {
        /* Delay 2-4 seconds */
        etimer_set(&et, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
        if (rank>1){
            struct data_packet data_pkt;
            if (randomSensorData){
                data_pkt.type=SENSOR_DATA;
                data_pkt.nodeSrc=node_id;
                data_pkt.nodeRank=rank;
                data_pkt.dataTemp=random_rand()%128; //datatemp on 8 bit
                data_pkt.dataOther=random_rand()%256; //data on 16 bit
            }else{
                printf("real hardware used\n");
                //TODO complete for real hardware
            }
            //TODO Sender mode, periodical and differential

       
            printf("sent sensor data\n");
            packetbuf_copyfrom(&data_pkt, sizeof(struct data_packet));
            runicast_send(&runicastConnection, &parentAddr,MAX_TRANSMISSION_PACKET);
        }
    }
    PROCESS_END();
}