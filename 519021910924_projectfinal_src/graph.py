
import matplotlib.pyplot as plt
import numpy as np
def process(filepath1):
  f = open(filepath1,encoding='utf-8')
  cpu=[]
  mem=[]
  while True:
        line = f.readline()
        if line:
            cpu_tmp,mem_tmp=str(line).split(' ', 1);
            # print(cpu_tmp,mem_tmp)
            cpu.append(float(cpu_tmp))
            mem.append(float(mem_tmp[:-1]))
        else:
            break
  len=cpu.__len__()
  # print(min(cpu),max(cpu))
  # print(cpu)
  x = np.arange(0,len)
  # y1= [30481,12583,51,9,2,2]
  # y2= [0.0065,0.016,0.039,0,0,0]
  
  fig,ax1 = plt.subplots()
  ax2 = ax1.twinx()           # 做镜像处理
  ax1.plot(x,cpu,'g-')
  ax2.plot(x,mem,'b--')
  title=filepath1.split('/',1)[1]
  save_img=title.split('.',1)[0]+'.png'
  ax1.set_title(title)
  ax1.set_xlabel('Time')    #设置x轴标题
  ax1.set_ylabel('cpu',color = 'g')   #设置Y1轴标题
  ax2.set_ylabel('mem',color = 'b')   #设置Y2轴标题 
  plt.savefig('./imgs/'+save_img)
  # plt.show()
# filename='1_sampling.txt'
def process_files(files):
  for filepath1 in files:
    process(filepath1)
files=['data/cpu/5282_sampling.txt','data/cpu/5399_sampling.txt','data/cpu/6179_sampling.txt','data/cpu/6439_sampling.txt','data/mem/6713_sampling.txt'
,'data/mem/6800_sampling.txt','data/mem/6919_sampling.txt','data/mem/7010_sampling.txt','data/mem/7102_sampling.txt']

process_files(files)

