import pytest
import msgpack

from twisted.test import proto_helpers
from twisted.internet.address import IPv4Address

from txchat.server import ChatFactory


@pytest.fixture(scope='function')
def factory():
    return ChatFactory()


@pytest.fixture(scope='function')
def proto(factory):
    p = factory.buildProtocol(IPv4Address('TCP', '127.0.0.9', 65534))
    transport = proto_helpers.StringTransportWithDisconnection()
    # XXX normally done by Factory?
    transport.protocol = p
    p.makeConnection(transport)
    return p


def test_one_message(proto):
    # set up
    test_message = ['carmack', 'Programming is not a zero-sum game.']
    proto.transport = proto_helpers.StringTransport()
    data = msgpack.packb(test_message)

    # send our test-message over the protocol
    proto.dataReceived(data)

    # server just echos message to all connected clients
    sent_data = proto.transport.value()
    assert sent_data != ''
    assert msgpack.unpackb(sent_data) == test_message


def test_two_messages_at_once(proto):
    # set up
    test_message0 = ['ZoP', 'Beautiful is better than ugly.']
    test_message1 = ['ZoP', 'Explicit is better than implicit.']
    data = msgpack.packb(test_message0) + msgpack.packb(test_message1)

    # send our test-messages over the protocol, all at once
    proto.dataReceived(data)

    # server just echos message to all connected clients
    unpacker = msgpack.Unpacker()
    unpacker.feed(proto.transport.value())
    messages = list(unpacker)
    assert len(messages) == 2
    assert messages[0] == test_message0
    assert messages[1] == test_message1


def test_one_message_two_clients(factory):
    # set up
    proto0 = proto(factory)
    proto1 = proto(factory)
    test_message = ['hst', 'Some may never live, but the crazy never die.']
    data = msgpack.packb(test_message)

    # send the test-message from one client
    proto0.dataReceived(data)

    # should have received it in BOTH clients
    assert proto0.transport.value() == proto1.transport.value()
    assert msgpack.unpackb(proto0.transport.value()) == test_message
    assert msgpack.unpackb(proto1.transport.value()) == test_message


def test_message_trickle(proto):
    test_message = ['albert', 'The only source of knowledge is experience.']
    data = msgpack.packb(test_message)

    # send our message in two chunks
    proto.dataReceived(data[:5])
    assert proto.transport.value() == ''
    proto.dataReceived(data[5:])

    # server just echos message to all connected clients
    sent_data = proto.transport.value()
    assert sent_data != ''
    assert msgpack.unpackb(sent_data) == test_message


def test_client_disconnect(factory, proto):
    from txchat import server
    assert proto in server.clients
    proto.transport.loseConnection()
    assert proto not in server.clients
