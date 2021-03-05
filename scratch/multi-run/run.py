from py_interface import *
from ctypes import *
import sys
import ns3_util
import time

class Env(Structure):
    _pack_ = 1
    _fields_ = [
        ('a', c_int),
        ('b', c_int)
    ]


class Act(Structure):
    _pack_ = 1
    _fields_ = [
        ('c', c_int)
    ]
print('*')
exp = Experiment(1234, 4096, 'multi-run', '../../')
print('*')
for i in range(2):
    print('!')
    exp.reset()
    print('!')
    rl = Ns3AIRL(2333, Env, Act)
    print('!')
    pro = exp.run(show_output=True)
    print('!')
    while not rl.isFinish():
        print('@')
        with rl as data:
            print('#')
            if data == None:
                break
            print('%')
            data.act.c = data.env.a+data.env.b
            print(data.env.a,data.env.b,data.act.c)
    pro.wait()
del exp
