from .lib.serialization import BinarySerializer, JsonSerializer, \
    XmlSerializer, BinarySerializerFlags
from .lib.util import BitBuffer, BitStream

__all__ = [
    'BinarySerializer', 'JsonSerializer', 'XmlSerializer',
    'BinarySerializerFlags', 'SerializedFile'
]


class SerializedFile(object):
    BINARY_HEADER = b'BINd'
    JSON_HEADER = b'JSON'

    type_system = None

    def __init__(self, path, mode='rb', newline=b'\n'):
        self.path = path
        self.mode = mode
        self.newline = newline

        if mode not in ('rb', 'wb'):
            raise ValueError('invalid mode: %r' % mode)
        if not isinstance(newline, bytes):
            raise ValueError('newline must be a bytes-like object')

        self._file = None

    def __enter__(self):
        self._file = open(self.path, self.mode)
        return self

    def __exit__(self, type, value, traceback):
        self._file.close()

    def _read_binary(self):
        data = self._file.read()
        buffer = BitBuffer(data, len(data))
        stream = BitStream(buffer)
        serializer = BinarySerializer(
            self.type_system, True,
            BinarySerializerFlags.WRITE_SERIALIZER_FLAGS)
        return serializer.load(stream)

    def _read_json(self):
        data = self._file.read()
        serializer = JsonSerializer(self.type_system, True)
        return serializer.load(data)

    def _read_xml(self):
        self._file.seek(0)

        data = self._file.read()
        serializer = XmlSerializer(self.type_system)
        return serializer.load(data)

    def read(self):
        self._file.seek(0)

        # Make sure that there is at least enough data to determine
        # which serializer was used.
        header = self._file.read(4)
        if len(header) < 4:
            raise RuntimeError('Not enough data to determine serializer used '
                               'in file: %s' % self.path)

        # Use the first 4 bytes to determine which serializer was used.
        if header == self.BINARY_HEADER:
            return self._read_binary()
        elif header == self.JSON_HEADER:
            return self._read_json()
        else:
            return self._read_xml()

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
        self._file.write(self.JSON_HEADER + self.newline)
        self._file.write(data.encode().replace(b'\n', self.newline))

    def write_xml(self, object):
        self._file.seek(0)

        # Serialize the object.
        serializer = XmlSerializer(self.type_system)
        data = serializer.save(object)

        # Write to the file.
        self._file.write(b'<?xml version="1.0" encoding="UTF-8"?>' + self.newline)
        self._file.write(data.encode().replace(b'\n', self.newline))
