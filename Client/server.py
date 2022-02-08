from threading import Thread, Event
from queue import Queue
import time
import random

# Server instance
class Server(Thread):
    # Initialise the thread controlling the server
    def __init__(self, server_name):
        Thread.__init__(self)
        self.thread = Thread(target=self.run, args=(self.name, ))
        self.thread.daemon = True # Thread ends when main process ends
        self.name = server_name
        self.queue = Queue() # Queue for accepting messages from other threads
        self.neighbours = [] # Neighbour servers to communicate with

        # PULL DETAILS FROM THE DATA LIBRARY (temp for now)
        self.state = round(random.random())
        self.term = round(random.random())
        self.voted_for = 0

    # Temporary get details function
    def getDetails(self):
        return [self.name, self.state, self.term, self.voted_for]

    # Start the server thread
    def start(self):
        self.thread.start() # Start the thread

    # Add a neighbour to the list of neighbour servers
    def addNeighbour(self, neighbour):
        self.neighbours.append(neighbour)

    # Send 'message' to server with index 'serverIndex' in neighbours
    def sendMessage(self, serverIndex, message):
        self.neighbours[int(serverIndex)].append(message)

    # Function to run that recieves messages
    def run(self, name):
        while(True):
            for i in range(len(self.neighbours)):
                self.sendMessage(i, ["hello server", i, "from server", self.name])

                message = self.queue.get()

                print(message)

                # Would pass to library here and get reply

                # Would then send reply to server that sent message in first place
            time.sleep(0.25)

    # Append a message to the servers queue
    def append(self, message):
        self.queue.put(message)
