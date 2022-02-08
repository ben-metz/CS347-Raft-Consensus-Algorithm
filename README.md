# CS347-Raft-Consensus-Algorithm
CS347 Coursework - Implementation of the Raft Consensus Algorithm

## Research Paper
https://raft.github.io/raft.pdf

## Running instructions (Linux is easiest because g++)
### Terminal 1 (Python Client Directory)
```python3 udp_server.py```

### Terminal 2 (C++ Implementation Directory)
```make -B```

```./manager```

## C++ Implementation Details

### JSON Library

https://github.com/open-source-parsers/jsoncpp

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

## Resources
**In Search of an Understandable Consensus Algorithm (Extended Version)** https://raft.github.io/raft.pdf (The Main Paper)

**The Implementation of Raft by the Paper Authors** https://github.com/logcabin/logcabin (specifically https://github.com/logcabin/logcabin/blob/master/Server/RaftConsensus.cc)

**Paxos Made Simple** https://lamport.azurewebsites.net/pubs/paxos-simple.pdf

## Useful Visualisation
Interactive, good intro - http://thesecretlivesofdata.com/raft/
General, useful reference - https://raft.github.io/

## Outline

### Layout
<img src="https://user-images.githubusercontent.com/47477832/150686980-559bae7e-cc7c-45a5-9470-64b399fd342a.png" width="400">

### Raft Library (C++)
- Takes in messages from other instances of the libraries and responds appropriately according to the algorithm
- Also interfaces with the data library, so messages should be directly forwarded into the data library

### Data Library (C++)
- Receives modifications from the raft library and acts upon these messages
- Probably going to be a list of numbers that is updated (as it is simple), but can always be extended to more complicated data

### Example
- User inputs a change into the client
- The client sends the data modification to the elected leader
- The elected leader uses the raft library to compose messages to send to other servers (could be a list with a message for each neighbouring server), this is returned
- The returned messages are then sent, this is external to the library as it is implementation specific
- When a server receives the message into its raft library, it extracts the modification message (if present) and forwards this to the data library to process the change, e.g. if ‘set 1 to 34’ is received, index 1 is set to 34 (just an example probably going to be different).

## Notes
- A node can be in 1 of 3 states: Follower, Candidate, Leader
- All nodes start in the follower state
- If followers do not hear from a leader then they become a candidate
- The candidate then requests votes from other nodes
- Nodes will reply with their vote
- The candidate becomes the leader if it gets votes from a majority of nodes
- This process is called Leader Election
- All changes to the system now go through the leader

- Each change is added as an entry in the node's log
- To commit an entry, the node first replicates it to the follower nodes
- It then waits until a majority of nodes have written the entry
- The entry is then committed to the leader node
- Once committed, the followers are notified that the leader has committed
- A consensus about the system state has been reached
- This is called Log Replication

Leader Election
- In raft, there are 2 timeout settings which control elections
- First is the election timeout
- The election timeout is the amount of time a follower waits until becoming a candidate
- It is randomised to be between 150ms and 300ms
- After the election timeout the follower becomes a candidate and starts a new election term, votes for itself, and sends out Request Vote messages to other nodes
- If the receiving node hasn't voted yet in this term then it votes for the candidate, and the node resets its election timeout
- Once a candidate has a majority of votes it becomes the leader
- The leader begins sending out Append Entries messages to its followers
- These messages are sent in intervals specified by the heartbeat timeout
- Followers then respond to each Append Entries message.
- The election term will continue until a follower stops receiving heartbeats and becomes a candidate.
- Requiring a majority of votes guarantees that only one leader can be elected per term.
- If two nodes become candidates at the same time then a split vote can occur, if this occurs, then there is a reelection as the split candidates cannot get majority (see visualisation), this happens until there is not a split decision

Log Replication:
- Once we have a leader elected we need to replicate all changes to our system to all nodes.
- This is done by using the Append Entries message that was used for heartbeats
- First a client sends a change to the leader
- The change is appended to the leader's log
- Then the change is sent to the followers on the next heartbeat
- An entry is committed once a majority of followers acknowledge it
- And a response is sent to the client

- Raft can even stay consistent in the face of network partitions
- If a partition is added that divides the network into 2, then we are left with 2 leaders in different terms (1 for each group)
- After the split, it is possible that the old leader doesn't have the majority anymore, so log entries remain uncommitted, the other group without the leader would reelect (as no heartbeat from old leader), and would be updated as the majority lies in this region
- If the partition is healed, the old leader may see that there is a higher election term and will step down, any uncommited entries at this point are rolled back
- (See visualisation for a better understanding)
