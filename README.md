# CS347-Raft-Consensus-Algorithm
CS347 Coursework - Implementation of the Raft Consensus Algorithm

## Research Paper
https://raft.github.io/raft.pdf

## Useful Visualisation
Interactive, good intro - http://thesecretlivesofdata.com/raft/
General, useful reference - https://raft.github.io/

## Running Instructions - Node.js + React
### Prerequisites
- Node.js v16.14.0 (Might work on earlier versions of Node.js) with corepack enabled
- g++ with support for C++11
- Linux, native or on WSL2 (Mac OS X uses a different Clang implementation)

Note: These steps have been tested to run on the VNC provided by Warwick DCS.

### Steps
1. Navigate to the `Interface` directory.
2. Run `npm install` to install all dependencies of the project, then run `npm run start` to start the Node.js server. **Note:** If you want to have the Python testing interface running at the same time, add the `--no-manager` argument (`npm run start -- --no-manager`).
3. In another terminal, navigate to the `raft-web-interface` directory, then perform `npm install` and `npm run start`.
4. Your browser should open `http://localhost:3000` automatically.

Note for users of WSL2:
1. Install `net-tools` if it is not present. Then, get the host IP of the WSL2 subsystem with `ifconfig`.
2. Add a file called `.env` inside the `raft-web-interface` folder with the following content: ```REACT_APP_WEBSOCKET_URL=ws://<IP from ifconfig>:8001```
3. Restart the `raft-web-interface` server

## Running Instructions - Python

### Prerequisites
- Python 3.9 or later
- g++ with support for C++11
- Linux, native or on WSL2 (Mac OS X uses a different Clang implementation)
- For WSL2 Python users: Xming

Note: It is easiest to get the project up and running with a Linux machine.

### Steps
1. Navigate to the `Interface` directory.
2. For WSL2 users: Start XLaunch with `Multiple windows` support > `Start no client` > Check the `No Access Control` option.
3. Run ```python3 raft_interface.py```.
4. Wait for build to complete; a window with the Raft algorithm interface should show up.

## Resources
**In Search of an Understandable Consensus Algorithm (Extended Version)** https://raft.github.io/raft.pdf (The Main Paper)

**The Implementation of Raft by the Paper Authors** https://github.com/logcabin/logcabin (specifically https://github.com/logcabin/logcabin/blob/master/Server/RaftConsensus.cc)

**Paxos Made Simple** https://lamport.azurewebsites.net/pubs/paxos-simple.pdf

**Planning for Change in a Formal Verification of the Raft Consensus Protocol** https://dl.acm.org/doi/pdf/10.1145/2854065.2854081
\[ We present the first formal verification of state machine safety for the Raft consensus protocol, a critical component of many distributed systems. \]

**Raft Refloated: Do We Have Consensus?** https://dl.acm.org/doi/pdf/10.1145/2723872.2723876
\[ In this study, we repeat the Raft authors’ performance analysis. We developed a clean-slate implementation of the Raft protocol and built an event-driven simulation framework for prototyping it on experimental topologies. We propose several optimizations to the Raft protocol and demonstrate their effectiveness under contention. Finally, we empirically validate the correctness of the Raft protocol invariants and evaluate Raft’s understandability claims. \]

**Reducing the Energy Footprint of a Distributed Consensus Algorithm** https://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=7371967

**Tangaroa: a Byzantine Fault Tolerant Raft** https://www.scs.stanford.edu/14au-cs244b/labs/projects/copeland_zhong.pdf

**Paxos vs Raft: Have we reached consensus on distributed consensus?** https://dl.acm.org/doi/pdf/10.1145/3380787.3393681

**Flotsam: Evaluating Implementations of the Raft Consensus Algorithm** https://connorgilbert.com/papers/flotsam.pdf \[ This work presents a system, Flotsam, for empirically testing Raft open-source implementations for errors. Randomized test cases are generated and tested in a virtualized environment based on Docker. Test operations are specified using a general interface to allow new implementations to be simply “plugged in”, and outputs are checked against each other using configurable criteria after injecting. \]

## C++ Implementation Details

This section details some of the main inner workings of the C++ code to make it easier to understand.

### JSON Library

Inter-process communication between Raft nodes uses the JSON format. C++ JSON library used is [nlohmann/json](https://github.com/nlohmann/json).

### Server
- The server class is essentially a sample implementation of a server that this algorithm is designed to run on. 
- It uses a thread to run the server_function in the class, which receives messages from other servers, and also sends details to the testing/web interface.
- The server has its own socket that it uses to receive messages, and also has the addresses of the other servers in the neighbours array, which is used to send messages between the servers.
- Server ID is used to determine the recipient server (stored alongside the address properties in neighbour struct).

### Manager
- This enables communication with the testing/web interfaces, and also manages initialisation of the servers.
- Has both send and receive sockets to communicate with the interfaces.
- The send function is utilised by the servers to communicate with the client.
- Port numbers for various functionality defined in the header, as well as the number of servers (limit of 5).

### Database
- This is the database that is instantiated on the servers, at the moment, it is an array of 5 integers. This is open to future expansion.
