from . import lib

# Lift symbols from .lib.pclass
from .lib.pclass import HashCalculator, WizardHashCalculator, TypeSystem,\
    StaticProperty, VectorProperty

__all__ = [
    'HashCalculator', 'WizardHashCalculator', 'TypeSystem', 'StaticProperty',
    'VectorProperty', 'PropertyClassMeta', 'PropertyClass'
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

        if cls.TYPE_NAME is None:
            cls.TYPE_NAME = 'class ' + name

        # Define a new ClassType.
        # TODO: Add support for the base_class argument.
        mcs.type_system.define_class(cls.TYPE_NAME, cls, None)

        return cls


class PropertyClass(lib.pclass.PropertyClass, metaclass=PropertyClassMeta):
    TYPE_NAME = None

    def __init__(self, type, type_system):
        super().__init__(type, type_system)

        for name in self.__class__.__dict__:
            attr = getattr(self.__class__, name)
            if isinstance(attr, lib.pclass.PropertyDef):
                attr.instantiate(self)
