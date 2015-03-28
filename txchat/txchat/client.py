from __future__ import print_function
import os

from twisted.internet.task import react
from twisted.internet.defer import Deferred, inlineCallbacks

from twisted.internet.protocol import Factory
from twisted.internet.protocol import Protocol
from twisted.internet.endpoints import TCP4ClientEndpoint, connectProtocol
from twisted.protocols.basic import LineReceiver

import msgpack


class ChatClient(Protocol):
    def __init__(self, done):
        self.done = done
        self.unpacker = msgpack.Unpacker()

    def connectionLost(self, reason):
        print(reason.getErrorMessage())
        self.done.callback(reason)

    def sendMessage(self, nick, msg):
        print("sending", nick, msg)
        data = msgpack.packb([nick, msg])
        self.transport.write(data)

    def dataReceived(self, data):
        # ditto to server: go over what about "burst" messages?
        # (and do "original" code here at first: msg = msgpack.unpack(data)
        self.unpacker.feed(data)
        for msg in self.unpacker:
            print("{}: {}".format(*msg))


class StdIOFactory(Factory):
    def __init__(self, nick, proto):
        self.nick = nick
        self.proto = proto

    def buildProtocol(self, addr):
        return StandardInput(self.nick, self.proto)


from twisted.internet.stdio import StandardIO
class StandardInput(LineReceiver, StandardIO):
    '''
    Reads stdin and writes every line received as a message to the
    server. No fancy editing or anything, simple pipe.
    '''
    delimiter = os.linesep

    def lineReceived(self, line):
        return self.protocol.sendMessage(self.nick, line)

    def __init__(self, nick, proto):
        self.nick = nick
        self.protocol = proto

    def connectionLost(self, reason):
        self.protocol.transport.loseConnection()
