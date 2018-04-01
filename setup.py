import os

from setuptools import setup

about = {}
cwd = os.path.abspath(os.path.dirname(__file__))
version_path = os.path.join(cwd, 'ki', '__version__.py')
with open(version_path, 'r', encoding='utf-8') as f:
    exec(f.read(), about)

with open('README.rst', 'r', encoding='utf-8') as f:
    long_description = f.read()

setup(
    name=about['__title__'],
    version=about['__version__'],
    author=about['__author__'],
    description=about['__description__'],
    long_description=long_description,
    packages=['ki'],
    zip_safe=False,
    install_requires=['pyuv>=1.4.0'],
    tests_require=['pytest>=3.5.0']
)
