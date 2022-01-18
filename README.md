# CS347-Raft-Consensus-Algorithm
CS347 Coursework - Implementation of the Raft Consensus Algorithm

## Research Paper
https://raft.github.io/raft.pdf

## Other Papers
https://lamport.azurewebsites.net/pubs/paxos-simple.pdf

## Useful Visualisation
http://thesecretlivesofdata.com/raft/

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
