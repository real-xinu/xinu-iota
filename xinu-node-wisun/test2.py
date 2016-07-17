import subprocess
import sys
import os
import time
from subprocess import Popen, PIPE
def main():
  bbb = "beagle"
  for i in range(102,119):   
    p = Popen(["cs-console", "  ",bbb + str(i)], stdin=PIPE) 
    p.stdin.write("\0")
    p.stdin.write('d')
    time.sleep(2)
    p.stdin.write('xinu.boot\n')
    time.sleep(1)
    p.stdin.write("\0")
    p.stdin.write('p')
    print(i)
    time.sleep(1)
main()
