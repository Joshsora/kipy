import asyncio
import logging
import time
from enum import IntEnum

from .protocol.dml import MessageManager
from .protocol.net import SessionCloseErrorCode, \
    ServerSession as CServerSession, ClientSession as CClientSession, \
    ServerDMLSession as CServerDMLSession, ClientDMLSession as CClientDMLSession
from .tasks import TaskParticipant, TaskSignal, asyncio_task
from .util import IDAllocator, AllocationError


def msghandler(name):
    """A decorator for binding message handlers to a function."""
    def wrapper(f):
        DMLSessionBase.handlers[name] = f
        return f
    return wrapper


class AccessLevel(IntEnum):
    NEW = 0
    ESTABLISHED = 1
    AUTHENTICATED = 2


class SessionBase(TaskParticipant):
    logger = logging.getLogger('SESSION')

    ENSURE_ALIVE_INTERVAL = 10.0
    KEEP_ALIVE_INTERVAL = 10.0

    def __init__(self, transport):
        self.transport = transport

        self._close_handlers = []

        self._ensure_alive.start(delay=self.ENSURE_ALIVE_INTERVAL)

    @asyncio_task
    def _ensure_alive(self):
        """Ensures that this session has been receiving keep alive packets.

        If none have been received within the allowed time frame, then the
        on_timeout() event will be triggered.
        """
        if not self.alive:
            self.on_timeout()
        return TaskSignal.AGAIN

    @asyncio_task
    def _keep_alive(self):
        """Sends a keep alive packet periodically.

        This should be overridden by subclasses to perform the correct
        behavior.
        """
        return TaskSignal.DONE

    def on_invalid_packet(self):
        """"Overrides `Session.on_invalid_packet()`."""
        self.logger.warning('id=%d, Got an invalid packet!', self.id)
        self.close(SessionCloseErrorCode.INVALID_MESSAGE)

    def send_packet_data(self, data, size):
        """"Overrides `Session.send_packet_data()`."""
        if self.transport is not None:
            self.logger.debug('id=%d, send_packet_data(%r, %d)', self.id, data, size)
            self.transport.write(data)

    def close(self, error):
        """"Overrides `Session.close()`."""
        # close() might be called more than once.
        # For this reason, we only take action if our connection is alive.
        if self.transport is None:
            return

        self.logger.debug('id=%d, close(%r)', self.id, error)

        # Stop all of our managed asyncio tasks.
        self.stop_tasks()

        # Invoke our close handlers.
        for close_handler in self._close_handlers:
            close_handler()

        self._close_handlers = []

        # Close the connection.
        self.transport.close()
        self.transport = None

    def on_established(self):
        """"Overrides `Session.on_established()`."""
        self.logger.debug('id=%d, on_established()', self.id)

        # Set our access level to ESTABLISHED.
        self.access_level = AccessLevel.ESTABLISHED

        # Start sending keep alive packets.
        self._keep_alive.start(delay=self.KEEP_ALIVE_INTERVAL)

    def on_timeout(self):
        """Called when this session is discovered to no longer be alive.

        The session should typically be killed here.
        """
        self.logger.debug('id=%d, Session timed out!', self.id)
        self.close(SessionCloseErrorCode.SESSION_DIED)

    def add_close_handler(self, func):
        """Adds the given function to this session's close handlers.

        A close handler is called immediately before the session's connection
        is closed.
        """
        if func not in self._close_handlers:
            self._close_handlers.append(func)

    def remove_close_handler(self, func):
        """Removes the given function from this session's close handlers.

        A close handler is called immediately before the session's connection
        is closed.
        """
        if func in self._close_handlers:
            self._close_handlers.remove(func)


class DMLSessionBase(SessionBase):
    handlers = {}

    def on_message(self, message):
        """"Overrides `DMLSession.on_message()`."""
        self.logger.debug('id=%d, on_message(%r)', self.id, message.handler)

        handler_func = DMLSessionBase.handlers.get(message.handler)
        if handler_func is not None:
            handler_func(self, message)
        else:
            self.logger.warning("id=%d, No handler found: '%s'", self.id, message.handler)

    def on_invalid_message(self, error):
        """"Overrides `DMLSession.on_invalid_message()`."""
        self.logger.warning('id=%d, Got an invalid message! (%r)', self.id, error)
        self.close(SessionCloseErrorCode.INVALID_MESSAGE)


class ProtocolBase(asyncio.Protocol):
    logger = logging.getLogger('PROTOCOL')

    def __init__(self):
        self.session = None

    def connection_made(self, transport):
        """"Overrides `Protocol.connection_made()`."""
        peername = transport.get_extra_info('peername')
        self.logger.debug('Connection made: %r', peername)

    def data_received(self, data):
        """"Overrides `Protocol.data_received()`.

        Passes the data off to the session for processing.
        """
        if self.session is not None:
            size = len(data)
            self.logger.debug('process_data(%r, %d)', data, size)
            self.session.process_data(data, size)

    def connection_lost(self, exc):
        """"Overrides `Protocol.connection_lost()`."""
        self.logger.debug('Connection lost: %r', exc)


class ServerSessionBase(SessionBase):
    KEEP_ALIVE_INTERVAL = 60.0

    def __init__(self, server, transport):
        SessionBase.__init__(self, transport)

        self.server = server

    @asyncio_task
    def _keep_alive(self):
        """Sends a keep alive packet periodically."""
        self.send_keep_alive(self.server.startup_time_delta)
        return TaskSignal.AGAIN

    def close(self, error):
        """"Overrides `SessionBase.close()`."""
        SessionBase.close(self, error)

        self.server.on_session_closed(self)


class ServerSession(ServerSessionBase, CServerSession):
    def __init__(self, server, transport, id):
        ServerSessionBase.__init__(self, server, transport)
        CServerSession.__init__(self, id)


class ServerDMLSession(DMLSessionBase, ServerSessionBase, CServerDMLSession):
    def __init__(self, server, transport, id, manager):
        DMLSessionBase.__init__(self, transport)
        ServerSessionBase.__init__(self, server, transport)
        CServerDMLSession.__init__(self, id, manager)


class ServerProtocol(ProtocolBase):
    def __init__(self, server):
        super().__init__()

        self.server = server

    def connection_made(self, transport):
        """"Overrides `ProtocolBase.connection_made()`.

        Creates a new session.
        """
        super().connection_made(transport)

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

    def connection_lost(self, exc):
        """"Overrides `ProtocolBase.connection_lost()`.

        Kills the session.
        """
        super().connection_lost(exc)

        if self.session is not None:
            # Free up the allocated ID.
            self.server.session_id_allocator.free(self.session.id)

            # Kill the session.
            self.session.close(SessionCloseErrorCode.SESSION_DIED)
            self.session = None

        self.server = None


class Server(object):
    logger = logging.getLogger('SERVER')

    PROTOCOL_CLS = ServerProtocol
    SESSION_CLS = ServerSession

    MIN_SESSION_ID = 1
    MAX_SESSION_ID = 0xFFFF

    def __init__(self, port):
        self.port = port

        self.startup_timestamp = time.time()

        self.session_id_allocator = IDAllocator(
            self.MIN_SESSION_ID, self.MAX_SESSION_ID)
        self.sessions = {}

    @property
    def startup_time_delta(self):
        """Returns the time that has elapsed since startup.

        This value will be in milliseconds.
        """
        return int((time.time() - self.startup_timestamp) * 1000.0)

    def run(self, event_loop):
        """Starts listening for incoming connections."""
        protocol_factory = lambda: self.PROTOCOL_CLS(self)
        coro = event_loop.create_server(protocol_factory, port=self.port)
        event_loop.run_until_complete(coro)

    def close(self):
        """Closes all sessions."""
        for session in self.sessions.copy().values():
            session.close(SessionCloseErrorCode.SESSION_DIED)

    def on_session_closed(self, session):
        """Removes the given session from our managed sessions."""
        if session.id in self.sessions:
            del self.sessions[session.id]

    def create_session(self, transport):
        """Returns a new session."""
        session_id = self.session_id_allocator.allocate()
        session = self.SESSION_CLS(self, transport, session_id)
        self.sessions[session.id] = session
        return session


class DMLServer(Server):
    SESSION_CLS = ServerDMLSession

    def __init__(self, port):
        super().__init__(port)

        self.message_manager = MessageManager()

    def create_session(self, transport):
        """Returns a new session."""
        session_id = self.session_id_allocator.allocate()
        session = self.SESSION_CLS(self, transport, session_id, self.message_manager)
        return session


class ClientSessionBase(SessionBase):
    KEEP_ALIVE_INTERVAL = 10.0

    def __init__(self, client, transport):
        SessionBase.__init__(self, transport)

        self.client = client
        self.client.session = self

    @asyncio_task
    def _keep_alive(self):
        """Sends a keep alive packet periodically."""
        self.send_keep_alive()
        return TaskSignal.AGAIN

    def close(self, error):
        """"Overrides `SessionBase.close()`."""
        SessionBase.close(self, error)

        self.client.session = None


class ClientSession(ClientSessionBase, CClientSession):
    def __init__(self, client, transport, id):
        ClientSessionBase.__init__(self, client, transport)
        CClientSession.__init__(self, id)


class ClientDMLSession(DMLSessionBase, ClientSessionBase, CClientDMLSession):
    def __init__(self, client, transport, id, manager):
        DMLSessionBase.__init__(self, transport)
        ClientSessionBase.__init__(self, client, transport)
        CClientDMLSession.__init__(self, id, manager)


class ClientProtocol(ProtocolBase):
    def __init__(self, client):
        super().__init__()

        self.client = client

    def connection_made(self, transport):
        """"Overrides `ProtocolBase.connection_made()`.

        Creates a new session.
        """
        super().connection_made(transport)

        self.session = self.client.create_session(transport)
        self.session.on_connected()

    def connection_lost(self, exc):
        """"Overrides `ProtocolBase.connection_lost()`.

        Kills the session.
        """
        super().connection_lost(exc)

        if self.session is not None:
            # Kill the session.
            self.session.close(SessionCloseErrorCode.SESSION_DIED)
            self.session = None

        self.client = None


class Client(object):
    logger = logging.getLogger('CLIENT')

    PROTOCOL_CLS = ClientProtocol
    SESSION_CLS = ClientSession

    def __init__(self, host, port):
        self.host = host
        self.port = port

        self.session = None

    def run(self, event_loop):
        """Tries to connect to the server."""
        protocol_factory = lambda: self.PROTOCOL_CLS(self)
        coro = event_loop.create_connection(
            protocol_factory, host=self.host, port=self.port)
        event_loop.run_until_complete(coro)

    def close(self):
        """Closes the session."""
        self.session.close(SessionCloseErrorCode.SESSION_DIED)

    def create_session(self, transport):
        """Returns a new session."""
        return self.SESSION_CLS(self, transport, 0)


class DMLClient(Client):
    SESSION_CLS = ClientDMLSession

    def __init__(self, port):
        super().__init__(port)

        self.message_manager = MessageManager()

    def create_session(self, transport):
        """Returns a new session."""
        return self.SESSION_CLS(self, transport, 0, self.message_manager)
