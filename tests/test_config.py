import pytest
from ki import config
from ki.errors import UserError


def sample_constraint(value):
    """
    The value must be within the range of 1-255.
    """
    return 1 <= value <= 0xFF


@pytest.fixture
def sample_config():
    config.define('group-a/var-1')
    config.define('group-a/var-2')
    config.define('group-a/var-3')

    config.define('group-a/group-b/var-1')
    config.define('group-a/group-b/var-2')
    config.define('group-a/group-b/var-3')

    # type/default/constraint tests.
    config.define('group-a/group-b/var-4',
                  type=int,
                  default=0xFF,
                  constraint=sample_constraint)

    yield config

    # Clear our variable definitions
    config._config.variables = []


@pytest.fixture
def sample_data():
    return {
        'group-a': {
            'var-1': 1,
            'var-2': 2,
            'var-3': 3,
            'group-b': {
                'var-1': 1,
                'var-2': 2,
                'var-3': 3
            }
        }
    }


def test_get(sample_config):
    # If the path given doesn't exist, then a KeyError should be
    # raised.
    with pytest.raises(KeyError):
        sample_config.get('fake/path/test')

    # If the variable using the path given doesn't have a value set, and also
    # has no default value, then a KeyError should be raised.
    with pytest.raises(KeyError):
        sample_config.get('group-a/var-1')

    # Finally, let's try a variable with a default value.
    value = sample_config.get('group-a/group-b/var-4')
    assert value == 0xFF


def test_valid_data(sample_config, sample_data):
    print(config._config.variables)
    sample_config.load_dict(sample_data)
    assert sample_config.get('group-a/var-1') == 1
    assert sample_config.get('group-a/var-2') == 2
    assert sample_config.get('group-a/var-3') == 3
    assert sample_config.get('group-a/group-b/var-1') == 1
    assert sample_config.get('group-a/group-b/var-2') == 2
    assert sample_config.get('group-a/group-b/var-3') == 3
    assert sample_config.get('group-a/group-b/var-4') == 0xFF


def test_invalid_data(sample_config, sample_data):
    # Delete some data.
    del sample_data['group-a']['var-3']
    del sample_data['group-a']['group-b']

    # If the data loaded is missing required variables, then a
    # UserError should be raised.
    with pytest.raises(UserError):
        sample_config.load_dict(sample_data)

    # When a value does not match the required type, then a UserError
    # should be raised.
    with pytest.raises(UserError):
        for variable in sample_config._config.variables:
            if variable.path == 'group-a/group-b/var-4':
                variable.value = 'test'

    # When a value does not meet the constraint requirements, then a
    # UserError should be raised.
    with pytest.raises(UserError):
        for variable in sample_config._config.variables:
            if variable.path == 'group-a/group-b/var-4':
                variable.value = 0xFF + 1


def test_yaml_file(sample_config):
    # If the filepath doesn't exist, then a UserError should be raised.
    with pytest.raises(UserError):
        sample_config.load_yaml_file('fake/file/test.yml')

    # If the file loaded is empty, then a UserError should be raised.
    with pytest.raises(UserError):
        sample_config.load_yaml_file('tests/samples/empty_config.yml')

    # If the file loaded contains invalid YAML data, then a UserError
    # should be raised.
    with pytest.raises(UserError):
        sample_config.load_yaml_file('tests/samples/invalid_config.yml')

    # Finally, let's try a valid file.
    sample_config.load_yaml_file('tests/samples/config.yml')
    assert sample_config.get('group-a/var-1') == 1
    assert sample_config.get('group-a/var-2') == 2
    assert sample_config.get('group-a/var-3') == 3
    assert sample_config.get('group-a/group-b/var-1') == 1
    assert sample_config.get('group-a/group-b/var-2') == 2
    assert sample_config.get('group-a/group-b/var-3') == 3
    assert sample_config.get('group-a/group-b/var-4') == 0xFF
