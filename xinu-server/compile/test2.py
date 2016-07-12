import subprocess
import sys
import os
import time
from subprocess import Popen, PIPE
def main():
    bbb = "beagle"
    
    p = Popen(["cs-console", "  ","beagle183"], stdin=PIPE) 
    p.stdin.write("\0")
    p.stdin.write('d')
    time.sleep(2)
    p.stdin.write('xinu.boot\n')
    time.sleep(1)
    p.stdin.write("\0")
    p.stdin.write('p')

    
main()
