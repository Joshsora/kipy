from enum import Enum


class AllocationError(Exception):
    pass


class IDAllocator(object):
    def __init__(self, min_id, max_id):
        self.next_id = min_id
        self.max_id = max_id
        self.unused_ids = set()

    def allocate(self):
        """"Returns an unused ID.

        If there are no more IDs left in the pool, an AllocationError
        will be thrown.
        """
        if self.unused_ids:
            return self.unused_ids.pop()
        if self.next_id > self.max_id:
            raise AllocationError('ID allocation limit reached -- %d' % self.max_id)
        allocated_id = self.next_id
        self.next_id += 1
        return allocated_id

    def free(self, id):
        """Allows the given ID to be allocated again."""
        self.unused_ids.add(id)


class ErrorCodeEnum(Enum):
    def __repr__(self):
        return '%s.%s' % (self.__class__.__name__, self.name)


def format_error_message(code=None, reason=None, recommendation=None):
    """Format an error message for logging."""
    message = ''

    if code is not None:
        message += 'code=%r' % code
        if reason is not None:
            message += ', '
        elif recommendation is not None:
            message += '\n'

    if reason is not None:
        message += reason
        if recommendation is not None:
            message += '\n'

    if recommendation is not None:
        message += 'Recommendation:\n\t%s\n' % recommendation

    return message
