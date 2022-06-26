import subprocess
from subprocess import Popen
import os
print(os.getpid())
p1=Popen(["sysbench","--cpu-max-prime=1000" ,"--threads=1" ,"--time=0","cpu","run"])

# print(666)

p2=Popen(["sysbench","--cpu-max-prime=1000" ,"--threads=1" ,"--time=0","cpu","run"])

message = input("输入t kill所有子进程")

if(message=='t'):

  p1.kill()
  p2.kill()