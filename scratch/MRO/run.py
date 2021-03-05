from py_interface import *
from ctypes import *
import sys
import ns3_util
import time


class mlInput(Structure):
    _pack_ = 1
    _fields_ = [
        ('x', c_double),
        ('y', c_double)
    ]


class mlOutput(Structure):
    _pack_ = 1
    _fields_ = [
        ('tttAdjutment', c_double)
    ]


exp = Experiment(1234, 4096, 'MRO', '../../')

for i in range(1):
	exp.reset()
	r1 = Ns3AIRL(2333, mlInput, mlOutput)
	pro = exp.run(show_output=false)
	while not r1.isFinish():
		with r1 as data:
			if data == None:
				break
			data.mlOutput.tttAdjutment = data.mlInput.x + data.mlInput.y
			print([data.mlInput.x,data.mlInput.y,data.mlOutput.tttAdjutment])
	pro.wait()
del exp



