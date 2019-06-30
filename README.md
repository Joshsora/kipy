kipy: Python bindings and extensions for libki
==============================================
![GitHub (pre-)release](https://img.shields.io/github/release-pre/Joshsora/kipy.svg)
![Docker Build Status](https://img.shields.io/docker/build/joshsora/kipy.svg)
![Requires.io](https://img.shields.io/requires/github/Joshsora/kipy.svg)
![GitHub](https://img.shields.io/github/license/Joshsora/kipy.svg)

Docker
------
The easiest way to use kipy is by utilizing our pre-built Docker image:
```
docker pull joshsora/kipy
```

You may also manually build the Docker image:
```
docker build -t kipy .
```

Local Installation
------------------
kipy can also be installed in your local environment:

#### Prerequisites
##### Version Requirements
* Python 3.5.0+
* CMake 3.11.0+
* C++11 supported compiler

##### Initialize Submodules
To initialize the dependency submodules, simply run the following:
```
git submodule update --init --recursive
```

#### Installing
To install kipy in your Python environment, simply run the following:
```
python setup.py install
```

#### Testing
Similarly, run this if you wish to run kipy's unit tests:
```
python setup.py test
```

Authors
-------
* [Joshua Scott](https://github.com/Joshsora/)
* [Caleb Pina](https://github.com/pythonology/)
