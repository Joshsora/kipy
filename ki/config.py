import collections
import json
import logging
import sys
import textwrap

from ruamel import yaml

from .error import ErrorCodeEnum, format_error_message


class ConfigError(ErrorCodeEnum):
    INTERNAL = object()
    FILE_ERROR = object()
    INVALID_DATA_TYPE = object()
    INVALID_DATA = object()
    MISSING_DATA = object()
    INVALID_CHILD = object()
    INVALID_PATH = object()


class ConfigNode(object):
    logger = logging.getLogger('CONFIG')

    DOC_INDENT_TOKEN = ' |  '
    DOC_DESCRIPTION_PREFIX = 'description : '

    def __init__(self, name=None, description=None):
        self.name = name
        self.description = description

        self._parent = None

    def __repr__(self):
        return '%s<%s>' % (self.__class__.__name__, self.path)

    @property
    def parent(self):
        return self._parent

    @parent.setter
    def parent(self, value):
        # If we are a child, we require a name.
        if value is not None and self.name is None:
            error_message = format_error_message(
                code=ConfigError.INVALID_CHILD,
                reason='A child cannot be defined without a name.',
                recommendation='Contact the developers.')
            self.logger.error(error_message)
            sys.exit(ConfigError.INVALID_CHILD)

        self._parent = value

    @property
    def path(self):
        """Returns the config path.

        Example: node-a/node-b/node-c
        """
        if self.parent is not None and self.parent.path is not None:
            return '%s/%s' % (self.parent.path, self.name)
        return self.name

    def _write_doc_indent(self, stream=sys.stdout, indent_level=0):
        stream.write(self.DOC_INDENT_TOKEN * indent_level)

    def _write_doc_name(self, stream=sys.stdout, indent_level=0):
        # Write this object's representation.
        self._write_doc_indent(stream=stream, indent_level=indent_level)
        stream.write('%r\n' % self)

    def _write_doc_description(self, stream=sys.stdout, wrap_width=70, indent_level=0):
        # Write the text-wrapped description.
        self._write_doc_indent(stream=stream, indent_level=indent_level)
        lines = textwrap.wrap(self.description, width=wrap_width)
        stream.write('%s%s\n' % (self.DOC_DESCRIPTION_PREFIX, lines.pop(0)))
        for line in lines:
            self._write_doc_indent(stream=stream, indent_level=indent_level)
            stream.write(' ' * len(self.DOC_DESCRIPTION_PREFIX))
            stream.write('%s\n' % line)

    def write_doc(self, stream=sys.stdout, wrap_width=70, indent_level=0):
        """Writes this node's documentation to the given stream."""
        self._write_doc_name(stream=stream, indent_level=indent_level)
        indent_level += 1
        self._write_doc_description(stream=stream, wrap_width=wrap_width,
                                    indent_level=indent_level)


class ConfigVar(ConfigNode):
    DOC_TYPE_PREFIX = 'type : '
    DOC_DEFAULT_PREFIX = 'default : '

    def __init__(self, name=None, description=None, type=None, default=None,
                 constraint=None):
        super().__init__(name=name, description=description)

        self.type = type
        self.default = default
        self.constraint = constraint

        # When both specified, `default` must be an instance of `type`.
        if self.type is not None and self.default is not None:
            if not isinstance(self.default, self.type):
                error_message = format_error_message(
                    code=ConfigError.INVALID_DATA_TYPE,
                    reason='Invalid default value given for %r: %r' %
                           (self, self.default),
                    recommendation='Contact the developers.')
                self.logger.error(error_message)
                sys.exit(ConfigError.INVALID_DATA_TYPE)

        # When both specified, `default` must meet the `constraint` requirements.
        if self.default is not None and self.constraint is not None:
            if not self.constraint(self.default):
                error_message = format_error_message(
                    code=ConfigError.INVALID_DATA,
                    reason='Invalid default value given for %r: %r' %
                           (self, self.default),
                    recommendation='Contact the developers.')
                self.logger.error(error_message)
                sys.exit(ConfigError.INVALID_DATA)

        self._value = None

    @property
    def value(self):
        """Returns the current value.

        If none has been set, the default will be used.
        """
        if self._value is not None:
            return self._value
        return self.default

    @value.setter
    def value(self, value):
        """Sets the current value.

        The value will be checked against the required type, and
        constraint.
        """
        # Verify that the type is correct.
        if self.type is not None:
            if not isinstance(value, self.type):
                error_message = format_error_message(
                    code=ConfigError.INVALID_DATA,
                    reason='Invalid value given for %r: %r' % (self, value),
                    recommendation='Verify that the value given is the '
                                   'correct type.')
                self.logger.error(error_message)
                sys.exit(ConfigError.INVALID_DATA)

        # Check the value against our constraint.
        if self.constraint is not None:
            if not self.constraint(value):
                error_message = format_error_message(
                    code=ConfigError.INVALID_DATA,
                    reason='Invalid value given for %r: %r' % (self, value),
                    recommendation='Verify that the value given meets the '
                                   'requirements.')
                self.logger.error(error_message)
                sys.exit(ConfigError.INVALID_DATA)

        # We're all good. Go ahead and set the current value.
        self._value = value

    def _write_doc_type(self, stream=sys.stdout, indent_level=0):
        if self.type is not None:
            self._write_doc_indent(stream=stream, indent_level=indent_level)

            # Use `type.__name__` if possible, otherwise use its
            # representation.
            type_name = getattr(self.type, '__name__')
            if type_name is None:
                type_name = repr(self.type)

            stream.write('%s%s\n' % (self.DOC_TYPE_PREFIX, type_name))

    def _write_doc_default(self, stream=sys.stdout, indent_level=0):
        if self.default is not None:
            self._write_doc_indent(stream=stream, indent_level=indent_level)
            stream.write('%s%r\n' % (self.DOC_DEFAULT_PREFIX, self.default))

    def write_doc(self, stream=sys.stdout, wrap_width=70, indent_level=0):
        """Writes this node's documentation to the given stream."""
        super().write_doc(stream=stream, wrap_width=wrap_width,
                          indent_level=indent_level)

        indent_level += 1
        self._write_doc_type(stream=stream, indent_level=indent_level)
        self._write_doc_default(stream=stream, indent_level=indent_level)


class ConfigCategory(ConfigNode):
    def __init__(self, name=None, description=None):
        super().__init__(name=name, description=description)

        self.categories = {}
        self.vars = {}

    def __getitem__(self, key):
        return self.vars[key].value

    def add_category(self, category):
        """Adds the given category to this category as a child."""
        if not isinstance(category, ConfigCategory):
            error_message = format_error_message(
                code=ConfigError.INTERNAL,
                reason='Failed to add category: %r' % category,
                recommendation='Contact the developers.')
            self.logger.error(error_message)
            sys.exit(ConfigError.INTERNAL)
        category.parent = self
        self.categories[category.name] = category

    def add_var(self, var):
        """Adds the given variable to this category."""
        if not isinstance(var, ConfigVar):
            error_message = format_error_message(
                code=ConfigError.INTERNAL,
                reason='Failed to add variable: %r' % var,
                recommendation='Contact the developers.')
            self.logger.error(error_message)
            sys.exit(ConfigError.INTERNAL)
        var.parent = self
        self.vars[var.name] = var

    def define_category(self, name, description=None):
        """Defines a new child category."""
        category = ConfigCategory(name=name, description=description)
        self.add_category(category)
        return category

    def define_var(self, name, description=None, type=None, default=None, constraint=None):
        """Defines a new variable in this category."""
        var = ConfigVar(name=name, description=description, type=type,
                        default=default, constraint=constraint)
        self.add_var(var)
        return var

    def get(self, path):
        """Finds and returns the value of a variable using the given path.

        The given path should be relative to this category.
        """
        category_names = path.split('/')
        var_name = category_names.pop()

        # Find the category.
        category = self
        for category_name in category_names:
            try:
                category = category.categories[category_name]
            except KeyError:
                error_message = format_error_message(
                    code=ConfigError.INVALID_PATH,
                    reason='No variable exists with the path: %r' % path,
                    recommendation='Contact the developers.')
                self.logger.error(error_message)
                sys.exit(ConfigError.INVALID_PATH)

        # Find the variable, and return its value.
        try:
            return category[var_name]
        except (KeyError, AttributeError):
            error_message = format_error_message(
                code=ConfigError.INVALID_PATH,
                reason='No variable exists with the path: %r' % path,
                recommendation='Contact the developers.')
            self.logger.error(error_message)
            sys.exit(ConfigError.INVALID_PATH)

    def load_dict(self, data):
        """Loads values from the given dictionary.

        All of the variables belonging to this category, and each child
        categories will be set.

        Returns any missing data in the form of:
            (missing_categories, missing_vars)
        """
        missing_categories = []
        missing_vars = []

        # Load the variable values.
        for name, var in self.vars.items():
            try:
                var.value = data[name]
            except KeyError:
                # If this variable has no default value, then mark it as
                # missing.
                if var.default is None:
                    missing_vars.append(var)

        # Recursively load our child categories.
        for name, category in self.categories.items():
            category_data = data.get(name)
            if not isinstance(category_data, dict):
                # This category is either empty or invalid -- let's check and
                # see if it contains any required variables.
                for _name, _var in category.vars.items():
                    # If this variable has no default value, then mark it as
                    # missing.
                    if _var.default is None:
                        missing_vars.append(_var)
            else:
                # Load the category.
                # If this category is found to be missing data, then add it to
                # our missing data for the final report.
                _missing_categories, _missing_vars = category.load_dict(category_data)
                missing_categories.extend(_missing_categories)
                missing_vars.extend(_missing_vars)

        return missing_categories, missing_vars

    def load_yaml_file(self, filename):
        """Loads values from the given YAML file.

        All of the variables belonging to this category, and each child
        category will be set.
        """
        try:
            with open(filename, 'r') as f:
                data = f.read()
        except IOError:
            error_message = format_error_message(
                code=ConfigError.FILE_ERROR,
                reason='Failed to open file: %r' % filename,
                recommendation='Verify that this file exists.\n\t'
                               'Verify that this file is not currently in use.')
            self.logger.error(error_message)
            sys.exit(ConfigError.FILE_ERROR)

        try:
            data = yaml.safe_load(data)

            # We can only load a dictionary.
            if not isinstance(data, dict):
                raise TypeError
        except (yaml.YAMLError, TypeError):
            error_message = format_error_message(
                code=ConfigError.INVALID_DATA,
                reason='Failed to load config data.',
                recommendation='Verify that your config file is properly '
                               'formatted.')
            self.logger.error(error_message)
            sys.exit(ConfigError.INVALID_DATA)

        missing_categories, missing_vars = self.load_dict(data)
        if missing_categories or missing_vars:
            error_reason = 'Failed to load config data.'

            # Add the missing categories to the error reason.
            if missing_categories:
                error_reason += '\nMissing categories:\n'
                for category in missing_categories:
                    error_reason += '\t%r\n' % category

            # Add the missing variables to the error reason.
            if missing_vars:
                error_reason += '\nMissing variables:\n'
                for var in missing_vars:
                    error_reason += '\t%r\n' % var

            error_message = format_error_message(
                code=ConfigError.MISSING_DATA,
                reason=error_reason,
                recommendation='Verify that these values exist in your config '
                               'file.')
            self.logger.error(error_message)
            sys.exit(ConfigError.MISSING_DATA)

    def _write_doc_vars(self, stream=sys.stdout, wrap_width=70, indent_level=0):
        for var in self.vars.values():
            self._write_doc_indent(stream=stream, indent_level=indent_level)
            stream.write('\n')
            var.write_doc(stream=stream, wrap_width=wrap_width,
                          indent_level=indent_level)

    def _write_doc_categories(self, stream=sys.stdout, wrap_width=70, indent_level=0):
        for category in self.categories.values():
            self._write_doc_indent(stream=stream, indent_level=indent_level)
            stream.write('\n')
            category.write_doc(stream=stream, wrap_width=wrap_width,
                               indent_level=indent_level)

    def write_doc(self, stream=sys.stdout, wrap_width=70, indent_level=0):
        """Writes this node's documentation to the given stream."""
        super().write_doc(stream=stream, wrap_width=wrap_width,
                          indent_level=indent_level)

        indent_level += 1
        self._write_doc_vars(stream=stream, wrap_width=wrap_width,
                             indent_level=indent_level)
        self._write_doc_categories(stream=stream, wrap_width=wrap_width,
                                   indent_level=indent_level)


class Config(ConfigCategory):
    def __repr__(self):
        return '%s<root>' % self.__class__.__name__

    def write_doc(self, stream=sys.stdout, wrap_width=70, indent_level=0):
        """Writes this node's documentation to the given stream."""
        for var in self.vars.values():
            var.write_doc(stream=stream, wrap_width=wrap_width,
                          indent_level=indent_level)
            stream.write('\n')

        for category in self.categories.values():
            category.write_doc(stream=stream, wrap_width=wrap_width,
                               indent_level=indent_level)
            stream.write('\n')
