from subprocess import *
import os
def readFromRoot(serial):
    p = Popen(["make","login","TARGET=z1","MOTES="+serial], stdout = PIPE, stdin = PIPE)
    while True:
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
    serial="/dev/pts/7"
    readFromRoot(serial)
"""
faire un serial serveur -> faire dans le dossier rootnode make login target motes
 make login TARGET=z1 MOTES=/dev/pts/7

 #TODO Gateway code
 #TODO serial thread in root node
 #TODO changement message pour detecter dans le gateway bien !
"""