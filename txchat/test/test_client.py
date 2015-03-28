import pytest
import msgpack
import mock

from twisted.test import proto_helpers
from twisted.internet.address import IPv4Address
from twisted.internet.defer import Deferred
from twisted.python.failure import Failure

from txchat.client import ChatClient, StandardInput, StdIOFactory


@pytest.fixture(scope='function')
def all_done():
    return Deferred()


@pytest.fixture(scope='function')
def proto(all_done):
    p = ChatClient(all_done)
    t = proto_helpers.StringTransportWithDisconnection()
    p.makeConnection(t)
    p.transport.protocol = p
    return p


def test_send_message(proto):
    proto.sendMessage('emma', 'People have only as much liberty as they '
                      'have the intelligence to want and the courage to take')

    sent = msgpack.unpackb(proto.transport.value())
    assert sent[0] == 'emma'
    assert sent[1][:6] == 'People'


def test_receive_message(proto):
    # this one's a little weird, since we want to make sure we
    # print()'d something out -- would be easier to give the
    # ChatClient a .file to .write() on but this shows how to use
    # mock.patch as well to monkey-patch a standard-lib thing

    method_calls = None
    with mock.patch('sys.stdout') as fakeout:
        proto.dataReceived(msgpack.packb(['m.g.', 'That State is the best governed which is governed the least']))
        method_calls = fakeout.method_calls

    # we could successfully use print() again now...
    assert len(fakeout.method_calls) >= 1
    data = ''.join(map(lambda x: ''.join(x[1]), fakeout.method_calls))
    assert data.startswith('m.g.: ')

def test_stdin(proto):
    factory = StdIOFactory('hst', proto)
    si = factory.buildProtocol(None)#StandardInput('hst', proto)
    test_message = 'There are times, however, and this is one of them, when even being right feels wrong.'

    si.lineReceived(test_message)

    sent = msgpack.unpackb(proto.transport.value())
    assert sent[0] == 'hst'
    assert sent[1] == test_message

def test_disconnect(all_done, proto):
    si = StandardInput('', proto)

    si.connectionLost(Failure(RuntimeError("the horror")))

    assert all_done.called
