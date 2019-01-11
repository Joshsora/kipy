import pytest

from ki.pclass import WizardHashCalculator, TypeSystem, PropertyClassMeta, \
    PropertyClass, EnumMeta, Enum, StaticProperty, VectorProperty
from ki.serialization import BinarySerializerFlags, BinarySerializer, \
    JsonSerializer
from ki.util import BitBuffer, BitStream

SAMPLE_DIR = 'tests/samples/serialization/'
BINARY_TEST_PARAMS = (
    ('regular.bin', False, BinarySerializerFlags.NONE),
    ('file.bin', True, BinarySerializerFlags.WRITE_SERIALIZER_FLAGS),
    ('regular_compressed.bin', False, BinarySerializerFlags.COMPRESSED),
    ('file_compressed.bin', True, BinarySerializerFlags(9))
)
JSON_TEST_PARAMS = (
    ('regular.json', False),
    ('file.json', True),
)


@pytest.fixture(scope='module')
def hash_calculator():
    return WizardHashCalculator()


@pytest.fixture(scope='module')
def type_system(hash_calculator):
    instance = TypeSystem(hash_calculator)
    PropertyClassMeta.type_system = instance
    EnumMeta.type_system = instance


    class TestablePropertyClass(PropertyClass):
        _TEST_VALUES = {}

        def apply_test_values(self):
            for key, value in self._TEST_VALUES.items():
                setattr(self, key, value)

        def validate_test_values(self):
            for key, value in self._TEST_VALUES.items():
                assert getattr(self, key) == value


    class NestedObjectKind(Enum):
        NONE = 0
        OBJECT = 1
        OBJECT_A = 2
        OBJECT_B = 3


    class NestedTestObject(TestablePropertyClass):
        kind = StaticProperty('m_kind', 'enum NestedObjectKind')

        _TEST_VALUES = {'kind': NestedObjectKind.OBJECT}


    class NestedTestObjectA(NestedTestObject):
        extra_value = StaticProperty('m_extra_value', 'int')

        _TEST_VALUES = {
            'kind': NestedObjectKind.OBJECT_A,
            'extra_value': 10
        }


    class NestedTestObjectB(NestedTestObject):
        _TEST_VALUES = {'kind': NestedObjectKind.OBJECT_B}


    class TestObject(TestablePropertyClass):
        int4 = StaticProperty('m_int4', 'bi4')
        int8 = StaticProperty('m_int8', 'char')
        int16 = StaticProperty('m_int16', 'short')
        int24 = StaticProperty('m_int24', 's24')
        int32 = StaticProperty('m_int32', 'int')
        int64 = StaticProperty('m_int64', 'long')

        uint4 = StaticProperty('m_uint4', 'bui4')
        uint8 = StaticProperty('m_uint8', 'unsigned char')
        uint16 = StaticProperty('m_uint16', 'unsigned short')
        uint24 = StaticProperty('m_uint24', 'u24')
        uint32 = StaticProperty('m_uint32', 'unsigned int')
        uint64 = StaticProperty('m_uint64', 'unsigned long')

        string = StaticProperty('m_string', 'std::string')
        wstring = StaticProperty('m_wstring', 'std::wstring')

        float32 = StaticProperty('m_float32', 'float')
        float64 = StaticProperty('m_float64', 'double')

        int_ptr = StaticProperty('m_int_ptr', 'int', is_pointer=True)

        int_array = VectorProperty('m_int_array', 'int')
        int_ptr_array = VectorProperty('m_int_ptr_array', 'int', is_pointer=True)

        int_vector = VectorProperty('m_int_vector', 'int')
        int_ptr_vector = VectorProperty('m_int_ptr_vector', 'int', is_pointer=True)

        object = StaticProperty('m_object', 'class NestedTestObjectA')
        object_ptr = StaticProperty('m_object_ptr', 'class NestedTestObject',
                                    is_pointer=True)
        null_object_ptr = StaticProperty('m_null_object_ptr', 'class NestedTestObject',
                                         is_pointer=True)

        object_vector = VectorProperty('m_object_vector', 'class NestedTestObject',
                                       is_pointer=True)
        object_ptr_vector = VectorProperty('m_object_ptr_vector', 'class NestedTestObject',
                                           is_pointer=True)

        _TEST_VALUES = {
            'int4': -6,
            'int8': 0x01,
            'int16': 0x0203,
            'int24': 0x040506,
            'int32': 0x0708090A,
            'int64': 0x0B0C0D0E0F101112,

            'uint4': 5,
            'uint8': 0x01,
            'uint16': 0x0203,
            'uint24': 0x040506,
            'uint32': 0x0708090A,
            'uint64': 0x0B0C0D0E0F101112,

            'string': 'This is a test value',
            'wstring': u'\u1d57\u02b0\u2071\u02e2\u0020\u2071\u02e2\u0020\u1d43\u0020'
                          '\u1d57\u1d49\u02e2\u1d57\u0020\u1d5b\u1d43\u02e1\u1d58\u1d49',

            'float32': 3.1415927410125732421875,
            'float64': 3.141592653589793115997963468544185161590576171875,

            'int_ptr': 52,

            'int_array': [*range(5)],
            'int_ptr_array': [*range(5)],

            'int_vector': [*range(100)],
            'int_ptr_vector': [*range(100)],

            'null_object_ptr': None
        }

        def apply_test_values(self):
            super().apply_test_values()

            object = instance.instantiate('class NestedTestObject')
            object.apply_test_values()
            object_a = instance.instantiate('class NestedTestObjectA')
            object_a.apply_test_values()
            object_b = instance.instantiate('class NestedTestObjectB')
            object_b.apply_test_values()

            self.object = object_a
            self.object_ptr = object_a

            self.object_vector = [object, object_a, object_b]
            self.object_ptr_vector = [object, object_a, object_b]

        def validate_test_values(self):
            super().validate_test_values()

            assert isinstance(self.object, NestedTestObjectA)
            self.object.validate_test_values()

            assert isinstance(self.object_ptr, NestedTestObjectA)
            self.object_ptr.validate_test_values()

            assert len(self.object_vector) == 3
            assert isinstance(self.object_vector[0], NestedTestObject)
            assert isinstance(self.object_vector[1], NestedTestObjectA)
            assert isinstance(self.object_vector[2], NestedTestObjectB)

            for object in self.object_vector:
                object.validate_test_values()

            assert len(self.object_ptr_vector) == 3
            assert isinstance(self.object_ptr_vector[0], NestedTestObject)
            assert isinstance(self.object_ptr_vector[1], NestedTestObjectA)
            assert isinstance(self.object_ptr_vector[2], NestedTestObjectB)

            for object_ptr in self.object_ptr_vector:
                object_ptr.validate_test_values()

    return instance


@pytest.fixture
def test_object(type_system):
    instance = type_system.instantiate('class TestObject')
    instance.apply_test_values()
    return instance


@pytest.mark.parametrize("sample_filename,is_file,flags", BINARY_TEST_PARAMS)
def test_binary_serialization(sample_filename, is_file, flags, type_system, test_object):
    serializer = BinarySerializer(type_system, is_file, flags)

    with open(SAMPLE_DIR + sample_filename, 'rb') as f:
        sample_data = f.read()

    # Create a new stream.
    buffer = BitBuffer()
    stream = BitStream(buffer)

    # Serialize the object, and save it to the stream.
    serializer.save(test_object, stream)
    size = stream.tell().as_bytes()

    # Validate the contents of the stream.
    assert stream.buffer.data[:size] == sample_data


@pytest.mark.parametrize("sample_filename,is_file,flags", BINARY_TEST_PARAMS)
def test_binary_deserialization(sample_filename, is_file, flags, type_system):
    serializer = BinarySerializer(type_system, is_file, flags)

    with open(SAMPLE_DIR + sample_filename, 'rb') as f:
        sample_data = f.read()

    # Create a stream using the sample data.
    buffer = BitBuffer(sample_data, len(sample_data))
    stream = BitStream(buffer)

    # Deserialize the data.
    test_object = serializer.load(stream, stream.buffer.size)

    # Validate the object.
    test_object.validate_test_values()


@pytest.mark.parametrize("sample_filename,is_file", JSON_TEST_PARAMS)
def test_json_serialization(sample_filename, is_file, type_system, test_object):
    serializer = JsonSerializer(type_system, is_file)

    with open(SAMPLE_DIR + sample_filename, 'r') as f:
        sample_data = f.read()

    # Serialize the object.
    data = serializer.save(test_object)

    # Validate the data.
    assert data == sample_data


@pytest.mark.parametrize("sample_filename,is_file", JSON_TEST_PARAMS)
def test_json_deserialization(sample_filename, is_file, type_system):
    serializer = JsonSerializer(type_system, is_file)

    with open(SAMPLE_DIR + sample_filename, 'r') as f:
        sample_data = f.read()

    # Deserialize the data.
    test_object = serializer.load(sample_data)

    # Validate the object.
    test_object.validate_test_values()
