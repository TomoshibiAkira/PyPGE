PyPGE - A Python Wrapper for [olcPixelGameEngine](https://github.com/OneLoneCoder/olcPixelGameEngine)
==============

This is a Python wrapper for [olcPixelGameEngine](https://github.com/OneLoneCoder/olcPixelGameEngine) using [pybind11](https://github.com/pybind/pybind11).

It's used for my personal NES on Python project.

Performance Caveats (and how to get rid of them)
------------------------------------------------

Currently it simply exposes all core classes from PGE as Python classes, which is not optimal performance-wise.

The perfect example is the original PGE [Hello World Example](https://github.com/OneLoneCoder/olcPixelGameEngine/wiki#example-olcpixelengine-hello-world). The extensive call (one call per pixel!) of `Pixel` construction and `Draw` function in the loop serverly impacts the performance of PGE on Python. You can find the comparison in `example/helloworld.py`.

As the main bottleneck lies at the Python loop, the simpliest way to eliminate them is to write some new bindings that support batch operations. Since pybind11 supports Numpy pretty well, parameters from the original PGE functions like positions and sizes could be directly saved as `np.ndarray` and passed into the bindings. `PGE::PyDrawArea` in the `PyPGE.cpp` is a good example to solve the performance problem from the Hello World example. If the `numpy` dependency is not something you like, `PGE::PyDrawArea` also supports `bytearray` and `bytes` objects. It is also faster than the original `olc::DrawSprite` (see `helloworld.py` for more info).

Debugging
---------

The `olcPixelGameEngine` uses `std::thread` for the game engine thread to handle frame updates / user inputs, etc. This would mean that any exception in that thread (e.g. some assertion in your python `OnUserUpdate()` function) CANNOT be propagated back to the main thread, and the whole program would hang without giving any information about the exception. This makes debugging the program a headache.

One way to workaround is to handle the exception in the `OnUserUpdate()` function a.k.a. wrapping the whole function body in a try/except clause and print out exception exclusively (see `BaseExample` in `helloworld.py`).

Requirements
------------

* Compiler that supports C++17
* Python 3.5+
* pybind11 >= 2.7.0
* Numpy >= 1.7.0 (only used in examples)
* [Ubuntu] `libx11-dev libglu1-mesa-dev`

Installation
------------

- clone this repo & go into the repo folder
- `pip install .` or `python setup.py install`

Usage (Examples)
----------------

* `examples/helloworld.py`: PGE [Hello World Example](https://github.com/OneLoneCoder/olcPixelGameEngine/wiki#example-olcpixelengine-hello-world).
* `examples/linecircle.py`: Randomly draw lines and spheres on the screen.
* `examples/breakout.py`: Breakout clone from [PGE's TUTORIAL](https://github.com/OneLoneCoder/olcPixelGameEngine/wiki). WIP.
    * Please run this script inside the `examples` folder otherwise the tiles cannot be loaded.

License
-------

* BSD-3
* `olcPixelGameEngine.h`: OLC-3