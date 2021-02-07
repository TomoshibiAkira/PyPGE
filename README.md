PyPGE - A Python Wrapper for [olcPixelGameEngine](https://github.com/OneLoneCoder/olcPixelGameEngine)
==============

This is a Python wrapper for [olcPixelGameEngine](https://github.com/OneLoneCoder/olcPixelGameEngine) using [pybind11](https://github.com/pybind/pybind11).

It's used for my personal NES on Python project.

Currently it simply exposes all core classes from PGE as Python classes, which is not optimal performance-wise.

The perfect example is the original PGE [Hello World Example](https://github.com/OneLoneCoder/olcPixelGameEngine/wiki#example-olcpixelengine-hello-world). The extensive call (one call per pixel!) of `Pixel` construction and `Draw` function in the loop serverly impacts the performance on Python. Please see `example/helloworld.py`.

Requirements
------------

* Compiler that supports C++19
* Python 3.5+
* Numpy >= 1.7.0

Installation
------------

 - clone this repo & go into the repo folder
 - `pip install .` or `python setup.py install`

Usage
-----

Please refer to the python scripts under the `example` folder.


License
-------

BSD-3