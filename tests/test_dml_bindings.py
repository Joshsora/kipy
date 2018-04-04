import pytest


@pytest.fixture
def record():
    from ki.dml import Record
    return Record()


def test_add_field(record):
    # Checking the presence of a field that doesn't exist should return False.
    assert record.has_byt_field('TestField') is False
    assert 'TestField' not in record

    # Attempting to retrieve a field that doesn't exist should return None.
    assert record.get_byt_field('TestField') is None

    # Attempting to retrieve a field that doesn't exist via the __getitem__
    # descriptor should raise a KeyError.
    with pytest.raises(KeyError):
        field = record['TestField']

    # Adding fields should return a new Field.
    field = record.add_byt_field('TestField')
    assert field is not None

    # Adding fields with the same name and type should return the previously
    # created Field.
    assert record.add_byt_field('TestField') is field

    # Adding fields with the same name but different types should return None.
    assert record.add_shrt_field('TestField') is None

    # Checking the presence of the previously created field with the correct
    # type should return True.
    assert record.has_byt_field('TestField') is True
    assert 'TestField' in record

    # Checking the presence of the previously created field with the incorrect
    # type should return False.
    assert record.has_shrt_field('TestField') is False

    # Retrieving fields with the correct type should return the previously
    # created Field.
    assert record.get_byt_field('TestField') is field

    # Retrieving fields with an incorrect type should return None.
    assert record.get_shrt_field('TestField') is None

    # Retrieving fields via the __getitem__ descriptor should return the
    # previously created Field.
    assert record['TestField'] is field

    # The field count should now be 1.
    assert record.field_count == 1

    # Size should now be 1.
    assert record.size == 1


def test_field_iteration(record):
    # Add some fields to iterate through.
    byt_field = record.add_byt_field('TestByt')
    shrt_field = record.add_shrt_field('TestShrt')
    int_field = record.add_int_field('TestInt')

    # Test that we can iterate over them correctly.
    i = 0
    for field in record:
        if i == 0:
            assert field is byt_field
        elif i == 1:
            assert field is shrt_field
        elif i == 2:
            assert field is int_field
        i += 1
    assert record.field_count == i


def test_non_transferable(record):
    field = record.add_str_field('TestNOXFER', False)
    field.value = 'Hello, world!'
    assert field.transferable is False
    assert record.to_bytes() == b''


def test_byt_serialization(record):
    field = record.add_byt_field('TestByt')
    field.value = -127
    assert record.to_bytes() == b'\x81'


def test_ubyt_serialization(record):
    field = record.add_ubyt_field('TestUByt')
    field.value = 255
    assert record.to_bytes() == b'\xFF'


def test_shrt_serialization(record):
    field = record.add_shrt_field('TestShrt')
    field.value = -32768
    assert record.to_bytes() == b'\x00\x80'


def test_ushrt_serialization(record):
    field = record.add_ushrt_field('TestUShrt')
    field.value = 65535
    assert record.to_bytes() == b'\xFF\xFF'


def test_int_serialization(record):
    field = record.add_int_field('TestInt')
    field.value = -2147483648
    assert record.to_bytes() == b'\x00\x00\x00\x80'


def test_uint_serialization(record):
    field = record.add_uint_field('TestUInt')
    field.value = 4294967295
    assert record.to_bytes() == b'\xFF\xFF\xFF\xFF'


def test_str_serialization(record):
    field = record.add_str_field('TestStr')
    field.value = 'TEST'
    assert record.to_bytes() == b'\x04\x00TEST'


def test_wstr_serialization(record):
    field = record.add_wstr_field('TestWStr')
    field.value = 'TEST'
    assert record.to_bytes() == b'\x04\x00T\x00E\x00S\x00T\x00'


def test_flt_serialization(record):
    field = record.add_flt_field('TestFlt')
    field.value = 152.4
    assert record.to_bytes() == b'\x66\x66\x18\x43'


def test_dbl_serialization(record):
    field = record.add_dbl_field('TestDbl')
    field.value = 152.4
    assert record.to_bytes() == b'\xCD\xCC\xCC\xCC\xCC\x0C\x63\x40'


def test_gid_serialization(record):
    field = record.add_gid_field('TestGid')
    field.value = 0x8899AABBCCDDEEFF
    assert record.to_bytes() == b'\xFF\xEE\xDD\xCC\xBB\xAA\x99\x88'


def test_byt_deserialization(record):
    field = record.add_byt_field('TestByt')
    record.from_bytes(b'\x81')
    assert field.value == -127


def test_ubyt_deserialization(record):
    field = record.add_ubyt_field('TestUByt')
    record.from_bytes(b'\xFF')
    assert field.value == 255


def test_shrt_deserialization(record):
    field = record.add_shrt_field('TestShrt')
    record.from_bytes(b'\x00\x80')
    assert field.value == -32768


def test_ushrt_deserialization(record):
    field = record.add_ushrt_field('TestUShrt')
    record.from_bytes(b'\xFF\xFF')
    assert field.value == 65535


def test_int_deserialization(record):
    field = record.add_int_field('TestInt')
    record.from_bytes(b'\x00\x00\x00\x80')
    assert field.value == -2147483648


def test_uint_deserialization(record):
    field = record.add_uint_field('TestUInt')
    record.from_bytes(b'\xFF\xFF\xFF\xFF')
    assert field.value == 4294967295


def test_str_deserialization(record):
    field = record.add_str_field('TestStr')
    record.from_bytes(b'\x04\x00TEST')
    assert field.value == 'TEST'


def test_wstr_deserialization(record):
    field = record.add_wstr_field('TestWStr')
    record.from_bytes(b'\x04\x00T\x00E\x00S\x00T\x00')
    assert field.value == 'TEST'


def test_flt_deserialization(record):
    import math
    field = record.add_flt_field('TestFlt')
    record.from_bytes(b'\x66\x66\x18\x43')
    assert math.isclose(field.value, 152.4, rel_tol=1e-07)


def test_dbl_deserialization(record):
    import math
    field = record.add_dbl_field('TestDbl')
    record.from_bytes(b'\xCD\xCC\xCC\xCC\xCC\x0C\x63\x40')
    assert math.isclose(field.value, 152.4, rel_tol=1e-07)


def test_gid_deserialization(record):
    field = record.add_gid_field('TestGid')
    record.from_bytes(b'\xFF\xEE\xDD\xCC\xBB\xAA\x99\x88')
    assert field.value == 0x8899AABBCCDDEEFF


def test_record_serialization(record):
    byt_field = record.add_byt_field('TestByt')
    ubyt_field = record.add_ubyt_field('TestUByt')
    shrt_field = record.add_shrt_field('TestShrt')
    ushrt_field = record.add_ushrt_field('TestUShrt')
    int_field = record.add_int_field('TestInt')
    uint_field = record.add_uint_field('TestUInt')
    str_field = record.add_str_field('TestStr')
    wstr_field = record.add_wstr_field('TestWStr')
    flt_field = record.add_flt_field('TestFlt')
    dbl_field = record.add_dbl_field('TestDbl')
    gid_field = record.add_gid_field('TestGid')
    noxfer_field = record.add_byt_field('TestNOXFER', False)

    byt_field.value = -127
    ubyt_field.value = 255
    shrt_field.value = -32768
    ushrt_field.value = 65535
    int_field.value = -2147483648
    uint_field.value = 4294967295
    str_field.value = 'TEST'
    wstr_field.value = 'TEST'
    flt_field.value = 152.4
    dbl_field.value = 152.4
    gid_field.value = 0x8899AABBCCDDEEFF
    noxfer_field.value = -127

    # Our serialized data should match our sample data.
    with open('tests/samples/dml.bin', 'rb') as f:
        assert record.to_bytes() == f.read()


def test_record_deserialization(record):
    import math

    byt_field = record.add_byt_field('TestByt')
    ubyt_field = record.add_ubyt_field('TestUByt')
    shrt_field = record.add_shrt_field('TestShrt')
    ushrt_field = record.add_ushrt_field('TestUShrt')
    int_field = record.add_int_field('TestInt')
    uint_field = record.add_uint_field('TestUInt')
    str_field = record.add_str_field('TestStr')
    wstr_field = record.add_wstr_field('TestWStr')
    flt_field = record.add_flt_field('TestFlt')
    dbl_field = record.add_dbl_field('TestDbl')
    gid_field = record.add_gid_field('TestGid')
    noxfer_field = record.add_byt_field('TestNOXFER', False)

    with open('tests/samples/dml.bin', 'rb') as f:
        record.from_bytes(f.read())

    assert byt_field.value == -127
    assert ubyt_field.value == 255
    assert shrt_field.value == -32768
    assert ushrt_field.value == 65535
    assert int_field.value == -2147483648
    assert uint_field.value == 4294967295
    assert str_field.value == 'TEST'
    assert wstr_field.value == 'TEST'
    assert math.isclose(flt_field.value, 152.4, rel_tol=1e-07)
    assert math.isclose(dbl_field.value, 152.4, rel_tol=1e-07)
    assert gid_field.value == 0x8899AABBCCDDEEFF
    assert noxfer_field.value == 0x0
