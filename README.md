# Ah5

Ah5 (Asynchronous HDF5) is a library enabling the use of a dedicated thread to
write HDF5 files in an asynchronous fashion.

## Getting the source

### Getting a release

The list of all Ah5 releases is available at
https://gitlab.maisondelasimulation.fr/jbigot/ah5/tags

The current Ah5 release (0.1.2) is available at
https://gitlab.maisondelasimulation.fr/jbigot/ah5/repository/archive.zip?ref=v0.1.2

### Getting the latest from git

The git repository address to clone Ah5 is:
https://gitlab.maisondelasimulation.fr/jbigot/ah5.git

Ah5 uses a git submodule for BPP, you therefore need to use the `--recursive`
option when cloning.
```bash
$ git clone --recursive https://gitlab.maisondelasimulation.fr/jbigot/ah5.git
```

If you have already cloned Ah5 without the submodules, you can get them with
```bash
$ git submodule init
$ git submodule update
```

## Dependancies

To use Ah5, you need:
* HDF5
* pthread (Available by default on most if not all Unix-like systems)
* OpenMP (Optional)

In addition, to compile Ah5, you need:
* cmake, version 3.0 minimum
* BPP (packed in Ah5)

## Usage

There are two ways you can use Ah5 from your project:
* **In-place**: you can include Ah5 in your project and use it directly from
   there,
* **Dependancy**: or you can use Ah5 as an external dependancy of your
   project.

Support is provided for using Ah5 from Cmake based projects, but you can use
it from plain Makefiles too.

### In-place usage

Using Ah5 in-place is very simple, just use add_subdirectory from cmake. It
provides the targets: `ah5.static`,  `ah5.shared`, `ah5_f90.static` &
`ah5_f90.shared` you can link with.
```cmake
add_subdirectory(path/to/ah5 Ah5)

target_link_libraries(my_exe ah5.static)
```

### Dependancy usage

Using Ah5 as a system-wide dependancy is very simple too, just use `find_package`
from cmake. It provides the targets: `ah5.static`,  `ah5.shared`,
`ah5_f90.static` & `ah5_f90.shared` you can link with.

e.g.
```cmake
find_package(path/to/ah5 Ah5)

target_link_libraries(my_exe ah5.shared)
```

## Installation

In order to install Ah5, you just need to execute the following commands inside
the Ah5 directory:
```bash
$ cmake -DCMAKE_INSTALL_PREFIX=/usr "."
$ make install
```

You can choose the installation path with the `CMAKE_INSTALL_PREFIX` parameter.

## Examples

You can find an example of how to use ah5 in the `examples` directory. If you
want more information about how to use ah5, either in place or installed, you
can take a look at the `test` diectory.