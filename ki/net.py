import asyncio
import logging

from .protocol.dml import MessageManager
from .protocol.net import ServerSession as CServerSession, \
    ClientSession as CClientSession, \
    ServerDMLSession as CServerDMLSession, \
    ClientDMLSession as CClientDMLSession, \
    SessionCloseErrorCode
from .util import IDAllocator, AllocationError


class SessionBase(object):
    logger = logging.getLogger('SESSION')

    def __init__(self, transport):
        self.transport = transport

    def send_packet_data(self, data, size):
        self.logger.debug('id=%d, send_packet_data(%r, %d)' %
                          (self.id, data, size))

        if self.transport is not None:
            self.transport.write(data)

    def close(self, error):
        self.logger.debug('id=%d, close(%r)' % (self.id, error))

        if self.transport is not None:
            self.transport.close()
            self.transport = None

    def on_established(self):
        self.logger.debug('id=%d, on_established()' % self.id)

        self.access_level = 1

    def on_invalid_packet(self):
        self.logger.warning('id=%d, Got an invalid packet!' % self.id)


class ServerSession(SessionBase, CServerSession):
    def __init__(self, transport, id):
        SessionBase.__init__(self, transport)
        CServerSession.__init__(self, id)


class ClientSession(SessionBase, CClientSession):
    def __init__(self, transport, id):
        SessionBase.__init__(self, transport)
        CClientSession.__init__(self, id)


class DMLSessionBase(SessionBase):
    handlers = {}

    def on_message(self, message):
        self.logger.debug('id=%d, on_message(%r)' % (self.id, message.handler))

        handler_func = self.handlers.get(message.handler)
        if handler_func is not None:
            handler_func(self, message)
        else:
            self.logger.warning("id=%d, No handler found: '%s'" % (self.id, message.handler))
            # FIXME: self.close(SessionCloseErrorCode.UNHANDLED_APPLICATION_MESSAGE)

    def on_invalid_message(self, error):
        self.logger.warning('id=%d, Got an invalid message! (%r)' % (self.id, error))


class ServerDMLSession(DMLSessionBase, CServerDMLSession):
    def __init__(self, transport, id, manager):
        DMLSessionBase.__init__(self, transport)
        CServerDMLSession.__init__(self, id, manager)


class ClientDMLSession(DMLSessionBase, CClientDMLSession):
    def __init__(self, transport, id, manager):
        DMLSessionBase.__init__(self, transport)
        CClientDMLSession.__init__(self, id, manager)


def msghandler(name):
    def wrapper(f):
        DMLSessionBase.handlers[name] = f
        return f
    return wrapper


class Protocol(asyncio.Protocol):
    logger = logging.getLogger('PROTOCOL')

    def __init__(self, server):
        self.server = server
        self.session = None

    def connection_made(self, transport):
        peername = transport.get_extra_info('peername')
        self.logger.debug('Connection made: %r, %d' % peername)

        try:
            self.session = self.server.create_session(transport)
        except AllocationError:
            # An ID could not be allocated for a new session; refuse
            # connection.
            self.logger.warning('Failed to allocate an ID for a new session!')
            self.logger.warning('Refusing connection.')
            transport.close()
        else:
            self.session.on_connected()

    def data_received(self, data):
        self.logger.debug('data_received(%r, %d)' % (data, len(data)))

        if self.session is not None:
            self.session.process_data(data, len(data))

    def connection_lost(self, exc):
        self.logger.debug('Connection lost: %r' % exc)

        if self.session is not None:
            # Free up the allocated ID.
            self.server.session_id_allocator.free(self.session.id)

            # Destroy the session.
            self.session.close(SessionCloseErrorCode.SESSION_DIED)
            self.session = None

        self.server = None


class Server(object):
    PROTOCOL_CLS = Protocol
    SESSION_CLS = ServerSession

    MIN_SESSION_ID = 1
    MAX_SESSION_ID = 0xFFFF

    logger = logging.getLogger('SERVER')

    def __init__(self, port, use_uvloop=False):
        self.port = port

        if use_uvloop:
            import uvloop
            self.event_loop = uvloop.new_event_loop()
        else:
            self.event_loop = asyncio.new_event_loop()
            self.logger.warning('uvloop has not been configured!')
            self.logger.warning('It is recommended that uvloop is enabled in '
                                'a production environment.')

        self.session_id_allocator = IDAllocator(
            self.MIN_SESSION_ID, self.MAX_SESSION_ID)
        self.sessions = {}

    def run(self):
        protocol_factory = lambda: self.PROTOCOL_CLS(self)
        coro = self.event_loop.create_server(protocol_factory, port=self.port)
        server = self.event_loop.run_until_complete(coro)
        try:
            self.event_loop.run_forever()
        finally:
            server.close()
            self.event_loop.run_until_complete(server.wait_closed())
            self.event_loop.close()

    def create_session(self, transport):
        session_id = self.session_id_allocator.allocate()
        session = self.SESSION_CLS(transport, session_id)
        self.sessions[session_id] = session
        return session


class DMLServer(Server):
    SESSION_CLS = ServerDMLSession

    def __init__(self, port, use_uvloop=False):
        super().__init__(port, use_uvloop=use_uvloop)

        self.message_manager = MessageManager()

    def create_session(self, transport):
        session_id = self.session_id_allocator.allocate()
        session = self.SESSION_CLS(transport, session_id, self.message_manager)
        self.sessions[session_id] = session
        return session
