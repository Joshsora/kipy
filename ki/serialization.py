from .lib.serialization import BinarySerializer, JsonSerializer, \
    BinarySerializerFlags
from .lib.util import BitBuffer, BitStream

__all__ = [
    'BinarySerializer', 'JsonSerializer', 'BinarySerializerFlags', 'SerializedFile'
]


class SerializedFile(object):
    BINARY_HEADER = b'BINd'
    JSON_HEADER = b'JSON'

    type_system = None

    def __init__(self, path, mode='rb'):
        self.path = path
        self.mode = mode

        assert mode in ('rb', 'wb'), \
            'SerializedFile must be opened in binary mode.'

        self._file = None

    def __enter__(self):
        self._file = open(self.path, self.mode)
        return self

    def __exit__(self, type, value, traceback):
        self._file.close()

    def read(self):
        self._file.seek(0)

        # Make sure that there is at least enough data to determine
        # which serializer was used.
        header = self._file.read(4)
        if len(header) < 4:
            raise RuntimeError('Not enough data to determine serializer used '
                               'in file: %s' % self.path)

        # Read the rest of the file.
        data = self._file.read()

        # Use the first 4 bytes to determine which serializer was used.
        if header == self.BINARY_HEADER:
            buffer = BitBuffer(data, len(data))
            stream = BitStream(buffer)
            serializer = BinarySerializer(
                self.type_system, True,
                BinarySerializerFlags.WRITE_SERIALIZER_FLAGS)
            return serializer.load(stream)
        elif header == self.JSON_HEADER:
            serializer = JsonSerializer(self.type_system, True)
            return serializer.load(data)
        else:
            raise NotImplementedError  # TODO

    def write_binary(self, object, flags=BinarySerializerFlags.NONE):
        self._file.seek(0)

        # Force the WRITE_SERIALIZER_FLAGS flag so that the correct flags
        # can be loaded later.
        flags |= BinarySerializerFlags.WRITE_SERIALIZER_FLAGS

        # Serialize the object.
        buffer = BitBuffer()
        stream = BitStream(buffer)
        serializer = BinarySerializer(self.type_system, True, flags)
        serializer.save(object, stream)

        # Write to the file.
        self._file.write(self.BINARY_HEADER)
        self._file.write(buffer.data[:stream.tell().as_bytes()])

    def write_json(self, object):
        self._file.seek(0)

        # Serialize the object.
        serializer = JsonSerializer(self.type_system, True)
        data = serializer.save(object)

        # Write to the file.
        self._file.write(self.JSON_HEADER)
        self._file.write(data.encode())

    def write_xml(self, object):
        raise NotImplementedError  # TODO
