from __future__ import print_function
import sys

import click

from twisted.internet.task import react
from twisted.internet.defer import inlineCallbacks, Deferred
from twisted.internet.error import ConnectionRefusedError
from twisted.internet.endpoints import clientFromString
from twisted.internet.endpoints import serverFromString
from twisted.internet.endpoints import connectProtocol
from twisted.python import log

from client import ChatClient, StandardInput, StdIOFactory
from server import ChatFactory

@inlineCallbacks
def server_main(reactor, endpoint):
    log.startLogging(sys.stdout)
    ep = serverFromString(reactor, bytes(endpoint))
    port = yield ep.listen(ChatFactory())
    print("listening: {}".format(port.getHost()))
    yield Deferred()  # wait forever


@inlineCallbacks
def client_main(reactor, endpoint, nick):
    ep = clientFromString(reactor, bytes(endpoint))
    done = Deferred()
    try:
        proto = yield connectProtocol(ep, ChatClient(done))
    except ConnectionRefusedError:
        print("Can't connect; is the server alive?")
        print("  tried", endpoint)
        return

    stdio = serverFromString(reactor, b'stdio:')
    port = yield stdio.listen(StdIOFactory(nick, proto))

    # yield proto.sendMessage(nick, "I have connected.")
    yield done  # callback()'d when protocol disconnects


@click.group()
def chat():
    pass


@chat.command()
#@click.option('--endpoint', '-e', default='tcp:thula.meejah.ca:10001', help='Twisted endpoint to connect to')
@click.option('--endpoint', '-e', default='tcp:127.0.0.1:10001', help='Twisted endpoint to connect to')
@click.option('--nick', '-n', default=None, help='Nickname to use', required=True)
def client(endpoint, nick):
    '''
    Start the chat client.
    '''
    return react(client_main, (endpoint, nick))


@chat.command()
@click.option('--endpoint', '-e', default='tcp:10001:interface=localhost', help='Twisted endpoint to listen on')
def server(endpoint):
    '''
    Start the chat server.
    '''
    return react(server_main, (endpoint,))
