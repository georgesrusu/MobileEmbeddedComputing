#ifndef messageTypes_h
#define messageTypes_h

//types of packets
enum messageTypes{
    DISCOVERY_REQUEST,
    DISCOVERY_RESPONSE,
    ALIVE_REQUEST,
    ALIVE_RESPONSE,
    SENSOR_DATA,
    SENSOR_DATA_AGGREGATE,

};
//general struct of packet for keep Alive, routing
struct packet{
    uint8_t type;
    uint16_t rank;
    uint8_t mode;
    uint8_t haveSubscriber;

};
//general struct of data packets
struct data_packet{
    uint8_t type;
    uint16_t nodeSrc;
    uint16_t nodeRank;
    int8_t dataTemp; //allow from -128° to 128°
    int16_t dataADXL;
};

//general struct for data aggregation optimization
struct data_packet_aggregate{
    uint8_t type;
    uint8_t numberPacket;
    struct data_packet packet1;
    struct data_packet packet2;
};

//different sending modes
enum dataSendingModes{
    DATA_ON_CHANGE, //0
    DATA_PERIODICALLY, //1
};
#endif