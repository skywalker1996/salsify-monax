#! /usr/bin/env python
import asyncio
import websockets
import json
from subprocess import Popen
import socket
import subprocess


STOP = 0
RUNNING = 1
RESIDUAL = 2

STATES = {0:"stop", 1:"running", 2:"residual"}

state = STOP

start_flag = False

TRACE_BASE = './traces/Belgium_4GLTE'


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
			res = {}
			res['cmd'] = 'initialize'
			res['result'] = 'success'
			res['traces'] = traces
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
				p = Popen(f"sh ./run.sh {commands['method']}",stdout=subprocess.PIPE, shell=True)
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
				p = Popen(f"sh ./run_mahimahi.sh {commands['method']} {commands['trace']}",stdout=subprocess.PIPE, shell=True)
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
		
		else:
			res = {}
			res['cmd'] = 'error'
			res['result'] = 'command error'

			await websocket.send(json.dumps(res))
	

start_server = websockets.serve(agent, "192.168.0.164", 9002)
print('VStreamTEST Agent running!')
asyncio.get_event_loop().run_until_complete(start_server)
asyncio.get_event_loop().run_forever()