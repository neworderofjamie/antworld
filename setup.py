import os
import pkgconfig
import sys
from copy import deepcopy
from platform import system, uname
from psutil import cpu_count
from subprocess import check_call
from pybind11.setup_helpers import Pybind11Extension, build_ext, WIN, MACOS
from setuptools import find_packages, setup

# Loop through command line arguments
debug_build = False
coverage_build = False
build_antworld_libs = True
filtered_args = []
for arg in sys.argv:
    if arg == "--debug":
        debug_build = True
    elif arg == "--coverage":
        coverage_build = True
        continue
    elif arg in ["clean", "egg_info", "sdist"]:
        build_antworld_libs = False

    filtered_args.append(arg)

# Add filtered (those that setuptools will understand) back to sys.argv
sys.argv = filtered_args

# Are we on Linux?
# **NOTE** Pybind11Extension provides WIN and MAC
LINUX = system() == "Linux"

# Determine correct suffix for FeNN libraries
if WIN:
    lib_suffix = "_Debug_DLL" if debug_build else "_Release_DLL"
else:
    if debug_build:
        lib_suffix = "_dynamic_debug"
    else:
        lib_suffix = "_dynamic"

abs_fenn_path = os.path.dirname(os.path.abspath(__file__))
fenn_path = os.path.dirname(os.path.abspath(__file__))

antworld_path = os.path.join(".", "antworld")
antworld_src = os.path.join(antworld_path, "src")
antworld_include = os.path.join(antworld_path, "include")

# Always package LibGeNN
if WIN:
    package_data = [f"antworld{lib_suffix}.dll"]
else:
    package_data = [f"libantworld{lib_suffix}.so"]

# Define standard kwargs for building all extensions
antworld_extension_kwargs = {
    "include_dirs": [antworld_include],
    "library_dirs": [antworld_path],
    "libraries": [f"genn{lib_suffix}"],
    "cxx_std": 17,
    "extra_compile_args": [],
    "extra_link_args": [],
    "define_macros": [("LINKING_ANTWORLD_DLL", 1)]}

# If this is Windows
if WIN:
    # Turn off warnings about dll-interface being required for stuff to be
    # used by clients and prevent windows.h exporting TOO many awful macros
    antworld_extension_kwargs["extra_compile_args"].extend(["/wd4251", "/wd4275", "-DWIN32_LEAN_AND_MEAN", "-DNOMINMAX"])

    # Add antworld library to dependencies
    fenn_extension_kwargs["depends"] = [os.path.join(antworld_path, "antworld" + lib_suffix + ".dll")]
    # Add FeNN libraries to extension
    fenn_extension_kwargs["depends"].extend(
        os.path.join(pyfenn_path, f"{l}{lib_suffix}.dll") 
        for l in fenn_libraries)
# Otherwise
else:
    # Add whatever configuration libffi requires
    ffi_config = pkgconfig.parse("libffi")
    for k, v in ffi_config.items():
        fenn_extension_kwargs[k].extend(v)

    # Add GeNN library to dependencies
    fenn_extension_kwargs["depends"] = [os.path.join(pyfenn_path, "libgenn" + lib_suffix + ".so"),
                                        os.path.join(pyfenn_path, "docStrings.h")]
    
    # Add FeNN libraries to extension
    fenn_extension_kwargs["depends"].extend(
        os.path.join(pyfenn_path, f"lib{l}{lib_suffix}.so") 
        for l in fenn_libraries)

    # If this is Linux, we want to add extension directory i.e. $ORIGIN to runtime
    # directories so libGeNN and backends can be found wherever package is installed
    if LINUX:
        fenn_extension_kwargs["runtime_library_dirs"] = ["$ORIGIN"]

ext_modules = [
    Pybind11Extension("_fenn",
                      [os.path.join(pyfenn_src, "fenn.cc")],
                      **fenn_extension_kwargs)]

# If we should build required FeNN libraries
if build_antworld_libs:
    # If compiler is MSVC
    if WIN:
        # **NOTE** ensure pygenn_path has trailing slash to make MSVC happy
        out_dir = os.path.join(abs_fenn_path, "pyfenn", "")

        # Build all dependencies for FeNN backend
        check_call(["msbuild", "riscv_ise.sln", f"/t:backend",
                    f"/p:Configuration={lib_suffix[1:]}",
                    "/m", "/verbosity:quiet",
                    f"/p:OutDir={out_dir}"],
                    cwd=abs_fenn_path)
    else:
        # Define make arguments
        make_arguments = ["make", "backend", "DYNAMIC=1",
                          f"LIBRARY_DIRECTORY={os.path.join(abs_fenn_path, 'pyfenn')}",
                          f"--jobs={cpu_count(logical=False)}"]
        if debug_build:
            make_arguments.append("DEBUG=1")

        if coverage_build:
            make_arguments.append("COVERAGE=1")

        # Build
        check_call(make_arguments, cwd=abs_fenn_path)
# Read version from txt file
with open(os.path.join(abs_fenn_path, "version.txt")) as version_file:
    version = version_file.read().strip()

setup(
    name="pyantworld",
    version=version,
    packages = find_packages(),
    package_data={"pyantworld": package_data},

    url="https://github.com/neworderofjamie/antworld/",
    ext_package="pyantworld",
    ext_modules=ext_modules,
    zip_safe=False,
    python_requires=">=3.8",
    install_requires=["numpy>=1.17", "psutil",
                      "setuptools>=61"])
