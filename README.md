# Ah5

Ah5 (Asynchronous HDF5) is a library enabling the use of a dedicated thread to
write HDF5 files in an asynchronous fashion.

## Getting the source

There is no release yet, you have to use git to get the source. To do so, use
the --recursive option to clone in order to also download the BPP dependancy.
If you have already cloned AH5 without the submodules, you can get them with
    $ git submodule init
    $ git submodule update

## Usage

There are two ways you can use Ah5 from your project:
*  **In-place**: you can include Ah5 in your project and use it directly from
   there,
*  **Dependancy**: or you can use Ah5 as an external dependancy of your
   project.

Support is provided for using Ah5 from Cmake based projects, but you can use
it from plain Makefiles too.

### In-place usage

Using Ah5 in-place is very simple, just use add_subdirectory from cmake.

### Dependancy usage

Using Ah5 as a dependancy is very simple too, just use find_package from cmake.
You can use a system-wide installed Ah5 or you can just configure Ah5 as a
user, without installation. It should be found anyway.

## Installation

Installing Ah5 is very simple. Inside the Ah5 directory, execute the
following commands:
```
cmake .
make install
```

In order to change the installation path for your project, set the
CMAKE_INSTALL_PREFIX cmake parameter:
```
cmake -DCMAKE_INSTALL_PREFIX=/usr .
```

## Examples

You can find two usage examples in the `test` directory. You can compile them
with the following commands after having installed Ah5.
```
cd tests
cmake .
make
```