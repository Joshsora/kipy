from . import lib

hash_calculator = lib.pclass.WizardHashCalculator()
type_system = lib.pclass.TypeSystem(hash_calculator)


class ClassType(lib.pclass.Type):
    def __init__(self, name, cls, type_system):
        super().__init__(name, type_system)
        self.cls = cls

        assert issubclass(self.cls, PropertyClass), \
            'The given class must inherit PropertyClass!'

        self.kind = lib.pclass.TypeKind.CLASS

    def instantiate(self):
        return self.cls(self, self.type_system)

    def write_to(self, stream, value):
        for prop in value.properties:
            prop.write_value_to(stream)

    def read_from(self, stream):
        obj = self.instantiate()
        for prop in obj.properties:
            prop.read_value_from(stream)
        return obj


class PropertyClassMeta(type):
    _type_refs = []

    def __new__(mcs, name, bases, dct):
        cls = super().__new__(mcs, name, bases, dct)

        if name == 'PropertyClass':
            # This is just us defining the Python-specific
            # PropertyClass below.
            return cls

        if cls.TYPE_NAME is None:
            cls.TYPE_NAME = 'class ' + name

        # Define a new ClassType.
        cls_type = ClassType(cls.TYPE_NAME, cls, type_system)
        type_system.define_type(cls_type)

        # We also need to store a reference to it on our end so that
        # the C++ TypeSystem will return it properly.
        mcs._type_refs.append(cls_type)

        return cls


class PropertyClass(lib.pclass.PropertyClass, metaclass=PropertyClassMeta):
    TYPE_NAME = None

    def __init__(self, type, type_system):
        super().__init__(type, type_system)

        self._property_refs = []

        for name in self.__class__.__dict__:
            attr = getattr(self.__class__, name)
            if isinstance(attr, PropertyDef):
                self._property_refs.append(attr(self))


class PropertyDef(object):
    def __init__(self, name, type_name, is_pointer=False):
        self.name = name
        self.type_name = type_name
        self.is_pointer = is_pointer

    def __call__(self, obj):
        raise NotImplementedError

    def __get__(self, obj, objtype=None):
        if isinstance(obj, PropertyClass):
            prop = obj.properties.get_property(self.name)
            return prop.value
        return self

    def __set__(self, obj, value):
        if isinstance(obj, PropertyClass):
            prop = obj.properties.get_property(self.name)
            prop.value = value


class StaticProperty(PropertyDef):
    def __call__(self, obj):
        type = type_system.get_type(self.type_name)
        return _StaticProperty(obj, self.name, type, is_pointer=self.is_pointer)


class VectorProperty(PropertyDef):
    def __call__(self, obj):
        type = type_system.get_type(self.type_name)
        return _VectorProperty(obj, self.name, type, is_pointer=self.is_pointer)


class _StaticProperty(lib.pclass.PropertyBase):
    def __init__(self, object, name, type, is_pointer=False):
        super().__init__(object, name, type)
        self._is_pointer = is_pointer

        self.value = None

    def is_pointer(self):
        return self._is_pointer

    def get_object(self):
        return self.value

    def write_value_to(self, stream):
        self.type.write_to(stream, self.value)

    def read_value_from(self, stream):
        self.value = self.type.read_from(stream)


class _VectorProperty(lib.pclass.DynamicPropertyBase):
    def __init__(self, object, name, type, is_pointer=False):
        super().__init__(object, name, type)
        self._is_pointer = is_pointer

        self.value = []

    def is_pointer(self):
        return self._is_pointer

    def get_element_count(self):
        return len(self.value)

    def get_object(self, index):
        return self.value[index]

    def write_value_to(self, stream, index):
        self.type.write_to(stream, self.value[index])

    def read_value_from(self, stream, index):
        self.value[index] = self.type.read_from(stream)
