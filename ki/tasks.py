import asyncio
import weakref
from enum import Enum, auto


class TaskSignal(Enum):
    DONE = auto()
    CONTINUE = auto()
    CONTINUE_DELAYED = auto()


class Task(object):
    """A representation of a task that is bound to a TaskParticipant
    instance.

    A task will invoke its tick behavior every frame until it is
    finished. Upon finishing, it will invoke its cleanup behavior.
    """
    running_tasks = {}

    def __init__(self, participant, tick_func, cleanup_func=None):
        self._participant = weakref.ref(participant)
        self._tick_func = tick_func
        self._cleanup_func = cleanup_func

        self.name = '%s-%d' % (tick_func.__name__, id(participant))

    def __call__(self, *args, **kwargs):
        # Invoke our tick behavior.
        return self._tick_func(self._participant(), *args, **kwargs)

    @property
    def asyncio_task(self):
        """Returns the underlying `asyncio.Task` object.

        If the task isn't running, `None` will be returned instead.
        """
        return Task.running_tasks.get(self.name)

    @property
    def running(self):
        """Returns whether or not this task is currently running."""
        return self.asyncio_task is not None

    def start(self, delay=None, args=None):
        """Starts the task."""
        if not self.running:
            Task.running_tasks[self.name] = \
                asyncio.ensure_future(self._tick(delay=delay, args=args))

    def stop(self):
        """Stops the task."""
        if self.running:
            asyncio_task = Task.running_tasks.pop(self.name)
            asyncio_task.cancel()

    async def _tick(self, delay=None, args=None):
        if args is None:
            args = ()

        try:
            # Wait out the delay before our first tick.
            if delay is not None:
                await asyncio.sleep(delay)

            while self.running:
                # Invoke our tick behavior.
                signal = self._tick_func(self._participant(), *args)

                if signal == TaskSignal.CONTINUE:
                    # Continue to the next execution frame.
                    continue
                elif signal == TaskSignal.CONTINUE_DELAYED:
                    # Wait out the delay before our next execution.
                    if delay is not None:
                        await asyncio.sleep(delay)
                else:
                    # Assume they'd like to end the task.
                    break
        except asyncio.CancelledError:
            pass
        finally:
            # Invoke our cleanup behavior.
            if self._cleanup_func is not None:
                self._cleanup_func(self._participant())

            # Invoke our internal cleanup behavior (if it hasn't already been).
            self.stop()


class TaskDecorator(object):
    """A decorator that can be used to define a new task, with the
    decorated method as its tick behavior.

    If you wish to go further and define its cleanup behavior, use
    `TaskDecorator.cleanup`.
    """

    def __init__(self, tick_func, cleanup_func=None):
        self._tick_func = tick_func
        self._cleanup_func = cleanup_func

    def __get__(self, obj, objtype=None):
        # Someone invoked `obj.attr`. Assuming that `obj` is an
        # instance of `TaskParticipant`, return a new `Task` instance.
        # This `Task` instance will be bound to `obj` for its
        # lifetime.
        if isinstance(obj, TaskParticipant):
            return Task(obj, self._tick_func, cleanup_func=self._cleanup_func)
        return self

    def tick(self, func):
        """A decorator that can be used to define this task's tick behavior."""
        # Return a copy of this `TaskDecorator` instance, with the
        # decorated function as the `tick_func`.
        return type(self)(func, cleanup_func=self._cleanup_func)

    def cleanup(self, func):
        """A decorator that can be used to define this task's cleanup behavior."""
        # Return a copy of this `TaskDecorator` instance, with the
        # decorated function as the `cleanup_func`.
        return type(self)(self._tick_func, cleanup_func=func)


asyncio_task = TaskDecorator  # Alias


class TaskParticipant(object):
    """The base for any class that wishes to house a managed asyncio task."""

    def iter_tasks(self):
        """A generator that can be used to iterate over all of the
        tasks that belong to this instance.
        """
        for name in dir(self):
            attr = getattr(self, name)
            if isinstance(attr, Task):
                yield attr

    def stop_tasks(self):
        """Stops all running tasks."""
        for task in self.iter_tasks():
            task.stop()
