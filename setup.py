import os
import platform
import re
import subprocess
import sys
from distutils.version import LooseVersion

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext


class CMakeExtension(Extension):
    def __init__(self, name, src_dir=''):
        Extension.__init__(self, name, sources=[])

        self.src_dir = os.path.abspath(src_dir)


class CMakeBuild(build_ext):
    def get_cmake_version(self):
        try:
            out = subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError(
                'CMake must be installed to build the following extensions: ' +
                ', '.join(ext.name for ext in cmake_extensions))
        return re.search(r'version\s*([\d.]+)', out.decode()).group(1)

    def run(self):
        if LooseVersion(self.get_cmake_version()) < '3.1.0':
            raise RuntimeError('CMake >= 3.1.0 is required')

        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        ext_dir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
        cmake_args = ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + ext_dir,
                      '-DPYTHON_EXECUTABLE=' + sys.executable]

        cfg = 'Debug' if self.debug else 'Release'
        build_args = ['--config', cfg]

        if platform.system() == 'Windows':
            cmake_args += ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_%s=%s' % (cfg.upper(), ext_dir)]
            if sys.maxsize > 2**32:
                cmake_args += ['-A', 'x64']
            build_args += ['--', '/m']  # Parallel build
        else:
            cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]
            build_args += ['--', '-j2']  # Parallel build

        build_dir = os.path.abspath(self.build_temp)
        if not os.path.exists(build_dir):
            os.makedirs(build_dir)

        cmake_setup = ['cmake', ext.src_dir] + cmake_args
        subprocess.check_call(cmake_setup, cwd=self.build_temp)

        cmake_build = ['cmake', '--build', '.'] + build_args
        subprocess.check_call(cmake_build, cwd=self.build_temp)


about = {}
setup_dir = os.path.abspath(os.path.dirname(__file__))
version_path = os.path.join(setup_dir, 'ki', '__version__.py')
with open(version_path, 'r', encoding='utf-8') as f:
    exec(f.read(), about)

with open('README.md', 'r', encoding='utf-8') as f:
    long_description = f.read()

setup_requires = ['pytest-runner>=2.0,<3dev']
install_requires = ['ruamel.yaml']
tests_require = ['pytest>=3.0.0']

setup(
    name=about['__title__'],
    version=about['__version__'],
    author=about['__author__'],
    description=about['__description__'],
    long_description=long_description,
    packages=['ki', 'ki.protocol'],
    ext_modules=[CMakeExtension('ki/lib')],
    cmdclass={'build_ext': CMakeBuild},
    zip_safe=False,
    setup_requires=setup_requires,
    install_requires=install_requires,
    tests_require=tests_require
)
