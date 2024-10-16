# Upstream Repository for CMSC23740 (Autumn 2024)

Upstream repo for individual programming projects and labs

## Configuring the build

The first step is to create a subdirectory for the build.

> `mkdir build`

Then we run **CMake** in the `build` directory.

> `cd build`
> `cmake ..`

There are several options to `cmake` that you might want to specify:

* `-DCS237_VERBOSE_MAKEFILE=ON` Enable verbose makefiles

* `-DCS237_ENABLE_DOXYGEN=ON` Enable doxygen for generating cs237 library
  documentation

* `-DCMAKE_BUILD_TYPE=Release` specifies that a "release" build should
  be generated (the default is a "debug" build).  While a "release"
  build is faster (because there is less error checking), we recommend
  the debug build for its greater runtime error checking.

* `-DCS237_BUILD_LABS=OFF` -- Disable the building of the lab assignments

* `-DCS237_BUILD_PROJS=OFF` -- Disable the building of the projects
