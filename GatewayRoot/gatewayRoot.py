import subprocess

def readFromRoot(cmd):
    p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    stdout = []
    while True:
        line = p.stdout.readline()
        stdout.append(line)
        print line,
        if line == '' and p.poll() != None:
            break
    return ''.join(stdout)


"""
faire un serial serveur -> faire dans le dossier rootnode make login target motes
 make login TARGET=z1 MOTES=/dev/pts/7

 #TODO Gateway code
 #TODO serial thread in root node
 #TODO changement message pour detecter dans le gateway bien !
"""