import pytest

from ki.pclass import WizardHashCalculator, TypeSystem, PropertyClassMeta, \
    PropertyClass


@pytest.fixture
def hash_calculator():
    return WizardHashCalculator()


@pytest.fixture
def type_system(hash_calculator):
    instance = TypeSystem(hash_calculator)
    PropertyClassMeta.type_system = instance
    return instance


def test_premature_class_definition():
    with pytest.raises(RuntimeError):
        class A(PropertyClass):
            pass


def test_nonexistent_type_name(type_system):
    # Exported C++ Method
    with pytest.raises(RuntimeError):
        type_system.get_type('struct MadeUp')

    # Python Descriptor
    with pytest.raises(RuntimeError):
        type_system['struct MadeUp']


def test_nonexistent_type_hash(type_system):
    # Exported C++ Method
    with pytest.raises(RuntimeError):
        type_system.get_type(0xDEADA55)

    # Python Descriptor
    with pytest.raises(RuntimeError):
        type_system[0xDEADA55]


def test_class_definition(type_system):
    name = 'class A'
    hash = type_system.hash_calculator.calculate_type_hash(name)

    class A(PropertyClass):
        pass

    # Exported C++ Methods
    assert type_system.has_type(name)
    assert type_system.has_type(hash)

    # Python Descriptors
    assert name in type_system
    assert hash in type_system


def test_type_name_override(type_system):
    name = 'struct B'
    hash = type_system.hash_calculator.calculate_type_hash(name)

    class A(PropertyClass):
        TYPE_NAME = name

    # Exported C++ Methods
    assert type_system.has_type(name)
    assert type_system.has_type(hash)

    # Python Descriptors
    assert name in type_system
    assert hash in type_system


def test_class_instantiation(type_system):
    name = 'class A'
    hash = type_system.hash_calculator.calculate_type_hash(name)

    class A(PropertyClass):
        pass

    assert isinstance(type_system.instantiate(name), A)
    assert isinstance(type_system.instantiate(hash), A)
