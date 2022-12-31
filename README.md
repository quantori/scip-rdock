# rDOCK

rDock is a fast and versatile Open Source docking program that can be used to dock small molecules against proteins and nucleic acids. 

## Bundle

This fork of rDock adds changes that allow it to run as a part of Quantori DockingFactory. The head project of DockingFactory is [DockingFactory Bundle](https://github.com/quantori/scip-dockingfactory-bundle). Other projects that are also parts of the bundle are:
- [DockingFactory](https://github.com/quantori/scip-dockingfactory)
- [DockingInterface](https://github.com/quantori/scip-dockinginterface)
- [Vina](https://github.com/quantori/scip-vina)
- [QVina 2](https://github.com/quantori/scip-qvina)
- [Smina](https://github.com/quantori/scip-smina)


## Build 

### BACKGROUND

rDock is written in C++ and makes heavy use of the C++ Standard Template Library (STL).
All source code is compiled into a single shared library (libRbt.so).
The executables are light-weight command-line applications linked with libRbt.so.

rDock should compile on the majority of Unix/Linux platforms with little or no
modification. However, only the following combinations have been thoroughly
tested:

openSUSE 32 and 64 bits.
Ubuntu 32 and 64 bits.
*  Both with compiler versions higher than 3.3.


#### PREREQUISITES:

Make sure you have the following packages installed:

*  gcc		GNU C compiler (or your preferred C compiler)
*  g++		GNU C++ compiler (or your preferred C++ compiler)
*  make
*  popt		Command-line argument processing (run-time)
*  popt-devel	Command-line argument processing (compile-time)
*  cppunit		C++ unit testing framework (port of JUnit)
*  cppunit-devel		C++ unit testing framework (port of JUnit)

After installing these dependencies, please follow the following steps:

### BASIC BUILD INSTRUCTIONS:

#### Step 1) BUILD

* Change current directory to build folder:
```
$ cd rDock_2021.1_src/build
```
And run, either:

```
$ make linux-g++           #for 32-bit build with g++ compiler
$ make linux-g++-64        #for 64-bit build with g++ compiler
```

If wish to use a different compiler/architecture combination, or if you wish to
change the compiler flags, see below.

The built libraries and executables are copied to their run-time locations
(../lib and ../bin) as part of Step 1.

#### Step 2) TEST

```
$ make test                #Runs rDock unit tests
```

If the test has succeed, you are done, enjoy using rDock!
Otherwise, please check your dependencies and all the previous commands or go to 
Support Section in the webpage (http://rdock.sourceforge.net) to ask for help.

#### Step 3) INSTALL

You can either run rDock directly from the build location initial testing), or 
install the binaries and data files to a new location.

Set the necessary environmental variables for running rDock in the command line.
(for example, in a bash shell):

```
$ export RBT_ROOT=/path/to/rDock/installation/
$ export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$RBT_ROOT/lib
$ export PATH=$PATH:$RBT_ROOT/bin
```

#### OTHER MAKE TARGETS

`make src_dist` 		Creates source distribution

`make clean`              Removes all intermediate build files

`make distclean`         Also removes installed libs and exes in ../lib and ../bin


## License

rDock is released under [GNU Lesser General Public License, Version 3](license.txt)

