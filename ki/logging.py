from enum import Enum


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
