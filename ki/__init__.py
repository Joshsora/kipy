from . import lib
from . import dml
from . import pclass
from . import protocol
from . import serialization
from . import util


def set_type_system(type_system):
    """Set the global TypeSystem instance.

    This will be used when defining new property classes/enums, and
    reading serialized files.
    """
    pclass.PropertyClassMeta.type_system = type_system
    pclass.EnumMeta.type_system = type_system
    serialization.SerializedFile.type_system = type_system
