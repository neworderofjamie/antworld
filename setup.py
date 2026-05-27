import os
import pkgconfig
import shutil
import sys
from copy import deepcopy
from glob import glob
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
        
abs_antworld_path = os.path.dirname(os.path.abspath(__file__))                            
pyantworld_path = os.path.join(".", "pyantworld")
pyantworld_src = os.path.join(pyantworld_path, "src")
antworld_include = os.path.join(".", "include")

# Always package LibGeNN
if WIN:
    package_data = [f"antworld{lib_suffix}.dll"]
else:
    package_data = [f"libantworld{lib_suffix}.so"]

# Define standard kwargs for building all extensions
antworld_extension_kwargs = {
    "include_dirs": [antworld_include],
    "library_dirs": [pyantworld_path],
    "libraries": [f"antworld{lib_suffix}"],
    "cxx_std": 17,
    "extra_compile_args": [],
    "extra_link_args": [],
    "define_macros": [("LINKING_ANTWORLD_DLL", 1)]}

# If this is Windows
if WIN:
    vcpkg_base = os.path.join(".", "vcpkg_installed", "x64-windows", "x64-windows")
    
    # Add vcpkg include directory
    vcpkg_include = os.path.join(vcpkg_base, "include")
    antworld_extension_kwargs["include_dirs"].extend((vcpkg_include, 
                                                      os.path.join(vcpkg_include, "opencv4")))
    
    # Add vcpkg library directory
    vcpkg_lib = os.path.join(vcpkg_base, *(("debug", "lib") if debug_build 
                                           else ("lib",)))
    antworld_extension_kwargs["library_dirs"].append(vcpkg_lib)

    
    # Link OpenGL as it's used directly in extension
    antworld_extension_kwargs["libraries"].append("OpenGL32")
    
    # Turn off warnings about dll-interface being required for stuff to be
    # used by clients and prevent windows.h exporting TOO many awful macros
    antworld_extension_kwargs["extra_compile_args"].extend(["/wd4251", "/wd4275", "-DWIN32_LEAN_AND_MEAN", "-DNOMINMAX"])

    # Add antworld library to dependencies
    antworld_extension_kwargs["depends"] = [os.path.join(pyantworld_path, "antworld" + lib_suffix + ".dll")]

# Otherwise
else:
    # Loop through pacakges
    for pkg in ["opencv4", "sfml-window", "glew"]:
        # Add whatever configuration libffi requires
        ffi_config = pkgconfig.parse(pkg)
        for k, v in ffi_config.items():
            antworld_extension_kwargs[k].extend(v)

    # Add antworld library to dependencies
    antworld_extension_kwargs["depends"] = [os.path.join(pyantworld_path, "antworld" + lib_suffix + ".so")]
    
    # If this is Linux, we want to add extension directory i.e. $ORIGIN to 
    # runtime directories so librariescan be found wherever package is installed
    if LINUX:
        antworld_extension_kwargs["runtime_library_dirs"] = ["$ORIGIN"]

ext_modules = [
    Pybind11Extension("_antworld",
                      [os.path.join(pyantworld_src, "antworld.cc")],
                      **antworld_extension_kwargs)]

# If we should build required FeNN libraries
if build_antworld_libs:
    # If compiler is MSVC
    if WIN:
        # **NOTE** ensure pygenn_path has trailing slash to make MSVC happy
        out_dir = os.path.join(abs_antworld_path, "pyantworld", "")

        # Find VCPKG bin directory
        abs_vcpkg_bin = os.path.join(abs_antworld_path, "vcpkg_installed", 
                                     "x64-windows", "x64-windows",
                                     *(("debug", "bin") if debug_build 
                                       else ("bin",)))    
        
        # Copy all DLLs
        for d in glob(os.path.join(abs_vcpkg_bin, "*.dll")):
            shutil.copy(d, out_dir)
        
        # Build all dependencies for FeNN backend
        check_call(["msbuild", "antworld.sln", f"/t:antworld",
                    f"/p:Configuration={lib_suffix[1:]}",
                    "/m", "/verbosity:quiet",
                    f"/p:OutDir={out_dir}"],
                    cwd=abs_antworld_path)
    else:
        # Define make arguments
        make_arguments = ["make", "backend", "DYNAMIC=1",
                          f"LIBRARY_DIRECTORY={os.path.join(abs_antworld_path, 'pyantworld')}",
                          f"--jobs={cpu_count(logical=False)}"]
        if debug_build:
            make_arguments.append("DEBUG=1")

        if coverage_build:
            make_arguments.append("COVERAGE=1")

        # Build
        check_call(make_arguments, cwd=abs_antworld_path)

# Read version from txt file
with open(os.path.join(abs_antworld_path, "version.txt")) as version_file:
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
