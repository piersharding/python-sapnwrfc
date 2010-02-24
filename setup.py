#! /usr/bin/python2.3

import sys
if sys.version < '2.3':
    print "\n\n   You Must Have Python Version >= 2.3  To Install sapnwrfc \n\n"
    sys.exit(1)


from distutils.core import setup, Extension
from distutils.command import build_ext
from distutils.dep_util import newer_group
from distutils import log
from distutils.spawn import spawn
from types import *
import os, string, re
class my_build_ext(build_ext.build_ext):
    """
    Customise the build process for Linux
    """
    def build_extensions(self):
        print "In my own BUILD_EXTENSIONS...\n"
        build_ext.build_ext.build_extensions(self)

    def build_extension(self, ext):
        sources = ext.sources
        if sources is None or type(sources) not in (ListType, TupleType):
            raise DistutilsSetupError, \
                  ("in 'ext_modules' option (extension '%s'), " +
                   "'sources' must be present and must be " +
                   "a list of source filenames") % ext.name
        sources = list(sources)

        fullname = self.get_ext_fullname(ext.name)
        if self.inplace:
            # ignore build-lib -- put the compiled extension into
            # the source tree along with pure Python modules

            modpath = string.split(fullname, '.')
            package = string.join(modpath[0:-1], '.')
            base = modpath[-1]

            build_py = self.get_finalized_command('build_py')
            package_dir = build_py.get_package_dir(package)
            ext_filename = os.path.join(package_dir,
                                        self.get_ext_filename(base))
        else:
            ext_filename = os.path.join(self.build_lib,
                                        self.get_ext_filename(fullname))
        depends = sources + ext.depends
        if not (self.force or newer_group(depends, ext_filename, 'newer')):
            log.debug("skipping '%s' extension (up-to-date)", ext.name)
            return
        else:
            log.info("building '%s' extension", ext.name)

        # First, scan the sources for SWIG definition files (.i), run
        # SWIG on 'em to create .c files, and modify the sources list
        # accordingly.
        sources = self.swig_sources(sources, ext)

        # Next, compile the source code to object files.

        # XXX not honouring 'define_macros' or 'undef_macros' -- the
        # CCompiler API needs to change to accommodate this, and I
        # want to do one thing at a time!

        # Two possible sources for extra compiler arguments:
        #   - 'extra_compile_args' in Extension object
        #   - CFLAGS environment variable (not particularly
        #     elegant, but people seem to expect it and I
        #     guess it's useful)
        # The environment variable should take precedence, and
        # any sensible compiler will give precedence to later
        # command line args.  Hence we combine them in order:
        extra_args = ext.extra_compile_args or []

        # Insert stop at preprocessor output
        if re.compile("^linux").match(sys.platform):
            extra_args[0:0] = ['-E']
            spawn(['rm', '-rf',  self.build_temp])

        macros = ext.define_macros[:]
        for undef in ext.undef_macros:
            macros.append((undef,))

        #log.info("JUST ABOUT TO COMPILE (V2)...\n")
        objects = self.compiler.compile(sources,
                                        output_dir=self.build_temp,
                                        macros=macros,
                                        include_dirs=ext.include_dirs,
                                        debug=self.debug,
                                        extra_postargs=extra_args,
                                        depends=ext.depends)

        if re.compile("^linux").match(sys.platform):
            log.info("DONE PREPROCESSOR COMPILE...\n")
            log.info( "objects: " + repr(objects) + "\n")
            for obj in objects:
                spawn(['ls', '-latr', obj])
                res = re.compile("^(.*?)\.o$").search(obj)
                obji = res.group(1) + ".i"
                objii = res.group(1) + ".ii"
                log.info( "target: " + objii + "\n")
                spawn(['rm', '-f', objii])
                spawn(['mv', '-f', obj, objii])
                spawn(['ls', '-latr', objii])
                spawn(['rm', '-f', obji])
                spawn(['perl', './tools/u16lit.pl', '-le', objii])
                spawn(['ls', '-latr', obji])
                cmdargs = []
                for cargs in self.compiler.compiler:
                    cmdargs.append(cargs)
                for incdir in self.include_dirs:
                    cmdargs.append("-I" + incdir)
                for incdir in ext.include_dirs:
                    cmdargs.append("-I" + incdir)
                for cflg in extra_args:
                    if cflg != "-E":
                        cmdargs.append(cflg)
                cmdargs.append('-c')
                cmdargs.append(obji)
                cmdargs.append('-o')
                cmdargs.append(obj)
                log.info( "cmdargs: " + repr(cmdargs) + "\n")
                spawn(['rm', '-f', obj])
                spawn(cmdargs)

        # XXX -- this is a Vile HACK!
        #
        # The setup.py script for Python on Unix needs to be able to
        # get this list so it can perform all the clean up needed to
        # avoid keeping object files around when cleaning out a failed
        # build of an extension module.  Since Distutils does not
        # track dependencies, we have to get rid of intermediates to
        # ensure all the intermediates will be properly re-built.
        #
        self._built_objects = objects[:]

        # Now link the object files together into a "shared object" --
        # of course, first we have to figure out all the other things
        # that go into the mix.
        if ext.extra_objects:
            objects.extend(ext.extra_objects)
        extra_args = ext.extra_link_args or []

        # Detect target language, if not provided
        language = ext.language or self.compiler.detect_language(sources)

        self.compiler.link_shared_object(
            objects, ext_filename,
            libraries=self.get_libraries(ext),
            library_dirs=ext.library_dirs,
            runtime_library_dirs=ext.runtime_library_dirs,
            extra_postargs=extra_args,
            export_symbols=self.get_export_symbols(ext),
            debug=self.debug,
            build_temp=self.build_temp,
            target_lang=language)


incdirs = []
libdirs = []
libs = []
if sys.platform=='win32':
    print "selecting win32 libraries...\n"
    incdirs = ['src/rfcsdk/include']
    libdirs = ['src/rfcsdk/lib']
    #libs = ['sapnwrfc', 'sapu16_mt', 'sapucum', 'icudecnumber', 'pthread', 'dl', 'm', 'rt']
    libs = ['sapnwrfc', 'libsapucum']
    macros = [('_LARGEFILE_SOURCE', None), ('SAPwithUNICODE', None), ('_CONSOLE', None), ('WIN32', None), ('SAPonNT', None), ('SAP_PLATFORM_MAKENAME', 'ntintel'), ('UNICODE', None), ('_UNICODE', None)]
    #compile_args = ['-mno-3dnow', '-fno-strict-aliasing', '-pipe', '-fexceptions', '-funsigned-char', '-Wall', '-Wno-uninitialized', '-Wno-long-long', '-Wcast-align']
    compile_args = []
elif re.compile("^linux").match(sys.platform):
    incdirs = ['/usr/sap/nwrfcsdk/include']
    libdirs = ['/usr/sap/nwrfcsdk/lib']
    libs = ['sapnwrfc', 'sapucum', 'icudecnumber', 'pthread', 'dl', 'm', 'rt']
    #macros = [('_LARGEFILE_SOURCE', None), ('_FILE_OFFSET_BITS', '64'), ('SAPwithUNICODE', None), ('SAPonUNIX', None), ('SAP_PLATFORM_MAKENAME', 'linuxx86_64'), ('__NO_MATH_INLINES', None), ('SAPwithTHREADS', None)]
    macros = [('_LARGEFILE_SOURCE', None), ('SAPwithUNICODE', None), ('SAPonUNIX', None), ('__NO_MATH_INLINES', None), ('SAPwithTHREADS', None)]
    compile_args = ['-mno-3dnow', '-fno-strict-aliasing', '-pipe', '-fexceptions', '-funsigned-char', '-Wall', '-Wno-uninitialized', '-Wno-long-long', '-Wcast-align', '-fPIC']
else:
    print "I dont know what to do with your platfor: ", system.platform, "\n"
    print "You MUST edit the setup.py to create a vriant for your environment.\n"
    sys.exit(1)

nwsaprfcutil_ext = Extension('nwsaprfcutil',
                    extra_compile_args = compile_args,
                    include_dirs = incdirs,
                    library_dirs = libdirs,
                    libraries = libs,
                    define_macros = macros,
                    sources = ['src/nwsaprfcutil.c'])

LONG_DESCRIPTION = """\

sapnwrfc - A Python RFC interface to SAP NetWeaver R/3 systems
--------------------------------------------------------------

Copyright (C) Piers Harding 2006 - 2009, All rights reserved

== Summary

Welcome to the sapnwrfc Python module.  
This module is intended to facilitate RFC calls to an SAP R/3 
system of release 4.6x and above.  
It may work for earlier versions but it hasn't been tested.
The fundamental purpose of the production of this package, is
to provide a clean object oriented interface to RFC calls from
within Python.  This will hopefully have a number of effects:
(1) make it really easy to do RFC calls to SAP from Python in 
an object oriented fashion (Doh!)
(2) promote Python as the interface/scripting/glue language of 
choice for interaction with SAP R/3.
(3) make the combination of Linux, Apache, and Python the killer 
app for internet connectivity with SAP.
(4) Establish a small fun open source project that people are 
more than welcome to contribute to, if they so wish.

== Prerequisites:
Tested on Microsoft Windows XP SP3. Vista User Access Control
(UAC) should prompt for UAC elevation if Python was installed 
for all users;not tested.

Please insure that the following software is installed:

Python 2.6:
(Tested with ActivePython 2.6.2.2  
based on Python 2.6.2 (r262:71600, Apr 21 2009, 15:05:37) 
[MSC v.1500 32 bit (Intel)])
This can be found at: http://www.activestate.com/activepython/

PyYAML: (Tested with PyYAML-3.08.win32-py2.6.exe)
This can be found at: http://pyyaml.org/wiki/PyYAML .

== Installation:
Follow the installation wizard.

== Post Installation:
Add the sap nwrfcsdk library path to windows system PATH 
environment variable. The library cannot be distributed 
for licensing reasons. Should be obtained from SAP through
its Service MarketPlace (http://service.sap.com/swdc)


== Bugs:
I appreciate bug reports and patches, just mail me! piers@ompka.net
For issues regarding this windows installer please mail also 
Menelaos Maglis at mmaglis@metacom.gr

sapnwrfc is Copyright (c) 2006 - 2009 Piers Harding.
It is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

A copy of the GNU Lesser General Public License (version 2.1) 
can be found at http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
"""
setup(name = "sapnwrfc", version = "0.10",license = "GLPL v2.1",
      description="SAP NetWeaver R/3 RFC Connector for Python",
      long_description=LONG_DESCRIPTION,
      author="Piers Harding",
      author_email="piers@ompka.net",
      url="http://www.piersharding.com/download/python/sapnwrfc/",
      py_modules = ['sapnwrfc/__init__', 'sapnwrfc/rfc/__init__'],
      ext_modules = [nwsaprfcutil_ext],
      cmdclass = {'build_ext': my_build_ext})
