import sys
import os
import subprocess

I = 0xdeadbeef
D = 23.0
D2 = 23.0

print("pointers", [hex(id(I)), hex(id(D)), hex(id(D2))])

print("objects", [hex(I), D, D2])
subprocess.call(sys.argv[1:] + ["./poke", str(os.getpid()), hex(id(I))])
subprocess.call(sys.argv[1:] + ["./poke", str(os.getpid()), hex(id(D))])
print("object", [hex(I), D, D2])
