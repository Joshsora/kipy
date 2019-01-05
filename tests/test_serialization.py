import pytest

from ki.pclass import WizardHashCalculator, TypeSystem, PropertyClassMeta, \
    PropertyClass, StaticProperty, VectorProperty
from ki.serialization import BinarySerializerFlags, BinarySerializer
from ki.util import BitBuffer, BitStream

TEST_OBJECT_TYPE_NAME = 'class TestObject'
TEST_OBJECT_VALUES = {
    'm_int4': -6,
    'm_int8': 0x01,
    'm_int16': 0x0203,
    'm_int24': 0x040506,
    'm_int32': 0x0708090A,
    'm_int64': 0x0B0C0D0E0F101112,

    'm_uint4': 5,
    'm_uint8': 0x01,
    'm_uint16': 0x0203,
    'm_uint24': 0x040506,
    'm_uint32': 0x0708090A,
    'm_uint64': 0x0B0C0D0E0F101112,

    'm_string': 'This is a test value',
    'm_wstring': u'\u1d57\u02b0\u2071\u02e2\u0020\u2071\u02e2\u0020\u1d43\u0020'
                  '\u1d57\u1d49\u02e2\u1d57\u0020\u1d5b\u1d43\u02e1\u1d58\u1d49',

    'm_float32': 3.1415927410125732421875,
    'm_float64': 3.141592653589793115997963468544185161590576171875,
}

SAMPLE_FILE_DIR = 'tests/samples/serialization/'
BINARY_SAMPLES = {
    'regular.bin': (False, BinarySerializerFlags.NONE),
    'file.bin': (True, BinarySerializerFlags.WRITE_SERIALIZER_FLAGS),
    'regular_compressed.bin': (False, BinarySerializerFlags.COMPRESSED),
    'file_compressed.bin': (True, BinarySerializerFlags(9))
}
JSON_SAMPLES = {}


@pytest.fixture(scope='module')
def hash_calculator():
    return WizardHashCalculator()


@pytest.fixture(scope='module')
def type_system(hash_calculator):
    instance = TypeSystem(hash_calculator)
    PropertyClassMeta.type_system = instance

    class TestObject(PropertyClass):
        TYPE_NAME = TEST_OBJECT_TYPE_NAME

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

    return instance


@pytest.fixture
def test_object(type_system):
    instance = type_system.instantiate(TEST_OBJECT_TYPE_NAME)

    instance.int4 = TEST_OBJECT_VALUES['m_int4']
    instance.int8 = TEST_OBJECT_VALUES['m_int8']
    instance.int16 = TEST_OBJECT_VALUES['m_int16']
    instance.int24 = TEST_OBJECT_VALUES['m_int24']
    instance.int32 = TEST_OBJECT_VALUES['m_int32']
    instance.int64 = TEST_OBJECT_VALUES['m_int64']

    instance.uint4 = TEST_OBJECT_VALUES['m_uint4']
    instance.uint8 = TEST_OBJECT_VALUES['m_uint8']
    instance.uint16 = TEST_OBJECT_VALUES['m_uint16']
    instance.uint24 = TEST_OBJECT_VALUES['m_uint24']
    instance.uint32 = TEST_OBJECT_VALUES['m_uint32']
    instance.uint64 = TEST_OBJECT_VALUES['m_uint64']

    instance.string = TEST_OBJECT_VALUES['m_string']
    instance.wstring = TEST_OBJECT_VALUES['m_wstring']

    instance.float32 = TEST_OBJECT_VALUES['m_float32']
    instance.float64 = TEST_OBJECT_VALUES['m_float64']

    return instance


def _validate_test_object(test_object):
    assert test_object.int4 == TEST_OBJECT_VALUES['m_int4']
    assert test_object.int8 == TEST_OBJECT_VALUES['m_int8']
    assert test_object.int16 == TEST_OBJECT_VALUES['m_int16']
    assert test_object.int24 == TEST_OBJECT_VALUES['m_int24']
    assert test_object.int32 == TEST_OBJECT_VALUES['m_int32']
    assert test_object.int64 == TEST_OBJECT_VALUES['m_int64']

    assert test_object.uint4 == TEST_OBJECT_VALUES['m_uint4']
    assert test_object.uint8 == TEST_OBJECT_VALUES['m_uint8']
    assert test_object.uint16 == TEST_OBJECT_VALUES['m_uint16']
    assert test_object.uint24 == TEST_OBJECT_VALUES['m_uint24']
    assert test_object.uint32 == TEST_OBJECT_VALUES['m_uint32']
    assert test_object.uint64 == TEST_OBJECT_VALUES['m_uint64']

    assert test_object.string == TEST_OBJECT_VALUES['m_string']
    assert test_object.wstring == TEST_OBJECT_VALUES['m_wstring']

    assert test_object.float32 == TEST_OBJECT_VALUES['m_float32']
    assert test_object.float64 == TEST_OBJECT_VALUES['m_float64']


@pytest.mark.parametrize("sample_filename", BINARY_SAMPLES.keys())
def test_binary_serialization(sample_filename, type_system, test_object):
    params = BINARY_SAMPLES[sample_filename]
    serializer = BinarySerializer(type_system, *params)

    with open(SAMPLE_FILE_DIR + sample_filename, 'rb') as f:
        sample_data = f.read()

    # Create a new stream.
    buffer = BitBuffer()
    stream = BitStream(buffer)

    # Serialize the object, and save it to the stream.
    serializer.save(test_object, stream)
    size = stream.tell().as_bytes()

    # Validate the contents of the stream.
    assert stream.buffer.data[:size] == sample_data


@pytest.mark.parametrize("sample_filename", BINARY_SAMPLES.keys())
def test_binary_deserialization(sample_filename, type_system):
    params = BINARY_SAMPLES[sample_filename]
    serializer = BinarySerializer(type_system, *params)

    with open(SAMPLE_FILE_DIR + sample_filename, 'rb') as f:
        sample_data = f.read()

    # Create a stream using the sample data.
    buffer = BitBuffer(sample_data, len(sample_data))
    stream = BitStream(buffer)

    # Deserialize the data.
    test_object = serializer.load(stream, stream.buffer.size)

    # Validate the object.
    _validate_test_object(test_object)


@pytest.mark.parametrize("sample_filename", JSON_SAMPLES.keys())
def test_json_serialization(sample_filename, type_system):
    raise NotImplementedError


@pytest.mark.parametrize("sample_filename", JSON_SAMPLES.keys())
def test_json_deserialization(sample_filename, type_system):
    raise NotImplementedError
