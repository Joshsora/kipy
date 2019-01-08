import pytest

from ki.pclass import WizardHashCalculator, TypeSystem, PropertyClassMeta, \
    PropertyClass, StaticProperty, VectorProperty, ArrayProperty
from ki.serialization import BinarySerializerFlags, BinarySerializer
from ki.util import BitBuffer, BitStream

SAMPLE_DIR = 'tests/samples/serialization/'
BINARY_TEST_PARAMS = (
    ('regular.bin', False, BinarySerializerFlags.NONE),
    ('file.bin', True, BinarySerializerFlags.WRITE_SERIALIZER_FLAGS),
    ('regular_compressed.bin', False, BinarySerializerFlags.COMPRESSED),
    ('file_compressed.bin', True, BinarySerializerFlags(9))
)
JSON_TEST_PARAMS = ()


@pytest.fixture(scope='module')
def hash_calculator():
    return WizardHashCalculator()


@pytest.fixture(scope='module')
def type_system(hash_calculator):
    instance = TypeSystem(hash_calculator)
    PropertyClassMeta.type_system = instance

    class TestObject(PropertyClass):
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

        int_array = ArrayProperty('m_int_array', 'int', 5)
        int_ptr_array = ArrayProperty('m_int_ptr_array', 'int', 5, is_pointer=True)

        int_vector = VectorProperty('m_int_vector', 'int')
        int_ptr_vector = VectorProperty('m_int_ptr_vector', 'int', is_pointer=True)

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
            'int_ptr_vector': [*range(100)]
        }

        def apply_test_values(self):
            for key, value in self._TEST_VALUES.items():
                setattr(self, key, value)

        def validate_test_values(self):
            for key, value in self._TEST_VALUES.items():
                assert getattr(self, key) == value

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


@pytest.mark.parametrize("sample_filename,is_file,flags", JSON_TEST_PARAMS)
def test_json_serialization(sample_filename, is_file, flags, type_system, test_object):
    raise NotImplementedError


@pytest.mark.parametrize("sample_filename,is_file,flags", JSON_TEST_PARAMS)
def test_json_deserialization(sample_filename, is_file, flags, type_system):
    raise NotImplementedError
