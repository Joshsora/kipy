from ruamel import yaml

from .errors import UserError


class ConfigError(Exception):
    pass


class ConfigVariable(object):
    """
    A config variable.

    It is recommended that you only create config variables using the
    :func:`Config.define` method.

    See the examples [citation needed] for a better understanding.
    """

    def __init__(self, path, description=None, type=None, default=None, constraint=None):
        self._value = default

        self.path = path
        self.description = description
        self.type = type
        self.constraint = constraint

        if constraint is not None and not callable(constraint):
            raise TypeError('The constraint given for %r is not callable.' % self)

        if default is not None:
            if type is not None and not isinstance(default, type):
                raise TypeError('The value given as a default for %r is not of type: %r' %
                                (self, type))

            if constraint is not None and not constraint(default):
                raise ValueError('The value given as a default for %r does not meet its '
                                 'constraint requirements.' % self)

    def __repr__(self):
        return '<%s>' % self.path

    @property
    def value(self):
        return self._value

    @value.setter
    def value(self, value):
        # Validate the type.
        if self.type is not None and not isinstance(value, self.type):
            raise UserError(name='Config/TypeError',
                            description='The value given is not of the correct data type for this '
                                        'variable.',
                            solution='Fix the value in the config.\n'
                                     'The value of %r must be of type: %r' % (self, value))

        # Verify that it meets the constraint requirements.
        if self.constraint is not None and not self.constraint(value):
            raise UserError(name='Config/ValueError',
                            description='The value given does not meet the constraint '
                                        'requirements for this variable.',
                            solution='Fix the value in the config.\n' + self.constraint.__doc__)

        self._value = value


class Config(object):
    """
    A config representation.

    To define variables that you intend to read, use the
    :func:`Config.define` method.

    After defining your variables, use either the
    :func:`Config.load_dict`, or :func:`Config.load_yaml_file` methods
    to read in variable values.

    To then later get the values of your variables, use the
    :func:`Config.get` method.
    """
    def __init__(self):
        self.variables = []

    def define(self, path, description=None, type=None, default=None, constraint=None):
        """
        Defines a variable.

        :rtype: :class:`ConfigVariable`
        """
        if self.is_defined(path):
            raise ConfigError('Tried to define <%s>, but it has already been defined.' % path)

        variable = ConfigVariable(path, description=description, type=type,
                                  default=default, constraint=constraint)
        self.variables.append(variable)
        return variable

    def is_defined(self, path):
        """
        Returns whether or not a variable using the given path has been
        defined.

        :rtype: bool
        """
        for variable in self.variables:
            if variable.path == path:
                return True
        return False

    def get(self, path):
        """
        Returns the value of the variable using the given path.

        :rtype: :class:`ConfigVariable`
        """
        for variable in self.variables:
            if variable.path == path:
                if variable.value is None:
                    break
                return variable.value

        raise KeyError("Couldn't get the value of <%s>" % path)

    def load_dict(self, data):
        """
        Loads variable data using the given dictionary.
        """
        missing_variables = []

        for variable in self.variables:
            group_names = variable.path.split('/')
            variable_name = group_names.pop()

            # Find the group.
            group = data
            for group_name in group_names:
                group = group.get(group_name, {})

            # Find the value.
            value = group.get(variable_name)
            if value is not None:
                variable.value = value
            else:
                missing_variables.append(variable)

        if missing_variables:
            solution = 'Define the following variables in the config:\n'
            for variable in missing_variables:
                solution += '\t%r\n' % variable

            raise UserError(name='Config/MissingVariables',
                            description='There are required variables that are missing.',
                            solution=solution)

    def load_yaml_file(self, path):
        """
        Loads variable data using the given YAML file.

        This YAML file must contain a dictionary.
        """
        try:
            with open(path, 'r') as f:
                data = f.read()
        except IOError as e:
            raise UserError(name='Config/IOError',
                            description='Failed to read the config file.',
                            solution='Make sure the file exists: %r\n'
                                     'Runtime info:\n%d: %s' % (path, e.errno, e.strerror))

        try:
            data = yaml.safe_load(data)
        except yaml.YAMLError as e:
            solution = 'Check the YAML syntax.'

            if hasattr(e, 'problem_mark'):
                if e.context is not None:
                    solution += '\nRuntime info:\n%s\n  %s %s' % (
                        e.problem_mark, e.problem, e.context)
                else:
                    solution += '\nRuntime info:\n%s\n  %s' % (e.problem_mark, e.problem)

            raise UserError(name='Config/ParseError',
                            description='Failed to parse the data provided in the config file.',
                            solution=solution)

        # We can only load a dictionary.
        if not isinstance(data, dict):
            raise UserError(name='Config/ParseError',
                            description='Failed to parse the data provided in the config file.',
                            solution='Verify that the file is not empty.')

        self.load_dict(data)


# Create a global config instance
_config = Config()

# Copy over the interface methods
define = _config.define
is_defined = _config.is_defined
get = _config.get
load_dict = _config.load_dict
load_yaml_file = _config.load_yaml_file
