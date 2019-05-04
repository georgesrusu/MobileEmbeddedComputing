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
#include "dev/tmp102.h"
#include "dev/battery-sensor.h"

#define MAX_TRANSMISSION_PACKET 4
#define MAX_PACKET_PARENT_ALIVE 4
#define MAX_PACKET_RUNICAST_FOR_AGGREGATION 5
#define MAX_PACKET_AGGREGATE 2


/*-------------------------------Variable Definition -------------------------------------*/
static struct broadcast_conn broadcastConnection;
static struct runicast_conn runicastConnection;
static uint16_t rank=0; //when rank is 0, means that it's a sensor and need for a parent
static uint16_t parentRank;
static linkaddr_t parentAddr;
static int ParentAliveCounter=0;
static int randomSensorData=1;
static uint8_t mode;
static uint8_t haveSubscriber=0;
static int8_t oldDataTemp=0;
static int16_t oldDataOther=0; 
static struct data_packet_aggregate dataPacketAggregate;
static int packetAggregateCounter=0;
static int countPacketRunicast=0;
/*-------------------------------Processes Definition -------------------------------------*/
PROCESS(broadcastProcess, "Broadcast communications");
PROCESS(runicastProcess, "Runicast communications");
PROCESS(getDataProcess, "Get sensor data communications");
AUTOSTART_PROCESSES(&broadcastProcess,&runicastProcess,&getDataProcess);
/*-------------------------------BroadCast Thread Definition --------------------------------------------*/

static void broadcastReceiver(struct broadcast_conn *c, const linkaddr_t *from){
    struct packet *pkt;
    pkt=packetbuf_dataptr();
    
    if (rank>0 && pkt->type == DISCOVERY_REQUEST){ //Discovery Request received from children
        //printf("broadcast DISCOVERY_REQUEST message received from %d.%d\n",from->u8[0], from->u8[1]);
        //DISCOVERY RESPONSE en UNICAST
        struct packet pkt_response;
        pkt_response.type=DISCOVERY_RESPONSE;
        pkt_response.rank=rank;
        pkt_response.mode=mode;
        pkt_response.haveSubscriber=haveSubscriber;
        int countTransmission=0;
        while (runicast_is_transmitting(&runicastConnection) && ++countTransmission<MAX_TRANSMISSION_PACKET){}
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
    struct data_packet_aggregate *data_pkt_agg;
    pkt=packetbuf_dataptr();
    data_pkt=packetbuf_dataptr();
    data_pkt_agg=packetbuf_dataptr();
    printf("RUNICAST IN MESSAGE\n");
    if (pkt->type ==DISCOVERY_RESPONSE){ //received Discoveru Response from potential future parent
        //printf("RUNICAST DISCOVERY_RESPONSE message received from %d.%d WITH RANK %d\n",from->u8[0], from->u8[1],(int)pkt->rank);
        if (rank==0){ //if no parent , received is new parent
            printf("New Parent found with RANK %d, Parent is %d,%d\n",(int)pkt->rank,from->u8[0], from->u8[1]);
            rank=pkt->rank;
            parentAddr=*from;
            parentRank=rank;
            mode=pkt->mode;
            rank++;
        }
        else if ((pkt->rank)+1 < rank){ //if we have a parent but the received is better than the one we have, change parent to new
            printf("Parent Changed found with RANK %d, Parent is %d,%d\n",(int)pkt->rank,from->u8[0], from->u8[1]);
            rank=pkt->rank;
            parentAddr=*from;
            parentRank=rank;
            mode=pkt->mode;
            rank++;
        }
    }
    else if (pkt->type ==ALIVE_REQUEST){ //alive message for keeping the parenting relation
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
    else if (pkt->type == ALIVE_RESPONSE){ //alive response to the alive request message
        printf("RUNICAST ALIVE_RESPONSE message received from %d.%d \n",from->u8[0], from->u8[1]);
        mode=pkt->mode;
        haveSubscriber=pkt->haveSubscriber;
        if (pkt->rank == 0 || pkt->rank > rank){ //reset parent chain, if the parent of our parent reset his rank (loose parent) we are also reseting
            printf("RESETPARENTFROM ALIVE pACKET\n");
            rank=0;
        }
        ParentAliveCounter=0;
    }
    else if(data_pkt->type == SENSOR_DATA){ //sensor data message and we aggregate 2 into 1
        printf("received sensor data from %d\n",from->u8[0]);
        if (rank>1){
            printf("packet counter is %d\n",packetAggregateCounter);
            if (packetAggregateCounter==0){
                countPacketRunicast=0;
                packetAggregateCounter++;
                printf("creation 1 packet aggregate\n");
                dataPacketAggregate.type=SENSOR_DATA_AGGREGATE;
                dataPacketAggregate.numberPacket=packetAggregateCounter;
                dataPacketAggregate.packet1=*data_pkt;
            }else if (packetAggregateCounter==1){
                printf("creation 2 packet aggregate\n");
                packetAggregateCounter++;
                dataPacketAggregate.numberPacket=packetAggregateCounter;
                dataPacketAggregate.packet2=*data_pkt;
            }
        }
    }
    else if (data_pkt_agg->type == SENSOR_DATA_AGGREGATE){ //OPTIMIZATION: in case we receive an aggregate packet that is half empty we fill it with a data packet
        if (rank>1){
            int countTransmission=0;
            while (runicast_is_transmitting(&runicastConnection) && ++countTransmission<MAX_TRANSMISSION_PACKET){}
            if (data_pkt_agg->numberPacket == MAX_PACKET_AGGREGATE){
                printf("Retransmission AGGEGATION\n");
                packetbuf_copyfrom(data_pkt_agg, sizeof(struct data_packet_aggregate));
                runicast_send(c, &parentAddr,MAX_TRANSMISSION_PACKET);
            }
            else{
                printf("AGGEGATION NON Remplie\n");
                if (packetAggregateCounter==1){
                    printf("remplir packet aggreagation\n");
                    data_pkt_agg->numberPacket=MAX_PACKET_AGGREGATE;
                    data_pkt_agg->packet2=dataPacketAggregate.packet1;
                    packetAggregateCounter--;
                    packetbuf_copyfrom(data_pkt_agg, sizeof(struct data_packet_aggregate));
                    runicast_send(c, &parentAddr,MAX_TRANSMISSION_PACKET);
                }
                else{
                    printf("Retransmission AGGEGATION NON rempli\n");
                    packetbuf_copyfrom(data_pkt_agg, sizeof(struct data_packet_aggregate));
                    runicast_send(c, &parentAddr,MAX_TRANSMISSION_PACKET);
                }
            }

        }
    }
    //if packet aggregate is full or if after X packet the aggregate is not yet full, we send it half full
    countPacketRunicast++;
    if (countPacketRunicast > MAX_PACKET_RUNICAST_FOR_AGGREGATION || packetAggregateCounter==MAX_PACKET_AGGREGATE) {
        if (rank>1 && packetAggregateCounter>0){
            int countTransmission=0;
            while (runicast_is_transmitting(&runicastConnection) && ++countTransmission<MAX_TRANSMISSION_PACKET){}
            printf("Transmission AGGERGATION\n");
            packetbuf_copyfrom(&dataPacketAggregate, sizeof(struct data_packet_aggregate));
            runicast_send(c, &parentAddr,MAX_TRANSMISSION_PACKET);
            packetAggregateCounter=0;

        }
        countPacketRunicast=0;
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
  
        if (rank>1){
            ParentAliveCounter++;
            struct packet pkt;
            pkt.type= ALIVE_REQUEST;
            int countTransmission=0;
            while (runicast_is_transmitting(&runicastConnection) && ++countTransmission<MAX_TRANSMISSION_PACKET){}
            //printf("ALIVE REQUEST SEND\n");
            packetbuf_copyfrom(&pkt, sizeof(struct packet));
            runicast_send(&runicastConnection, &parentAddr ,MAX_TRANSMISSION_PACKET);
            //}
        }
        etimer_set(&et, CLOCK_SECOND * 3 + random_rand() % (CLOCK_SECOND * 2));
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
        //check if parent is still there
        if (ParentAliveCounter > MAX_PACKET_PARENT_ALIVE){
            printf("RESET PARENT\n");
            rank=0; //if rank=0 parent will be overiden
            ParentAliveCounter=0;
            //parentAddr=null;
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
        etimer_set(&et, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
        if (rank>1 && haveSubscriber){
            struct data_packet data_pkt;
            data_pkt.type=SENSOR_DATA;
            data_pkt.nodeSrc=node_id;
            data_pkt.nodeRank=rank;
            if (randomSensorData){
                data_pkt.dataTemp= random_rand()%128; //datatemp on 8 bit
                data_pkt.dataOther=random_rand()%256; //data on 16 bit
                //data_pkt.dataTemp= -26; //datatemp on 8 bit
                //data_pkt.dataOther=10;
            }else{
                printf("real hardware used\n");
                tmp102_init();
                SENSORS_ACTIVATE(battery_sensor);
                data_pkt.dataTemp= tmp102_read_temp_raw(); //datatemp on 8 bit
                //data_pkt.dataOther=tmp102_read_temp_raw(); //data on 16 bit
                data_pkt.dataOther=battery_sensor.value(0);//data on 16 bit
                //TODO complete for real hardware
            }
            //changing modes
            if (mode == DATA_PERIODICALLY){
                /* Delay 2-4 seconds */
                //printf("MODE PERIODICALLY ACTIVATED\n");
                int countTransmission=0;
                while (runicast_is_transmitting(&runicastConnection) && ++countTransmission<MAX_TRANSMISSION_PACKET){}
                printf("sent sensor data\n");
                packetbuf_copyfrom(&data_pkt, sizeof(struct data_packet));
                runicast_send(&runicastConnection, &parentAddr,MAX_TRANSMISSION_PACKET);
            }
            else if (mode == DATA_ON_CHANGE){
                //printf("MODE DATA ON CHANGE ACTIVATED\n");
                if (oldDataTemp != data_pkt.dataTemp || oldDataOther != data_pkt.dataOther){ //if data from previous packet as been changed
                    //printf("DATA ON CHANGE\n");
                    oldDataTemp=data_pkt.dataTemp; //previous temp is different
                    oldDataOther=data_pkt.dataOther; //previous data is different
                    int countTransmission=0;
                    while (runicast_is_transmitting(&runicastConnection) && ++countTransmission<MAX_TRANSMISSION_PACKET){}
                    printf("sent sensor data\n");
                    packetbuf_copyfrom(&data_pkt, sizeof(struct data_packet));
                    runicast_send(&runicastConnection, &parentAddr,MAX_TRANSMISSION_PACKET);
                    
                }
            }
        }
    }
    PROCESS_END();
}