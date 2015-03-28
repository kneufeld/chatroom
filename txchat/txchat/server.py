from __future__ import print_function

from twisted.internet.task import react
from twisted.internet.endpoints import TCP4ServerEndpoint
from twisted.internet.error import ConnectionDone
from twisted.internet.defer import Deferred, inlineCallbacks
from twisted.python import log

from twisted.internet.protocol import Protocol, Factory

import msgpack

# global state. normally this would go in your factory instance, or
# similar, and you'd pass that to your protocol
clients = set()


# FIXUP, changed classname to match c++
class ChatSession(Protocol):
    def __init__(self):
        self.decoder = msgpack.Unpacker()  # dependency-inject me instead, please
        self.sent = 0
        self.recv = 0

    def gotMessage(self, msg):
        self.recv += 1
        packed = msgpack.packb(msg)
        global clients  # better ways, e.g. Factory
        # log.msg("Broadcasting {} bytes to {} clients.".format(len(packed), len(clients)))
        for proto in clients:
            if proto is not self:
                proto.transport.write(packed)
                self.sent += 1

    def dataReceived(self, data):
        # log.msg("Got {} bytes from {}.".format(len(data), self.transport.getPeer()))
        self.decoder.feed(data)
        for msg in self.decoder:
            self.gotMessage(msg)

    def connectionMade(self):
        # print("messages r/w:", self.recv, self.sent)
        global clients
        clients.add(self)

    def connectionLost(self, reason):
        # print("messages r/w:", self.recv, self.sent)
        global clients
        clients.remove(self)
        reason.trap(ConnectionDone)
        print("Sad times at ridgemont high", reason)


class ChatFactory(Factory):
    def buildProtocol(self, addr):
        return ChatSession()
