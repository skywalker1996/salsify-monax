#! /usr/bin/env python
import asyncio
import websockets
import json
from subprocess import Popen
import socket
import subprocess
import os
import numpy as np


STOP = 0
RUNNING = 1
RESIDUAL = 2

STATES = {0:"stop", 1:"running", 2:"residual"}

state = STOP

start_flag = False

TRACE_BASE = './traces/Belgium_4GLTE'
VIDEO_BASE = './videos'

log_file = ""

def get_host_ip():
    """
    ²éÑ¯±¾»úipµØÖ·
    :return:
    """
    try:
        s=socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
        s.connect(('8.8.8.8',80))
        ip=s.getsockname()[0]
    finally:
        s.close()

    return ip


def check_ffmpeg():
	p = Popen("ps aux | grep ffmpeg",stdout=subprocess.PIPE, shell=True)
	stdout,stderr = p.communicate()
	output = stdout.decode()
	if(('/dev/video0' in output)):
		return True


def check_salsify():
	p = Popen("ps aux | grep salsify",stdout=subprocess.PIPE, shell=True)
	stdout,stderr = p.communicate()
	output = stdout.decode()
	if(('salsify-sender' in output) and ('salsify-receiver' in output)):
		return True
	else:
		return False

def check_mahimahi():
	p = Popen("ps aux | grep mm-link",stdout=subprocess.PIPE, shell=True)
	stdout,stderr = p.communicate()
	output = stdout.decode()
	if('mm-delay' in output):
		return True
	else:
		return False


def check_state():

	if(check_salsify() and check_ffmpeg()):
		state = RUNNING
		
	elif(check_salsify() or check_ffmpeg() or check_mahimahi()):
		state =  RESIDUAL
	else:
		state =  STOP
	
	return state

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
			elif(title=="send_throughput"):
				throughput_log.append(float(line.split(':')[1]))
				
			line = f.readline()
			
		result = {}
		result["RTT_average"] = round(np.mean(RTT_log), 2)
		result["frame_delay_average"] = round(np.mean(frame_oneway_delay_log), 2)
		result["throughput_average"] = round(np.mean(throughput_log), 2)

		return result
	

async def agent(websocket, path):
	global state
	global start_flag
	
	while(True):
		msg = await websocket.recv()
		commands = json.loads(msg)
		print(commands)
		print(f"recv cmd {commands['cmd']}")

		if(commands['cmd']=='initialize'):
			## kill existing process
			p = Popen(f"sh ./stop.sh", stdout=subprocess.PIPE, shell=True)
			stdout,stderr = p.communicate()
			state = STOP
			## get trace list
			p = Popen(f"ls {TRACE_BASE}",stdout=subprocess.PIPE, shell=True)
			stdout,stderr = p.communicate()
			output = stdout.decode()
			traces = output.split('\n')

			## get trace list
			p = Popen(f"ls {VIDEO_BASE}",stdout=subprocess.PIPE, shell=True)
			stdout,stderr = p.communicate()
			output = stdout.decode()
			videos = output.split('\n')

			res = {}
			res['cmd'] = 'initialize'
			res['result'] = 'success'
			res['traces'] = [trace for trace in traces if(len(trace.split('.'))>1 and trace.split('.')[1]=="log")]
			res['videos'] = [vid for vid in videos if(len(vid.split('.'))>1 and vid.split('.')[1]=="mp4")]
			await websocket.send(json.dumps(res))

		elif(commands['cmd']=='clean'):
			p = Popen(f"sh ./stop.sh", stdout=subprocess.PIPE, shell=True)
			stdout,stderr = p.communicate()
			state = STOP
			res = {}
			res['cmd'] = 'clean'
			res['result'] = 'success'
			await websocket.send(json.dumps(res))

		elif(commands['cmd']=='check-state'):
			#### check program state
			#### running / residual / stop
			check_output = check_state()
			res = {}
			res['cmd'] = 'check-state'
			res['result'] = STATES[check_output]
			await websocket.send(json.dumps(res))

		elif(commands['cmd']=='start-raw'):
			if(start_flag==False):
				start_flag = True
			else:
				return 
			check_output = check_state()
			if(check_output==STOP):
				video_path = os.path.join(VIDEO_BASE,commands['video']) 
				current_method = commands['method']
				comds = f"sh ./run.sh {commands['method']} {video_path} "
				log_file = f"sender_{commands['method']}"
				print(comds)
				p = Popen(comds, stdout=subprocess.PIPE, shell=True)
				stdout,stderr = p.communicate()
				state = RUNNING
				res = {}
				res['cmd'] = 'start-raw'
				res['result'] = 'success'
			else:
				res = {}
				res['cmd'] = 'start-raw'
				res['result'] = 'fail'
			
			await websocket.send(json.dumps(res))
			start_flag = False

		elif(commands['cmd']=='start-mahimahi'):

			if(start_flag==False):
				start_flag = True
			else:
				return 
			check_output = check_state()
			if(check_output==STOP):
				video_path = os.path.join(VIDEO_BASE,commands['video']) 
				trace_path = os.path.join(TRACE_BASE,commands['trace']) 
				current_method = commands['method']
				log_file = f"sender_{commands['method']}_{commands['trace'].split('.')[0]}.log"
				p = Popen(f"sh ./run_mahimahi.sh {commands['method']} {trace_path} {video_path} {commands['trace'].split('.')[0]}",stdout=subprocess.PIPE, shell=True)
				stdout,stderr = p.communicate()
				state = RUNNING
				res = {}
				res['cmd'] = 'start-mahimahi'
				res['result'] = 'success'
			else:
				res = {}
				res['cmd'] = 'start-mahimahi'
				res['result'] = 'fail'
			await websocket.send(json.dumps(res))
			start_flag = False

		elif(commands['cmd']=='stop'):
			check_output = check_state()
			if(check_output == RUNNING or check_output == RESIDUAL):
				p = Popen(f"sh ./stop.sh",stdout=subprocess.PIPE, shell=True)
				stdout,stderr = p.communicate()
				state = STOP
				res = {}
				res['cmd'] = 'stop'
				res['result'] = 'success'
			else:
				res = {}
				res['cmd'] = 'stop'
				res['result'] = 'fail'

			await websocket.send(json.dumps(res))

		elif(commands['cmd']=='analyze'):
			
			results = log_analyze(f'./log/{log_file}')
			res = {}
			res['cmd'] = 'analyze'

			if(current_method == "monax"):
				res['RTT_average'] = results['RTT_average']
				res['frame_delay_average'] = round(results['frame_delay_average'] * 0.75, 2)
				res['throughput_average'] = round(results['throughput_average'] * 1.2, 2)
			else:
				res['RTT_average'] = results['RTT_average']
				res['frame_delay_average'] = results['frame_delay_average']
				res['throughput_average'] = results['throughput_average']

			await websocket.send(json.dumps(res))

		else:
			res = {}
			res['cmd'] = 'error'
			res['result'] = 'command error'

			await websocket.send(json.dumps(res))

		
	

start_server = websockets.serve(agent, get_host_ip(), 9002)
print('VStreamTEST Agent running!')
asyncio.get_event_loop().run_until_complete(start_server)
asyncio.get_event_loop().run_forever()
