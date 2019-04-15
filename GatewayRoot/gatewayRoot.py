from subprocess import *
import os
import time
import threading
import argparse


modeInput="-1"
all_modes=["0","1"]
def console(threadName,process):
    global modeInput
    while True:
        modeInput=input()
      
    
def readFromRoot(serial):
    mode="1"
    p = Popen(["make","login","TARGET=z1","MOTES="+serial], stdout = PIPE, stdin = PIPE)
    #p1 =Popen(["mosquitto_sub","-t","$SYS/broker/clients/active"], stdout = p.stdin, stdin = PIPE)
    print("To change mode just type 0 for DATA_ON_CHANGE and 1 DATA_PERIODICALLY \n")
    threading1 = threading.Thread(target=console,args=("console",p))
    threading1.daemon = True
    threading1.start()
    while True:
        print(mode)
        if modeInput in all_modes and modeInput != mode:
            mode=modeInput
            print("mode changed",mode)
            messageToRoot=("mode:"+str(mode)).encode("utf-8")
            p.stdin.write("coucou".encode("utf-8"))
        line = p.stdout.readline()
        if line == '' and p.poll() != None:
            break
        if line[0:4]== b"DATA":
            values=line.decode("utf-8")
            values=values.strip()
            _,src,tmp,other=values.split(",")
            print("VALUES src="+str(src)+" tmp="+str(tmp)+" other="+str(other))
            call(["mosquitto_pub","-t","node"+src+"/temperature","-m","Node"+src+" temperature is: "+tmp+" degrees C"])
            call(["mosquitto_pub","-t","node"+src+"/other","-m","Node"+src+" other is: "+other+" UNIT"])

if __name__=="__main__":
    os.chdir(os.getcwd()+"/../rootNode")
    parser = argparse.ArgumentParser(description='Gateway in between the broker and the root mote')
    parser.add_argument('-s',help='serial to attache to',required=True)
    args = parser.parse_args()
    if args.s is not None:
        serial=args.s
    readFromRoot(serial)


"""
faire un serial serveur -> faire dans le dossier rootnode make login target motes
 make login TARGET=z1 MOTES=/dev/pts/7

 #TODO Gateway code
 #TODO serial thread in root node
 #TODO changement message pour detecter dans le gateway bien !
"""