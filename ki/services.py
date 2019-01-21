import asyncio
import logging
import weakref

from .lib.protocol.dml import MessageManager
from .tasks import TaskParticipant


class MessageHandler(object):
    """A representation of a message handler that is bound to a
    `Service` instance.

    A message handler should be invoked by passing (sender, message).
    """

    def __init__(self, service, name, func):
        self._service = weakref.ref(service)
        self.name = name
        self._func = func

    def __call__(self, sender, message):
        return self._func(self._service(), sender, message)


class MessageHandlerDecorator(object):
    """A decorator used to define a new message handler for a `Service`
    class.
    """

    def __init__(self, name):
        self.name = name
        self._func = None

    def __call__(self, func):
        self._func = func
        return self

    def __get__(self, obj, objtype=None):
        # Someone invoked `obj.attr`. Assuming that `obj` is an
        # instance of `Service`, return a new `MessageHandler` instance.
        # This `MessageHandler` instance will be bound to `obj` for its
        # lifetime.
        if isinstance(obj, Service):
            return MessageHandler(obj, self.name, self._func)
        return self


msghandler = MessageHandlerDecorator  # Alias


class Service(TaskParticipant):
    """The base for any class that wishes to house a message handler."""
    logger = logging.getLogger('SERVICE')

    def __init__(self, participant):
        self.participant = participant
        self.message_mgr = participant.message_mgr

        # Register our message handlers.
        for message_handler in self.iter_message_handlers():
            participant.register_message_handler(message_handler)

    def iter_message_handlers(self):
        """A generator that can be used to iterate over all of the
        message handlers that belong to this instance.
        """
        for name in dir(self):
            attr = getattr(self, name)
            if isinstance(attr, MessageHandler):
                yield attr


class ServiceParticipant(object):
    """The base for any class that wishes to house a service, and
    invoke its message handlers.
    """
    logger = logging.getLogger('SERVICE-PARTICIPANT')

    def __init__(self):
        self.message_mgr = MessageManager()
        self.message_handlers = {}

    def register_message_handler(self, message_handler):
        """Registers the given message handler as a managed one."""
        self.message_handlers[message_handler.name] = message_handler

    def handle_message(self, sender, message):
        """Invokes the correct message handler for the given message."""
        self.logger.debug('handle_message(%r, %r)', sender, message.handler)

        message_handler = self.message_handlers.get(message.handler)
        if message_handler is None:
            self.logger.warning("sender=%r, No handler found: '%s'",
                                sender, message.handler)
            return

        asyncio.ensure_future(
            message_handler(sender, message)
        )
