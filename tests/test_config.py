import pytest
from ki.config import ConfigError, Config, ConfigCategory, ConfigVar


@pytest.fixture
def config():
    return Config()


@pytest.fixture
def sample_config(config):
    # category-a
    category_a = config.define_category(name='category-a')
    category_a.define_var(name='var-1')
    category_a.define_var(name='var-2')
    category_a.define_var(name='var-3')

    # category-a/category-b
    category_b = category_a.define_category(name='category-b')
    category_b.define_var(name='var-1')
    category_b.define_var(name='var-2')
    category_b.define_var(name='var-3')

    # type/default/constraint tests.
    category_b.define_var(
        name='var-4',
        type=int,
        default=0xFF,
        constraint=lambda value: 1 <= value <= 0xFF
    )

    return config


@pytest.fixture
def sample_data():
    return {
        'category-a': {
            'var-1': 1,
            'var-2': 2,
            'var-3': 3,
            'category-b': {
                'var-1': 1,
                'var-2': 2,
                'var-3': 3
            }
        }
    }


def test_add_category(config):
    # If the value passed is not a `ConfigCategory`, then an exit code of
    # `ConfigError.INTERNAL` should be given.
    with pytest.raises(SystemExit) as e:
        config.add_category(None)
    assert e.value.code == ConfigError.INTERNAL

    # If the `ConfigCategory` has no name, then an exit code of
    # `ConfigError.INVALID_DEFINITION` should be given.
    with pytest.raises(SystemExit) as e:
        config.add_category(ConfigCategory())
    assert e.value.code == ConfigError.INVALID_DEFINITION

    # Finally, let's try a valid `ConfigCategory`.
    category = ConfigCategory(name='test')
    config.add_category(category)
    assert category.parent is config
    assert category.name in config.categories


def test_add_var(config):
    # If the value passed is not a `ConfigVar`, then an exit code of
    # `ConfigError.INTERNAL` should be given.
    with pytest.raises(SystemExit) as e:
        config.add_var(None)
    assert e.value.code == ConfigError.INTERNAL

    # If the `ConfigVar` has no name, then an exit code of
    # `ConfigError.INVALID_DEFINITION` should be given.
    with pytest.raises(SystemExit) as e:
        config.add_var(ConfigVar())
    assert e.value.code == ConfigError.INVALID_DEFINITION

    # Finally, let's try a valid `ConfigVar`.
    var = ConfigVar(name='test')
    config.add_var(var)
    assert var.parent is config
    assert var.name in config.vars


def test_get(sample_config):
    # If the path given doesn't exist, then an exit code of
    # `ConfigError.INVALID_PATH` should be given.
    with pytest.raises(SystemExit) as e:
        sample_config.get('fake/path/test')
    assert e.value.code == ConfigError.INVALID_PATH

    # If the variable using the path given doesn't have a value set, and also
    # has no default value, then an exit code of `ConfigError.MISSING_DATA`
    # should be given.
    with pytest.raises(SystemExit) as e:
        sample_config.get('category-a/var-1')
    assert e.value.code == ConfigError.MISSING_DATA

    # Finally, let's try a variable with a default value.
    value = sample_config.get('category-a/category-b/var-4')
    assert value == 0xFF


def test_valid_data(sample_config, sample_data):
    missing_vars = sample_config.load_dict(sample_data)
    assert not missing_vars
    assert sample_config.get('category-a/var-1') == 1
    assert sample_config.get('category-a/var-2') == 2
    assert sample_config.get('category-a/var-3') == 3
    assert sample_config.get('category-a/category-b/var-1') == 1
    assert sample_config.get('category-a/category-b/var-2') == 2
    assert sample_config.get('category-a/category-b/var-3') == 3
    assert sample_config.get('category-a/category-b/var-4') == 0xFF


def test_invalid_data(sample_config, sample_data):
    # Delete some data.
    del sample_data['category-a']['var-3']
    del sample_data['category-a']['category-b']

    # Test for missing variables.
    missing_vars = sample_config.load_dict(sample_data)
    category_a = sample_config.categories['category-a']
    assert category_a.vars['var-1'] not in missing_vars
    assert category_a.vars['var-2'] not in missing_vars
    assert category_a.vars['var-3'] in missing_vars
    category_b = category_a.categories['category-b']
    assert category_b.vars['var-1'] in missing_vars
    assert category_b.vars['var-2'] in missing_vars
    assert category_b.vars['var-3'] in missing_vars
    assert category_b.vars['var-4'] not in missing_vars

    # When a value does not match the required type, then an exit code of
    # `ConfigError.INVALID_TYPE` should be given.
    with pytest.raises(SystemExit) as e:
        category_b.vars['var-4'].value = 'test'
    assert e.value.code == ConfigError.INVALID_TYPE

    # When a value does not meet the constraints, then an exit code of
    # `ConfigError.INVALID_VALUE` should be given.
    with pytest.raises(SystemExit) as e:
        category_b.vars['var-4'].value = 0xFF + 1
    assert e.value.code == ConfigError.INVALID_VALUE


def test_yaml_file(sample_config):
    # If the filename given doesn't exist, then an exit code of
    # `ConfigError.FILE_ERROR` should be given.
    with pytest.raises(SystemExit) as e:
        sample_config.load_yaml_file('fake/file/test.yml')
    assert e.value.code == ConfigError.FILE_ERROR

    # If the file loaded is empty, then an exit code of
    # `ConfigError.INVALID_DATA` should be given.
    with pytest.raises(SystemExit) as e:
        sample_config.load_yaml_file('tests/samples/empty_config.yml')
    assert e.value.code == ConfigError.INVALID_DATA

    # If the file loaded contains invalid YAML data, then an exit code of
    # `ConfigError.INVALID_DATA` should be given.
    with pytest.raises(SystemExit) as e:
        sample_config.load_yaml_file('tests/samples/invalid_config.yml')
    assert e.value.code == ConfigError.INVALID_DATA

    # Finally, let's try a valid file.
    sample_config.load_yaml_file('tests/samples/config.yml')
    assert sample_config.get('category-a/var-1') == 1
    assert sample_config.get('category-a/var-2') == 2
    assert sample_config.get('category-a/var-3') == 3
    assert sample_config.get('category-a/category-b/var-1') == 1
    assert sample_config.get('category-a/category-b/var-2') == 2
    assert sample_config.get('category-a/category-b/var-3') == 3
    assert sample_config.get('category-a/category-b/var-4') == 0xFF
