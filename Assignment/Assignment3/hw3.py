#!/usr/bin/env python3

from concurrent import futures
import sys  # For sys.argv, sys.exit()
import socket  # for gethostbyname()

import grpc

import csci4220_hw3_pb2
import csci4220_hw3_pb2_grpc

class KadImplServicer(csci4220_hw3_pb2_grpc.KadImplServicer):

	def __init__(self, id, port, addr, k):
		self.id = id
		self.port = port
		self.addr = addr
		self.k = k
		self.node = csci4220_hw3_pb2.Node(id=self.id, port=self.port, address=self.addr)
		

		self.k_buckets = [ [], [], [], [] ]
		self.key_value = []
		
	# TODO functions for each command


def run():
	if len(sys.argv) != 4:
		print("Error, correct usage is {} [my id] [my port] [k]".format(sys.argv[0]))
		sys.exit(-1)

	local_id = int(sys.argv[1])
	my_port = str(int(sys.argv[2])) # add_insecure_port() will want a string
	k = int(sys.argv[3])
	my_hostname = socket.gethostname() # Gets my host name
	my_address = socket.gethostbyname(my_hostname) # Gets my IP address from my hostname

	# Setting up the server
	server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
	servicer = KadImplServicer(local_id, int(my_port), my_address, k)
	csci4220_hw3_pb2_grpc.add_KadImplServicer_to_server(servicer, server)
	server.add_insecure_port("[::]:" + my_port)
	server.start()

	''' Use the following code to convert a hostname to an IP and start a channel
	Note that every stub needs a channel attached to it
	When you are done with a channel you should call .close() on the channel.
	Submitty may kill your program if you have too many file descriptors open
	at the same time. '''
	
	#remote_addr = socket.gethostbyname(remote_addr_string)
	#remote_port = int(remote_port_string)
	#channel = grpc.insecure_channel(remote_addr + ':' + str(remote_port))

	# Start reading in client inputs
	while True:
		# Reading and formating the inputs
		inputs = input()
		arguments = inputs.split()

		# The first argument is the command
		command = arguments[0]


		# BOOTSTRAP: <remote hostname> <remote port>
		if command == "BOOTSTRAP":

			remote_hostname = arguments[1]
			remote_port = arguments[2]
			# Get the remote IP address
			remote_addr = socket.gethostbyname(remote_hostname)
			print(command, arguments)
			
			# TODO: BOOTSTRAP function

		# FIND_NODE <nodeID>
		elif command == "FIND_NODE":

			node_id = int(arguments[1])

			# TODO: FIND_NODE function
			print(command, arguments)

		# FIND_VALUE <key>
		elif command == "FIND_VALUE":

			key = int(arguments[1])

			# TODO: FIND_VALUE function
			print(command, arguments)

		# STORE <key> <value>
		elif command == "STORE":

			key = int(arguments[1])
			value = arguments[2]

			# TODO: STORE function
			print(command, arguments)

		# QUIT
		elif command == "QUIT":
			
			# TODO: QUIT function
			print(command)
			break


if __name__ == '__main__':
	run()
