/*
 * Authors : George Rusu, Jeremy Bricq, Ilias Boulif
 * Mobile Embedded Computing Project
 * Root Node Code
 */

#include "contiki-conf.h"
#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include <stdio.h>
#include "../Common/messageTypes.h"
#include "dev/serial-line.h"


#define MAX_TRANSMISSION_PACKET 4
/*-------------------------------Variable Definition -------------------------------------*/
static struct broadcast_conn broadcastConnection;
static struct runicast_conn runicastConnection;
static uint16_t rank=1; //root has rank 1 always, to create the tree
static uint8_t mode=DATA_PERIODICALLY; //default sending mode
static uint8_t haveSubscriber=0; //default no subscribers until the gateway is connected
/*-------------------------------Processes Definition -------------------------------------*/
PROCESS(broadcastProcess, "Broadcast communications");
PROCESS(runicastProcess, "Runicast communications");
PROCESS(serialProcess, "Serial communications with the gateway");
AUTOSTART_PROCESSES(&broadcastProcess,&runicastProcess,&serialProcess);
/*-------------------------------BroadCast Thread Definition --------------------------------------------*/

static void broadcastReceive(struct broadcast_conn *c, const linkaddr_t *from){
    struct packet *pkt;
    pkt=packetbuf_dataptr();

    if (pkt->type == DISCOVERY_REQUEST){ //receiving discovery request
        //printf("broadcast DISCOVERY_REQUEST message received from %d.%d\n",from->u8[0], from->u8[1]);
        //DISCOVERY RESPONSE en UNICAST
        struct packet pkt_response;
        pkt_response.type=DISCOVERY_RESPONSE;
        pkt_response.rank=rank;
        pkt_response.mode=mode;
        pkt_response.haveSubscriber=haveSubscriber; //send if there are subscriber
        int countTransmission=0;
        while (runicast_is_transmitting(&runicastConnection) && ++countTransmission<MAX_TRANSMISSION_PACKET){}
        packetbuf_copyfrom(&pkt_response, sizeof(struct packet));
        runicast_send(&runicastConnection, from,MAX_TRANSMISSION_PACKET);
    }
    
}

static const struct broadcast_callbacks broadcastCallback = {broadcastReceive};

PROCESS_THREAD(broadcastProcess, ev, data){
  static struct etimer et;

  PROCESS_EXITHANDLER(broadcast_close(&broadcastConnection);)

  PROCESS_BEGIN();

  broadcast_open(&broadcastConnection, 129, &broadcastCallback);

  while(1) {

    /* Delay 2-4 seconds */
    etimer_set(&et, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

  }

  PROCESS_END();
}

/*-------------------------------Runicast Thread Definition --------------------------------------------*/
static void runicastReceiver(struct runicast_conn *c, const linkaddr_t *from, uint8_t seqno){
    struct packet *pkt;
    struct data_packet *data_pkt;
    struct data_packet_aggregate *data_pkt_agg;
    pkt=packetbuf_dataptr();
    data_pkt=packetbuf_dataptr();
    data_pkt_agg=packetbuf_dataptr();
    if (pkt->type ==ALIVE_REQUEST){ //receiving alive_request
        printf("RUNICAST ALIVE_REQUEST message received from %d.%d \n",from->u8[0], from->u8[1]);
        struct packet pkt_response;
        pkt_response.type=ALIVE_RESPONSE;
        pkt_response.rank=rank;
        pkt_response.mode=mode;
        pkt_response.haveSubscriber=haveSubscriber;
        int countTransmission=0;
        while (runicast_is_transmitting(&runicastConnection) && ++countTransmission<MAX_TRANSMISSION_PACKET){}
        packetbuf_copyfrom(&pkt_response, sizeof(struct packet));
        runicast_send(c, from,MAX_TRANSMISSION_PACKET);
    }
    else if(data_pkt->type == SENSOR_DATA){ //receiving data and publish to Gateway
        printf("DATA,%d,%d,%d\n",data_pkt->nodeSrc,data_pkt->dataTemp,data_pkt->dataADXL); //IMPORTANT
    }
    else if (data_pkt_agg->type == SENSOR_DATA_AGGREGATE){ //In case of receiving a data aggregate packet -> need to decompose the two packets
        printf("receivede AGGREGATE PACKET\n");
        printf("received size %d\n",data_pkt_agg->numberPacket);
        if (data_pkt_agg->numberPacket >0){
            printf("DATA,%d,%d,%d\n",data_pkt_agg->packet1.nodeSrc,data_pkt_agg->packet1.dataTemp,data_pkt_agg->packet1.dataADXL); //IMPORTANT for publishing
        }
        if (data_pkt_agg->numberPacket >1){
            printf("DATA,%d,%d,%d\n",data_pkt_agg->packet2.nodeSrc,data_pkt_agg->packet2.dataTemp,data_pkt_agg->packet2.dataADXL); //IMPORTANT for publishing
        }
    }
}

static void runicastSender(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions){
    //printf("runicast message sent to %d.%d: retransmissions %d\n", to->u8[0], to->u8[1], retransmissions);
}

static void runicastTimeOut(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions){
    //printf("runicast message sent to %d.%d: timeout %d\n", to->u8[0], to->u8[1], retransmissions);
}
static const struct runicast_callbacks runicastCallback = {runicastReceiver, runicastSender,runicastTimeOut};

PROCESS_THREAD(runicastProcess, ev, data){
    PROCESS_EXITHANDLER(runicast_close(&runicastConnection);)
    PROCESS_BEGIN();

    runicast_open(&runicastConnection, 146, &runicastCallback);

    while(1) {
        PROCESS_YIELD();
    }

    PROCESS_END();
}

/*-------------------------------Serial Thread Definition --------------------------------------------*/
PROCESS_THREAD(serialProcess, ev, data)
{
    PROCESS_BEGIN();

    while(1) {
        PROCESS_YIELD();
        if(ev == serial_line_event_message) {
	        char* receivedData = (char *) data;
            printf("received line %s\n",(char *)data);
	        if(!strncmp(receivedData,"mode",4)){
                //Mode receving from console
                if(receivedData[5]=='0'){ // format is mode:0 or mode:1
                    mode = DATA_ON_CHANGE;
                    //printf("mode is DATA on CHANGE\n");
                } else if(receivedData[5]=='1'){
                    mode=DATA_PERIODICALLY;
                    //printf("mode is DATA PERIODICALLY\n");
                }

            } else {
                //receiving number of subscriber
                int connectedSubscribers = receivedData[0]-'0';
                //printf("subscribers are %d\n",connectedSubscribers);
                if(connectedSubscribers > 1){  //more than 1 because console is counted as one
                    //printf("MORE THAN 1 subscriber\n");
                    haveSubscriber=1;
                } else {
                    haveSubscriber=0;
                    //printf("STOP 0 SUBSCRIBERn");
                }

            }
        }
    }

    PROCESS_END();
}
