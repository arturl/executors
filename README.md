executors
=========

Prototype of the Executors proposal for ISO C++

IDE Builds
==========

XCode
-----
```
mkdir build
cd build
cmake -G"Xcode" ../ -B.
```

Visual Studio 12, 13
--------------------
```
mkdir build
cd build
cmake -G"Visual Studio 12" ..\ -B.
```

console builds
==============

OSX
---
```
mkdir build
cd build
cmake -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo -B. ../
make
```

Windows
-------
```
mkdir build
cd build
cmake -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo -B. ..\
nmake
```

The build only produces a test binary.

Running tests
=============

* You can use the CMake test runner ```ctest```
* You can run the test binary directly ```extr_test```
* Tests can be selected by name or tag ```extr_test [time]```
