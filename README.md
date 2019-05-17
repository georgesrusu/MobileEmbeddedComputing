# Mobile and Embedded Computing Project
## Introduction
The aim of this project is to propose an implementation of an IoT network using Rime where the sensor data is published through an MQTT-Rime gateway to normal MQTT subscribers. To achieve that we have implemented as requested by the project’s statement, a tree-based routing protocol using Rime and an MQTT- Rime gateway.

We implemented the project using Contiki OS (C language) and Python3. In the report you can find more details about the implementation, its strucutre, organizaion and workflow.

## Common
Contains the definitions of the messages :types and structures. This is common to sensors and to the root.

## SensorNode
In this foler you can find the sensor nodes code for the Contiki Cooja simulator

## RootNode
Contains the code for the root of network. This was also developped using the Cooja Simulator

## GatewayRoot
This contains the gateway implementation. This gateway act as a middleware between the root node (IOT sensor network) and the MQTT broker. We builded the gateway in Python3.

## MQTT-Subscriber
This is an implementation of an MQTT subscriber that will subscriber to some topics to the MQTT broker. This was implemented for testing purposes, in order to be easier to see if the sensor nodes are pushing data correctly.

## Requirements

Python3
Contiki OS/VM
Serial2pty Cooja's plugin

## Launching the project
1. Start the Cooja simulator
2. Add a root node
3. Add several sensor nodes and makes sure that at least one of them can reach the root
4. Launch the serial2pty plugin and take note of the open pipe
5. Launch the gateway.py:
    ```
    python3 gatewayRoot.py -s OPENED_PIPE
    ```
6. Launch the MQTT-Subscriber to receive the data:
   ```
   python3 subscriber.py -n [SRC_NODES_TO_SUBSCRIBES] -t [TOPICS_INTERESTED]
   ```
    Note: if no parameters are given, you will subscribe to all the nodes and topics available.