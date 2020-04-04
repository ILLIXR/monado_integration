
# Monado - XR Runtime (XRT)

> * Promotional homepage: <https://monado.dev>
> * Maintained at <https://gitlab.freedesktop.org/monado/monado>
> * Latest API documentation: <https://monado.pages.freedesktop.org/monado>

Monado is an open source XR runtime delivering immersive experiences such as VR
and AR on mobile, PC/desktop, and any other device
(because gosh darn people
come up with a lot of weird hardware).
Monado aims to be a complete and conforming implementation
of the OpenXR API made by Khronos.
The project currently is being developed for GNU/Linux
and aims to support other operating systems in the near future.
"Monado" has no specific meaning and is just a name.

## Monado source tree

* `src/xrt/include` - headers that define the internal interfaces of Monado.
* `src/xrt/compositor` - code for doing distortion and driving the display hardware of a device.
* `src/xrt/auxiliary` - utilities and other larger components.
* `src/xrt/drivers` - hardware drivers.
* `src/xrt/state_trackers/oxr` - OpenXR API implementation.
* `src/xrt/targets` - glue code and build logic to produce final binaries.
* `src/external` - a small collection of external code and headers.

## Getting Started

Dependencies include:

* [CMake][] 3.13 or newer (Note Ubuntu 18.04 only has 3.10)
* Vulkan headers
* OpenGL headers
* Eigen3
* glslangValidator - Debian/Ubuntu package `glslang-tool`.
* libusb
* libudev
* Video 4 Linux - Debian/Ubuntu package `libv4l-dev`.

Optional (but recommended) dependencies:

* libxcb and xcb-xrandr development packages
* [OpenHMD][] (found using pkg-config)

Truly optional dependencies, useful for some drivers, app support, etc.:

* Doxygen
* Wayland development packages
* Xlib development packages
* libhidapi
* OpenCV
* libuvc
* ffmpeg
* libjpeg

Tested distributions that are fully compatible,
on Intel and AMD graphics:

* Ubuntu 18.10 (18.04 does not work)
* Debian 10 `buster`
  * Up-to-date package lists can be found in our CI config file,
    `.gitlab-ci.yml`

These distributions include recent-enough versions of all the
software to use direct mode,
without using any external, third-party, or backported
package sources.

See also [Status of DRM Leases][drm-lease]
for more details on specific packages, versions, and commits.

Build process is similar to other CMake builds,
so something like the following will build it.

Go into the source directory, create a build directory,
and change into it.

```bash
mkdir build
cd build
```

Then, invoke [CMake to generate a project][cmake-generate].
Feel free to change the build type or generator ("Ninja" is fast and parallel) as you see fit.

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -G "Unix Makefiles"
```

If you plan to install the runtime,
append something like `-DCMAKE_INSTALL_PREFIX=~/.local`
to specify the root of the install directory.
(The default install prefix is `/usr/local`.)

To build, [the generic CMake build commands][cmake-build] below will work on all systems,
though you can manually invoke your build tool (`make`, `ninja`, etc.) if you prefer.
The first command builds the runtime and docs,
and the second, which is optional, installs the runtime under `${CMAKE_INSTALL_PREFIX}`.

```bash
cmake --build .
cmake --build . --target install
```

Alternately, if using Make, the following will build the runtime and docs, then install.
Replace `make` with `ninja` if you used the Ninja generator.

```bash
make
make install
```

Documentation can be browsed by opening `doc/html/index.html` in the build directory in a web browser.

## Getting started using OpenXR with Monado

This implements the [OpenXR][] API,
so to do anything with it, you'll need an application
that uses OpenXR, along with the OpenXR loader.
The OpenXR loader is a glue library that connects OpenXR applications to OpenXR runtimes such as Monado
It determines which runtime to use by reading config file default `/usr/local/share/openxr/0/active_runtime.json`
and processes environment variables such as `XR_RUNTIME_JSON=/usr/share/openxr/0/openxr_monado.json`.
It can also insert OpenXR API Layers without the application or the runtime having to do it.

You can use the `hello_xr` sample provided with the
OpenXR loader and API layers.

The OpenXR loader can be pointed to a runtime json file in a nonstandard location with the environment variable `XR_RUNTIME_JSON`. Example:

```bash
XR_RUNTIME_JSON=~/monado/build/openxr_monado-dev.json ./openxr-example
```

For this reason this runtime creates two manifest files within the build directory:

* `openxr_monado.json` uses a relative path to the runtime, and is intended to be installed with `make install`.
* `openxr_monado_dev.json` uses an absolute path to the runtime in its build directory,
  and is intended to be used for development without installing the runtime.

If Monado has been installed through a distribution package
and provides the "active runtime" file /usr/local/share/openxr/0/active_runtime.json,
then the loader will automatically use Monado when starting any OpenXR application.

If Monado has been compiled in a custom directory like ~/monado/build,
the OpenXR loader can be pointed to the runtime when starting an OpenXR application
by setting the environment variable XR_RUNTIME_JSON to the `openxr_monado_dev.json` manifest
that was generated by the build: see above.

Note that the loader can always find and load the runtime
if the path to the runtime library given in the json manifest is an absolute path,
but if a relative path like `libopenxr_monado.so.0` is given,
then `LD_LIBRARY_PATH` must include the directory that contains `libopenxr_monado.so.0`.
The absolute path in `openxr_monado_dev.json` takes care of this for you.

## Configuring Monado with Illixr

To build Monado with Illixr support, you need to supply a path to the Illixr
header files. Some files require C++ headers of the Illixr common library. To do
that, supply the `ILLIXR_PATH` variable when running cmake:

```
cmake -DILLIXR_PATH=/path/to/illixr /path/to/monado
```

Where `/path/to/illixr` should contain the `common` folder. Note that meson is
not supported yet. You have to use cmake.

The above compilation only uses some header files and in no way statically links
or somehow binds monado to that specific Illixr. You need to configure two
environment variables to specify the Illixr shared library and launch
parameters:

```
export ILLIXR_PATH=/path/to/libillixrrt.so
export ILLIXR_COMP=/path/to/libtimewarp_gl.so:/path/to/libslam1.so:/path/to/other_component.so
```

* `ILLIXR_PATH` should be the path to the shared library build of the Illixr
  runtime.
* `ILLIXR_COMP` should be a list of colon-separated paths to the components'
  shared libraries to be loaded into spindle switchboard.

Note NOT to load any application like `gldemo`, as Monado will be sending calls
from the OpenXR app. The Illixr Monado driver will load itself as an application
component in switchboard automatically.

## Direct mode

Our direct mode code requires a connected HMD to have the `non-desktop` xrandr
property set to 1.
Only the most common HMDs have the needed quirks added to the linux kernel.
Just keep on reading for more info on how to work around that.

If you know that your HMD lacks the quirk you can run this command **before** or
after connecting the HMD and it will have it. Where `HDMI-A-0` is the xrandr
output name where you plug the HMD in.

```bash
xrandr --output HDMI-A-0 --prop --set non-desktop 1
```

You can verify that it stuck with the command.

```bash
xrandr --prop
```

## Coding style and formatting

[clang-format][] is used,
and a `.clang-format` config file is present in the repo
to allow your editor to use them.

To manually apply clang-format to every non-external source file in the tree,
run this command in the source dir with a `sh`-compatible shell
(Git for Windows git-bash should be OK):

```bash
scripts/format-project.sh
```

You can optionally put something like `CLANG_FORMAT=clang-format-7` before that command
if your clang-format binary isn't named `clang-format`.
Note that you'll typically prefer to use something like `git clang-format`
to just re-format your changes, in case version differences in tools result in overall format changes.

[OpenHMD]: https://openhmd.net
[drm-lease]: https://haagch.frickel.club/#!drmlease%2Emd
[OpenXR]: https://khronos.org/openxr
[clang-format]: https://releases.llvm.org/7.0.0/tools/clang/docs/ClangFormat.html
[cmake-build]: https://cmake.org/cmake/help/v3.12/manual/cmake.1.html#build-tool-mode
[cmake-generate]: https://cmake.org/cmake/help/v3.12/manual/cmake.1.html
[CMake]: https://cmake.org

## Contributing, Code of Conduct

See `CONTRIBUTING.md` for details of contribution guidelines.

Please note that this project is released with a Contributor Code of Conduct.
By participating in this project you agree to abide by its terms.

We follow the standard freedesktop.org code of conduct,
available at <https://www.freedesktop.org/wiki/CodeOfConduct/>,
which is based on the [Contributor Covenant](https://www.contributor-covenant.org).

Instances of abusive, harassing, or otherwise unacceptable behavior may be
reported by contacting:

* First-line project contacts:
  * Jakob Bornecrantz <jakob@collabora.com>
  * Ryan Pavlik <ryan.pavlik@collabora.com>
* freedesktop.org contacts: see most recent list at <https://www.freedesktop.org/wiki/CodeOfConduct/>

## Copyright and License for this README.md file

For this file only:

> Copyright 2018-2019 Collabora, Ltd.
> Code of Conduct section: excerpt adapted from the [Contributor Covenant](https://www.contributor-covenant.org), version 1.4.1,
> available at <https://www.contributor-covenant.org/version/1/4/code-of-conduct.html>,
> and from the freedesktop.org-specific version of that code,
> available at <https://www.freedesktop.org/wiki/CodeOfConduct/>
>
>
> SPDX-License-Identifier: CC-BY-4.0
