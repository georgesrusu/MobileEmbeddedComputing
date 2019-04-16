#ifndef messageTypes_h
#define messageTypes_h

enum messageTypes{
    DISCOVERY_REQUEST,
    DISCOVERY_RESPONSE,
    ALIVE_REQUEST,
    ALIVE_RESPONSE,
    SENSOR_DATA,
    SENSOR_DATA_AGGREGATE,

};

struct packet{
    uint8_t type;
    uint16_t rank;
    uint8_t mode;
    uint8_t haveSubscriber;

};

struct data_packet{
    uint8_t type;
    uint16_t nodeSrc;
    uint16_t nodeRank;
    int8_t dataTemp; //allo from -128° to 128°
    int16_t dataOther; //to be changed
};

struct data_packet_aggregate{
    uint8_t type;
    uint8_t numberPacket;
    struct data_packet packet1;
    struct data_packet packet2;
};

enum dataSendingModes{
    DATA_ON_CHANGE, //0
    DATA_PERIODICALLY, //1
};
#endif