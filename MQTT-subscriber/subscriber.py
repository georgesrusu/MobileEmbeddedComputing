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
    parser.add_argument('-n',help='nodes to subscribe,ex: 1,2,3.If none -> all')
    parser.add_argument('-t',help='topics to subscribe,ex: temperature,other. If none-> all')
    args = parser.parse_args()
    if args.n is not None:
        nodeList=list(args.n.split(","))
    else:
        nodeList="+"

    if args.t is not None:
        topicList=list(args.t.split(","))
    else:
        topicList="+"
    subscribe(node=nodeList,topic=topicList)