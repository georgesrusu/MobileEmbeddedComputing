#ifndef messageTypes_h
#define messageTypes_h

enum messageTypes{
    DISCOVERY_REQUEST,
    DISCOVERY_RESPONSE,
    ALIVE_REQUEST,
    ALIVE_RESPONSE,
    SENSOR_DATA,

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

enum dataSendingModes{
    DATA_ON_CHANGE, //0
    DATA_PERIODICALLY, //1
};
#endif