# CS347-Raft-Consensus-Algorithm
CS347 Coursework - Implementation of the Raft Consensus Algorithm

## Research Paper
https://raft.github.io/raft.pdf

## Running instructions (Linux is easiest because g++)
### Terminal 1
```python3 udp_server.py```

### Terminal 2
```make -B```

```./manager```

## C++ Implementation Details

This section details some of the main inner workings of the C++ code to make it easier to understand.

### Server
- The server class is essentially a sample implementation of a server that this algorithm is designed to run on. 
- It uses a thread to run the server_function in the class, which receives messages from other servers, and also sends details to the python client (at the moment).
- The server has its own socket that it uses to receive messages, and also has the addresses of the other servers in the neighbours array, which is used to send messages between the servers.
- Server ID is used to determine the recipient server (stored alongside the address stuff in neighbour struct).

### Manager
- This enables communication with the Python client, and also manages initialisation of the servers.
- Has both send and receive sockets to communicate with the Python client.
- The send function is utilised by the servers to communicate with the client, this is synchronised between the threads with a mutex lock.
- Port numbers for various functionality defined in the header, as well as the number of servers (limit of 6 at the moment).

### Database
- This is the database that is instantiated on the servers, at the moment, it is an array of 5 integers. This can be expanded later.

## Python Interface Details
- Has send and receive sockets, like the manager class in the C++ implementation, to communicate with the C++ code.
- Has a thread that listens for packets, and the main thread runs the tkinter interface and sends input commands.
- The Input section takes an server ID, index and value, which can now update the databases of the servers (over sockets).

## ESP Network Plan
<img src="https://user-images.githubusercontent.com/47477832/149416954-9d44d517-6ec0-499b-bb36-600a9a042169.png" width="400">

## Interface
<img src="https://user-images.githubusercontent.com/47477832/149643044-8e76b5f6-930f-4371-8615-4483c66ef537.png" width="600">

- Added a thread that only collects packets sent from the data collector node.
- UI allows the issue of commands to modify the database on the server node, clients unaffected at the moment as the algorithm needs to be implemented.
- Limited details, more will likely be added as algorithm is implemented.
- Only server, client 1 and client 2 nodes set up.

### Notes
- Will replicate a list of integers over all ESP's that are identical to that of the server using the raft algorithm.
- Server node is interacted with by providing an index and a new value, this modifies the element and will need to be replicated on all nodes.
- The state collector is essentially a converter from ESP-NOW to UDP as both cannot function at the same time on the ESP.
- The state collector will probably request information from each node (log changes, time of changes, current database state, etc) as this will allow minimisation of clashes of ESP-NOW transfers.
- The receiver/stats sender will beam the states back to the interface over UDP, we will have to develop the program to properly visualise the output (simple interface atm).
- The server is fixed, and it will act as the provider of the info (green circle in visualisation).
- In a simple config with all nodes connected, one of the ESP nodes will be a leader and the others will be followers.
- Each ESP will be loaded with the created raft library.
- ESP-NOW used between the ESP's (changes, heartbeats, etc), there is a hard-limit of 250 byte transfers, so will need to keep communications light.
