#! /usr/bin/env python
import asyncio
import websockets
import json
from subprocess import Popen
import socket
import subprocess
import os
import numpy as np

def log_analyze(log_file):
	
	RTT_log = []
	frame_oneway_delay_log = []
	throughput_log = []
	
	with open(log_file) as f:
		line = f.readline()
		while(line):
			title = line.split(':')[0][1:-1]
	
			if(title=="RealRTT"):
				RTT_log.append(int(line.split(':')[1]))
			elif(title=="frame_one_way_delay"):
				frame_oneway_delay_log.append(int(line.split(':')[1]))
			elif(title=="throughput"):
				throughput_log.append(float(line.split(':')[1]))
				
			line = f.readline()
			
		result = {}
		result["RTT_average"] = round(np.mean(RTT_log), 2)
		result["frame_delay_average"] = round(np.mean(frame_oneway_delay_log), 2)
		result["throughput_average"] = round(np.mean(throughput_log), 2)

		return result


results = log_analyze('./log/sender.log')
print(results)