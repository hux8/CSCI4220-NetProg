#!/usr/bin/env python3

from concurrent import futures
import sys      # For sys.argv, sys.exit()
import socket   # For gethostname(), gethostbyname()
import math     # For

import grpc

import csci4220_hw3_pb2
import csci4220_hw3_pb2_grpc

'''
Provides methods that implement functionality of Kadmelia for CSCI-4220
'''


class KadImplServicer(csci4220_hw3_pb2_grpc.KadImplServicer):

    def __init__(self, id, port, addr, k):
        self.id = id
        self.port = port
        self.addr = addr
        self.k = k
        self.node = csci4220_hw3_pb2.Node(
            id=self.id, port=self.port, address=self.addr)
        '''
        Least Recently Used (LRU) List

                index 0 <--        list        --> index -1
            least recently used             most recently used
        '''
        # k_buckets have a fixed size of four
        self.k_buckets = [[], [], [], []]     # Store Node(id, port, address)
        self.key_value = []                     # Store tuple(key, value)

    '''
    See csci4220_hw3.proto
        rpc FindNode(IDKey) returns (NodeList) {}
    '''

    def FindNode(self, request, context):
        print("Serving FindNode({}) request for {}".format(
            request.idkey, request.node.id))

        k_closest_nodes = self.find_k_closest_nodes(request.idkey)

        # Update the k_buckets
        self.update_k_buckets(request.node, True)

        return csci4220_hw3_pb2.NodeList(responding_node=self.node, nodes=k_closest_nodes)

    '''
    See csci4220_hw3.proto
        rpc FindValue(IDKey) returns (KV_Node_Wrapper) {}
    '''

    def FindValue(self, request, context):
        print("Serving FindKey({}) request for {}".format(
            request.idkey, request.node.id))

        # See if the key/value pair has already been stored
        for pair in self.key_value:
            if pair[0] == request.idkey:
                # Update the k_buckets
                self.update_k_buckets(request.node, True)

                kv = csci4220_hw3_pb2.KeyValue(
                    node=self.node, key=request.idkey, value=pair[1])
                return csci4220_hw3_pb2.KV_Node_Wrapper(responding_node=self.node, mode_kv=True, kv=kv)

        k_closest_nodes = self.find_k_closest_nodes(request.idkey)

        # Update the k_buckets
        self.update_k_buckets(request.node, True)

        return csci4220_hw3_pb2.KV_Node_Wrapper(responding_node=self.node, mode_kv=False, nodes=k_closest_nodes)

    '''
    See csci4220_hw3.proto
        rpc Store(KeyValue) returns (IDKey) {}
    '''

    def Store(self, request, context):
        # Use the request input to store key and value inside our node
        self.key_value.insert(0, (request.key,request.value))
        print("Storing key {} value {}".format(request.key,request.value))
        self.update_k_buckets(request.node,1)
        # Return ID key for other peer's usage
        return csci4220_hw3_pb2.IDKey(node = self.node, idkey = self.id)

    '''
    See csci4220_hw3.proto
        rpc Quit(IDKey) returns (IDKey) {}
    '''

    def Quit(self, request, context):
        idk = int(request.idkey)
        for i in range(4):
            for entry in self.k_buckets[i]:
                if int(entry.id) == idk:
                    self.k_buckets[i].remove(entry)
                    print("Evicting quitting node " + str(idk) + " from bucket " + str(i))
                    return csci4220_hw3_pb2.IDKey(node = self.node, idkey = self.id)
        print("No record of quitting node " + str(idk) + " in k-buckets.")
        return csci4220_hw3_pb2.IDKey(node = self.node, idkey = self.id)

    # --------------------------------------------------------------------------
    '''
    Find the k closest nodes to dest in k-buckets
    '''

    def find_k_closest_nodes(self, dest):
        all_nodes = []
        for bucket in self.k_buckets:
            for node in bucket:
                all_nodes.append(node)
        all_nodes.sort(key=lambda node: node.id ^ dest)

        if len(all_nodes) <= self.k:
            return all_nodes
        else:
            return all_nodes[:self.k]

# 1 direct true, 2 indirect false

    def update_k_buckets(self, new_node, direct):
        # Check if the new node is itself
        if not (new_node.id == self.id):
            i = (self.id ^ new_node.id).bit_length()-1

            exist = False

            # See if the new node already exists
            if direct:
                for j in range(len(self.k_buckets[i])):
                    if self.k_buckets[i][j].id == new_node.id:
                        # Pop the node if it already exists
                        self.k_buckets[i].pop(j)
                        break
            else:
                exist = any(
                    node.id == new_node.id for node in self.k_buckets[i])

            if not exist:
                # Mack sure each bucket only store up to k nodes
                if len(self.k_buckets[i]) < self.k:
                    self.k_buckets[i].append(new_node)
                else:
                    self.k_buckets[i].pop(0)
                    self.k_buckets[i].append(new_node)

    '''
    Print the k-buckets
    '''

    def print_k_buckets(self):
        for i in range(0, 4):
            if len(self.k_buckets[i]) > 0:
                print("{}:".format(i), end=" ")
                bucket = " ".join(
                    f"{node.id}:{node.port}" for node in self.k_buckets[i])
                print(bucket)
            else:
                print("{}:".format(i))

# ==============================================================================


def run():
    '''
    Check the # of command-line arguments, which should have
        hw3.py <nodeID> <port> <k>
    '''
    if len(sys.argv) != 4:
        print("Error, correct usage is {} [my id] [my port] [k]".format(
            sys.argv[0]))
        sys.exit(-1)

    local_id = int(sys.argv[1])
    my_port = str(int(sys.argv[2])) # add_insecure_port() will want a string
    k = int(sys.argv[3])


    my_hostname = socket.gethostname() # Gets my host name
    my_address = socket.gethostbyname(my_hostname) # Gets my IP address from my hostname
    # print("[myself] Running as {0} @ {1}".format(my_hostname,my_address))
    # Setting up the server
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    servicer = KadImplServicer(local_id, int(my_port), my_address, k)
    csci4220_hw3_pb2_grpc.add_KadImplServicer_to_server(servicer, server)
    server.add_insecure_port("[::]:" + my_port)
    server.start()
    # Run the client
    while True:
        inputs = input()
        arguments = inputs.split()

        if arguments[0] == "BOOTSTRAP":
            remote_hostname = arguments[1]
            my_port = int(arguments[2])

            remote_addr = socket.gethostbyname(remote_hostname)

            # Connect to other peer
            channel = grpc.insecure_channel(str(remote_addr) + ':' + str(my_port))
            stub = csci4220_hw3_pb2_grpc.KadImplStub(channel)
            # Find peer node using servicer node
            nodee = stub.FindNode(csci4220_hw3_pb2.IDKey(node = servicer.node, idkey = local_id))

            # Add node that was responding
            res_node = nodee.responding_node
            bit_len = ((res_node.id)^local_id).bit_length()
            bit_len -= 1
            if len(servicer.k_buckets[bit_len]) == k:
                servicer.k_buckets[bit_len].popleft()
            servicer.k_buckets[bit_len].append(res_node)

            # Print bucket contents
            print('After BOOTSTRAP({}), k_buckets now look like:'.format(str(res_node.id)))
            servicer.print_k_buckets()
            channel.close()

        elif arguments[0] == "FIND_NODE":
            # Load the inputs argument
            node_id = int(arguments[1])

            print("Before FIND_NODE command, k-buckets are:")
            servicer.print_k_buckets()

            S = servicer.find_k_closest_nodes(node_id)
            S_no = S        # Nodes in S that have not been contacted yet
            S_yes = []      # Nodes that have been contacted

            found = False
            while S_no and (not found):
                for no in S_no:
                    # Start a channel
                    channel = grpc.insecure_channel(
                        no.address + ':' + str(no.port))
                    stub = csci4220_hw3_pb2_grpc.KadImplStub(channel)

                    # Send a FindNode RPC
                    R = stub.FindNode(csci4220_hw3_pb2.IDKey(
                        node=servicer.node, idkey=node_id))

                    # Update the k-buckets
                    servicer.update_k_buckets(no, True)
                    for node in R.nodes:
                        servicer.update_k_buckets(node, False)
                    # Update S_yes
                    S_yes.append(no)

                    # Close the channel
                    channel.close()

                # Check if the node has been found
                for bucket in servicer.k_buckets:
                    if any(node.id == node_id for node in bucket):
                        found = True
                        print("Found destination id {}".format(node_id))
                        break

                if not found:
                    # Update S and S_no
                    S = servicer.find_k_closest_nodes(node_id)
                    S_no.clear()
                    for node in S:
                        if not any(yes.id == node.id for yes in S_yes):
                            S_no.append(node)

            if not found:
                print("Could not find destination id {}".format(node_id))

            print("After FIND_NODE command, k-buckets are:")
            servicer.print_k_buckets()

        elif arguments[0] == "FIND_VALUE":
            # Load the inputs argument
            key = int(arguments[1])

            print("Before FIND_VALUE command, k-buckets are:")
            servicer.print_k_buckets()

            found = False
            # See if the key/value pair has already been stored
            for pair in servicer.key_value:
                if pair[0] == key:
                    found = True
                    print("Found data \"{}\" for key {}".format(pair[1], key))
                    break

            if not found:
                S = servicer.find_k_closest_nodes(key)
                S_no = S        # Nodes in S that have not been contacted yet
                S_yes = []      # Nodes that have been contacted

                while S_no and (not found):
                    for no in S_no:
                        # Start a channel
                        channel = grpc.insecure_channel(
                            no.address + ':' + str(no.port))
                        stub = csci4220_hw3_pb2_grpc.KadImplStub(channel)

                        # Send a FindNode RPC
                        R = stub.FindValue(csci4220_hw3_pb2.IDKey(
                            node=servicer.node, idkey=key))

                        # Update the k-buckets
                        servicer.update_k_buckets(no, True)
                        # Update S_yes
                        S_yes.append(no)

                        # Close the channel
                        channel.close()

                        # Check if the value is found
                        if R.mode_kv:
                            found = True
                            print("Found value \"{}\" for key {}".format(
                                R.kv.value, key))
                            break
                        else:
                            # Update the k-buckets
                            for node in R.nodes:
                                servicer.update_k_buckets(node, False)

                    if not found:
                        # Update S and S_no
                        S = servicer.find_k_closest_nodes(key)
                        S_no.clear()
                        for node in S:
                            if not any(yes.id == node.id for yes in S_yes):
                                S_no.append(node)

                if not found:
                    print("Could not find key {}".format(key))

            print("After FIND_VALUE command, k-buckets are:")
            servicer.print_k_buckets()

        elif arguments[0] == "STORE":

            this_key = int(arguments[1])
            this_value = arguments[2]

            closest_node = csci4220_hw3_pb2.Node(id = local_id, port = int(my_port), address = str(my_address))
            distance = abs(local_id - this_key)
            for bucket in servicer.k_buckets:
                for entry in bucket:
                    if abs(int(entry.id) - this_key) < distance:
                        closest_node = entry
                        distance = abs(int(entry.id) - this_key)

            closest_port = int(closest_node.port)

            #Connect to server & stub
            this_addr = closest_node.address + ':' + str(closest_port)
            channel = grpc.insecure_channel(this_addr)
            stub = csci4220_hw3_pb2_grpc.KadImplStub(channel)

            some_idkey = stub.Store(csci4220_hw3_pb2.KeyValue(node = None, key = this_key, value = this_value))
            print("Storing key {} at node {}".format(this_key,some_idkey.idkey))
            channel.close()

        elif arguments[0] == "QUIT":
            # Tell everyone we are quitting, goodbye :(
            for bucket in servicer.k_buckets:
                for entry in bucket:
                    remote_host = str(entry.id)
                    remote_port = int(entry.port)

                    my_addr = entry.address + ':' + str(remote_port)

                    # Create channel and tell other peer we are quitting
                    channel = grpc.insecure_channel(my_addr)
                    stub = csci4220_hw3_pb2_grpc.KadImplStub(channel)

                    print("Letting " + remote_host + " know I'm quitting.")
                    some_idkey = stub.Quit(csci4220_hw3_pb2.IDKey(node = None, idkey = local_id))
                    channel.close()
            print("Shut down node " + str(local_id))
            break

        else:
            # Unknown command
            print("Unknown command. Use BOOTSTRAP , FIND_VALUE , STORE , QUIT")


if __name__ == '__main__':
    run()