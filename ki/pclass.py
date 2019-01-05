from . import lib

# Lift symbols from .lib.pclass...
from .lib.pclass import IHashCalculator, WizardHashCalculator, TypeSystem, \
    StaticProperty, VectorProperty, ArrayProperty

__all__ = [
    'IHashCalculator', 'WizardHashCalculator', 'TypeSystem', 'StaticProperty',
    'VectorProperty', 'ArrayProperty', 'PropertyClassMeta', 'PropertyClass'
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
            if isinstance(attr, lib.pclass.PropertyDef):
                cls._property_defs.append(attr)

        # Define a new ClassType.
        # TODO: Add support for the base_class argument.
        mcs.type_system.define_class(cls.TYPE_NAME, cls, None)

        return cls


class PropertyClass(lib.pclass.PropertyClass, metaclass=PropertyClassMeta):
    _property_defs = []

    TYPE_NAME = None

    def __init__(self, type, type_system):
        super().__init__(type, type_system)

        for property_def in self._property_defs:
            property_def.instantiate(self)
