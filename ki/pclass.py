import copy

from . import lib
from .lib.pclass import IHashCalculator, WizardHashCalculator, TypeSystem, \
    PropertyFlags, StaticProperty, VectorProperty
from .lib.serialization import BinarySerializer, BinarySerializerFlags, \
    JsonSerializer, XmlSerializer
from .lib.util import BitBuffer, BitStream

__all__ = [
    'IHashCalculator', 'WizardHashCalculator', 'TypeSystem', 'PropertyFlags',
    'StaticProperty', 'VectorProperty', 'PropertyClassMeta', 'PropertyClass',
    'EnumMeta', 'Enum', 'EnumElement'
]


class PropertyClassMeta(type):
    type_system = None

    def __new__(mcs, name, bases, dct):
        cls = super().__new__(mcs, name, bases, dct)

        if name == 'PropertyClass':
            # This is just us defining the Python-specific
            # PropertyClass below.
            return cls

        if mcs.type_system is None:
            raise RuntimeError('PropertyClassMeta.type_system must be defined '
                                'before defining a PropertyClass.')

        if cls.TYPE_NAME is None or mcs.type_system.has_type(cls.TYPE_NAME):
            cls.TYPE_NAME = 'class ' + name

        cls._property_defs = cls._property_defs.copy()

        for attr in dct.values():
            if isinstance(attr, (StaticProperty, VectorProperty)):
                cls._property_defs.append(attr)

        # Define a new ClassType.
        # TODO: Add support for the base_class argument.
        mcs.type_system.define_class(cls.TYPE_NAME, cls, None)

        # Pointer Aliases
        mcs.type_system.define_class('%s*' % cls.TYPE_NAME, cls, None)
        mcs.type_system.define_class('class SharedPointer<%s>' % cls.TYPE_NAME, cls, None)

        return cls

class PropertyClass(lib.pclass.PropertyClass, metaclass=PropertyClassMeta):
    TYPE_NAME = None

    _property_defs = []

    def __init__(self, type, type_system):
        super().__init__(type, type_system)

        for property_def in self._property_defs:
            property_def.instantiate(self)

    def __deepcopy__(self, memodict={}):
        # Instantiate a new copy.
        instance = self._type_system.instantiate(self.TYPE_NAME)

        # Assign its property values.
        for name, attr in self.__class__.__dict__.items():
            if isinstance(attr, (StaticProperty, VectorProperty)):
                value = copy.deepcopy(getattr(self, name))
                setattr(instance, name, value)

        return instance

    def serialize_binary(self, flags=BinarySerializerFlags.NONE):
        buffer = BitBuffer()
        stream = BitStream(buffer)
        serializer = BinarySerializer(self._type_system, False, flags)
        serializer.save(self, stream)
        return buffer.data[:stream.tell().as_bytes()]

    def serialize_json(self):
        serializer = JsonSerializer(self._type_system, False)
        return serializer.save(self)

    def serialize_xml(self):
        serializer = XmlSerializer(self._type_system)
        return serializer.save(self)


class EnumMeta(type):
    type_system = None

    def __new__(mcs, name, bases, dct):
        cls = super().__new__(mcs, name, bases, dct)

        if name == 'Enum':
            # This is just us defining the Python-specific
            # Enum below.
            return cls

        if mcs.type_system is None:
            raise RuntimeError('EnumMeta.type_system must be defined '
                                'before defining an Enum.')

        if cls.TYPE_NAME is None or mcs.type_system.has_type(cls.TYPE_NAME):
            cls.TYPE_NAME = 'enum ' + name

        # Define a new EnumType.
        enum_type = mcs.type_system.define_enum(cls.TYPE_NAME)

        # Convert our EnumElement attributes to `lib.pclass.Enum`.
        for name, attr in dct.items():
            if isinstance(attr, EnumElement):
                enum_type.add_element(attr.name, attr.value)
                setattr(cls, name, lib.pclass.Enum(enum_type, attr.value))

        return cls


class Enum(metaclass=EnumMeta):
    TYPE_NAME = None


class EnumElement(object):
    def __init__(self, name, value):
        self.name = name
        self.value = value
