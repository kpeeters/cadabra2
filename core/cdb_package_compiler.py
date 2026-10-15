##
# \file    cdb_package_compiler.py
# \ingroup pythoncore
# Import hook which makes Cadabra notebooks importable as Python modules.
#
# This module is imported by the `cadabra2` module itself when it gets
# loaded, which calls `install` to register the `PackageCompiler` finder
# on `sys.meta_path`. A plain `import cadabra2` is therefore enough to
# be able to `import` a `.cnb`, `.cdb` or `.ipynb` file.

import sys
import os
from importlib.machinery import ModuleSpec, SourceFileLoader
from importlib.abc import MetaPathFinder
from cdb_appdirs import user_config_dir
from pathlib import Path

# The function converting a notebook to a Python file; set by `install`
# (this is `cadabra2.compile_package__`, but we cannot import `cadabra2`
# here as this module is loaded while `cadabra2` is being initialised).
_compile_package = None

class PackageCompiler(MetaPathFinder):
        @classmethod
        def find_spec(cls, fullname, path, target=None):
                # Top-level import if path=None
                if path is None or path == "":
                        path = sys.path
                # Get unqualified package name
                if '.' in fullname:
                        parents = fullname.split('.')
                        name = parents.pop()
                else:
                        name = fullname
                        parents = []
                # Go through path and try to find a notebook.
                for entry in path:
                   have_cnb = os.path.isfile(os.path.join(entry, name + ".cnb"))
                   have_cdb = os.path.isfile(os.path.join(entry, name + ".cdb"))
                   have_ipynb = os.path.isfile(os.path.join(entry, name + ".ipynb"))
                   if have_cnb or have_cdb or have_ipynb:
                       # Prepend the folder to the `parents`, so that two identically named
                       # notebooks in different folders will not map to the same cached/converted file.
                       path_parts = list( Path(entry).resolve().parts[1:] )
                       parents = path_parts + parents

                       # Notebook was found. Create a version of it in the temporary directory, then
                       # return a ModuleSpec object to allow Python to load that file
                       pkg_path = os.path.join(user_config_dir(), "cadabra_packages", *parents)
                       # Create the path if it doesn't exist
                       if not os.path.exists(pkg_path):
                               os.makedirs(pkg_path)
                       if have_cnb:
                          _compile_package(os.path.join(entry, name + ".cnb"), os.path.join(pkg_path, name + ".py"))
                       elif have_cdb:
                          _compile_package(os.path.join(entry, name + ".cdb"), os.path.join(pkg_path, name + ".py"))
                       else:
                          _compile_package(os.path.join(entry, name + ".ipynb"), os.path.join(pkg_path, name + ".py"))
                       return ModuleSpec(
                                        fullname,
                                        SourceFileLoader(fullname, os.path.join(pkg_path, name + ".py")),
                                        origin=os.path.join(pkg_path, name + ".py"))

                # Return none if no notebook was found
                return None

def install(compile_func):
        """Register the notebook import hook. Prepends to sys.meta_path, so that
        all imports will first be checked in case they are notebooks that need
        compiling. Safe to call more than once."""
        global _compile_package
        _compile_package = compile_func
        if PackageCompiler not in sys.meta_path:
                sys.meta_path.insert(0, PackageCompiler)
