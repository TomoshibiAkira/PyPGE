from setuptools import setup

# Available at setup time due to pyproject.toml
from pybind11.setup_helpers import Pybind11Extension

import sys

__version__ = "0.1.0"
NAME_STRING = "PyPGE"

libraries = []
if sys.platform.startswith('linux'):
    libraries = ['X11', 'GL', 'pthread', 'stdc++fs']


ext_modules = [
    Pybind11Extension(NAME_STRING,
        ["src/PyPGE.cpp"],
        define_macros = [('VERSION_INFO', __version__)],
        libraries=libraries,
        cxx_std=17,
        ),
]

setup(
    name=NAME_STRING,
    version=__version__,
    author="TomoshibiAkira",
    author_email="q491360885@gmail.com",
    url="https://github.com/TomoshibiAkira",
    description="A python wrapper for olcPixelGameEngine",
    long_description="",
    ext_modules=ext_modules,
    zip_safe=False,
)
