import os
import platform
import re
import subprocess
import sys
from distutils.version import LooseVersion

from setuptools import setup, find_packages, Extension
from setuptools.command.build_ext import build_ext


class CMakeExtension(Extension):
    def __init__(self, name, src_dir=''):
        Extension.__init__(self, name, sources=[])

        self.src_dir = os.path.abspath(src_dir)


class CMakeBuild(build_ext):
    def run(self):
        try:
            out = subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError(
                'CMake must be installed to build the following extensions: ' +
                ', '.join(e.name for e in self.extensions))

        if platform.system() == 'Windows':
            cmake_version = LooseVersion(re.search(r'version\s*([\d.]+)', out.decode()).group(1))
            if cmake_version < '3.1.0':
                raise RuntimeError('CMake >= 3.1.0 is required on Windows')

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
            build_args += ['--', '/m']
        else:
            cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]
            build_args += ['--', '-j2']

        env = os.environ.copy()
        env['CXXFLAGS'] = '%s -DVERSION_INFO="%s"' % (
            env.get('CXXFLAGS', ''), self.distribution.get_version())
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        subprocess.check_call(['cmake', ext.src_dir] + cmake_args,
                              cwd=self.build_temp, env=env)
        subprocess.check_call(['cmake', '--build', '.'] + build_args,
                              cwd=self.build_temp)
        print()  # Add an empty line for cleaner output.


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
    packages=find_packages(),
    ext_modules=[
        CMakeExtension('ki.dml'),
        CMakeExtension('ki.protocol')
    ],
    cmdclass={
        'build_ext': CMakeBuild
    },
    zip_safe=False,
    setup_requires=['pytest-runner'],
    install_requires=['pyuv>=1.4.0'],
    tests_require=['pytest>=3.0.0']
)
