# CS347-Raft-Consensus-Algorithm
CS347 Coursework - Implementation of the Raft Consensus Algorithm

## Research Paper
https://raft.github.io/raft.pdf

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
