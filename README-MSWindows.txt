This README-MSWindows.txt file was last modified by Jonadab the Unsightly one, 2015-Sep-07.
Copyright (C) 2013 Alex Smith, 2015 Nathan Eady.

This README file is licensed under the NetHack General Public License.
See libnethack/dat/license for details.

How to Build NetHack Fourk on Windows:
======================================

Official releases of NetHack Fourk are made available in pre-compiled
binary form for Windows x64.  For 32-bit Windows, currently, you will
need to compile your own, because the developer doesn't have a 32-bit
Windows system to build on.  (Sorry about that.)

In order to compile your own copy of NetHack Fourk, you will need what
developers call a "build chain", i.e., a set of tools for building
software.  Unless you happen to already have a complete build chain
installed (e.g., I suspect Cygwin would probably work), we believe
currently the easiest way to do this is as follows:

 1. Download and install Strawberry Perl.  In addition to Perl itself,
    which is needed to run the NH4 build system aimake, installing
    Strawberry Perl also gives you a working C compiler set (including
    the preprocessor and linker and so on), make, and several other
    needed build-related tools.  Additionally, the Strawberry Perl
    installer automatically puts the things it installs in your PATH,
    which saves you from having to do that manually.  Recommended.

 2. You also need to install Flex and Bison, and ensure that they
    are in your path.  (These are needed for the level and dungeon
    compilers.)  The versions from GnuWin32 will work, except that
    they don't like to be installed any place with a space in the
    directory path, so you will need to install them in an atypical
    (for modern versions of Windows) place, such as C:\GnuWin32\
    or something like that.  You will need to add this directory
    to your PATH (search for "PATH" in the Control Panel).

 3. If you want graphical tiles support, or if you want to use the
    SDL-based "fake terminal" instead of the regular Windows console,
    you also need the SDL library _and_ development headers.  SDL
    version 2 is required.
    * Download the MinGW version of the development headers and import
      libraries from www.libsdl.org and _also_ download the library
      itself.
    * Copy the  entire include/SDL2 subdirectory  from the appropriate
      processor-dependent  directory of  the SDL  distribution  to the
      c/include folder that was  created when you installed Strawberry
      Perl (so that it becomes c/include/SDL2).
    * Copy all the files lib/*.a from the processor-dependent directory
      of your SDL distribution to the c/include folder that was created
      when you installed Strawberry Perl (so that they become e.g.
      c/lib/libSDL2.a and so on).
    * Copy the file SDL2.dll that you obtained when downloading the
      SDL library itself to the 'prebuild' folder inside the 'nhfourk'
      folder that contains the NetHack Fourk distribution.

 4. Once you've downloaded and installed all the prerequisites, open up
    the Strawberry Perl command prompt, change to the directory that
    contains the NetHack Fourk distribution, and do this:
    mkdir build
    cd build
    perl ..\nhfourk\aimake -i ..\install --directory-layout=single_directory

    There are other possible options you can pass to aimake, including
    other possible directory layouts that it supports; but detailing all
    the different build options is beyond the scope of this basic guide.
    You can get more information about aimake's options by typing
    perl ..\nhfourk\aimake --documentation

    Of particular interest, you can use --without=gui to tell aimake
    not to try to build SDL support, or --with=tilecompile to tell
    aimake to install the tile compiler for you to use, in case you
    want to make your own tilesets.

 5. aimake will take a long time (depending on your hardware; it is
    particularly slow on systems with not much physical RAM) but should
    automatically figure out everything and compile NetHack Fourk.
    It may show a whole bunch of error messages.  This doesn't always
    mean that it is failing.  aimake can often work around errors that
    occur and still produce a working build, even if e.g. ld.exe has
    crashed during the build.  If you think the build has failed 
    horribly, go ahead and check the install directory for nhfourk.exe
    The build may actually have been successful, despite the errors.
    (If you are building the GUI/tiles support, you will be looking
    for nhfourk-sdl.exe.)

For more details, see the README file.  (It's in Unix format, so you
may need a text editor that can handle Unix line-ending conventions.
Notepad is notoriously bad at this, but PFE or Notepad++ will work.)

