from subprocess import *
import os
import argparse

def subscribe(node="+",topic="+"):
    cmd=["mosquitto_sub"]
    for nodeID in node:
        for topicName in topic:
            cmd.append("-t")
            if node!="+":
                cmd.append("node"+str(nodeID)+"/"+str(topicName))
            else:
                cmd.append(str(nodeID)+"/"+str(topicName))
    p = Popen(cmd, stdout = PIPE, stdin = PIPE)
    while True:
        line = p.stdout.readline()
        #line1 = p1.stdout.readline()
        print(line)
        #print(line1)
        if line == '' and p.poll() != None:
            break

if __name__=="__main__":
    parser = argparse.ArgumentParser(description='MQTT subscriber')
    parser.add_argument('-n',help='nodes to subscribe')
    parser.add_argument('-t',help='topics to subscribe')
    args = parser.parse_args()
 

    #    nodeList=[2,3]
    #    topicList=["temperature"]
        #subscribe(node=nodeList,topic=topicList)
   #     subscribe()