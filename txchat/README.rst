txchat
======

Demo `Twisted`_ chatroom server for `PolyglotAB`_ asynchronous
programming talk.

It is very simple:

 1. there is one "room"
 2. all messages are binary, encoded with `msgpack`_:
    list-of-strings: ['nick', 'message']
 3. all incoming messages are broadcast to all clients.


Install and Run
---------------

For details, see the ``Makefile``.
To get started, create and activate a virtualenv::

   make venv
   source venv/bin/activate

Run all the tests, see the coverage::

   make test
   sensible-browser htmlcov/index.html

Run the server::

   txchat server

Run a client::

   txchat client --nick=name



.. _Twisted: https://twistedmatrix.com/
.. _msgpack: http://msgpack.org/
.. _PolyglotAB: http://ab.polyglotconf.com/
