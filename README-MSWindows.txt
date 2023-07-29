This README-MSWindows.txt file was last modified by Jonadab the Unsightly One, 2017-Nov-16.
Copyright (C) 2013 Alex Smith, 2015 Nathan Eady.

This README file is licensed under the NetHack General Public License.
See libnethack/dat/license for details.


Overview: Methods for Building NetHack Fourk on Windows:
========================================================

In order to compile your own copy of NetHack Fourk, you will need what
developers call a "build chain", i.e., a set of tools for building
software.  We believe currently the easiest way to do this is to use
the first method below, building with Strawberry Perl.

If you are interested in building with Cygwin, see the Building with
Cygwin section, below.  If you are interested in building with WSL,
also see the Building with Cygwin section below, and note that since
WSL cannot run Windows-native code (which will include things like
makedefs.exe, dlb.exe, levcomp.exe, etc. that you will build as part
of the project), you will need to figure out some way to work around
these issues.


Building with Strawberry Perl (Recommended):
============================================

 1. Install Strawberry Perl
    A. Go to http://strawberryperl.com/
    B. Download the latest, at the time of this writing strawberry-perl-5.26.0.2-64bit.msi
    C. When the download completes, open up the folder you downloaded to, right-click the .msi,
       and choose Install.  Follow the instructions (Next, Next, etc.), granting UAC admin permission when needed.
 2. Install flex and bison.
    A. http://gnuwin32.sourceforge.net/
    B. On the left, under Download, click Packages.
    C. Scroll down to Bison and right-click on Setup, choose Open in New Tab.  Save the file.
    D. Scroll down to Flex and right-click on Setup, choose Open in New Tab.  Save the file.
    E. Open the folder you downloaded them to.  Right-click on flex-2.5.4a-1.exe and choose
       Run as Administrator.  Give UAC permission, Next, Accept, Next.
    F. IMPORTANT: when it asks you where to install, it defaults to a path with spaces in it
       (something under C:\Program Files (x86) in all probability).  This is bad and wrong.
       You must change it to something with no spaces, such as C:\GnuWin32
    G. Next, Next, Next, Next, Install.  Finish.
    H. Repeat steps E, F if necessary, and G for the bison-2.4.1-setup.exe that you also
       downloaded.  Step F may not be needed this time, as it may remember from the previous.
    I. Cortana, type "envir".  Wait a couple of seconds.  Under "Settings" it should list
       Edit Environment Variables for Your Account.  Click this.
    J. Path, Edit, Add C:\GnuWin32\bin to the list.  Ok Ok Ok.
    K. Open a cmd.exe window, type PATH (and hit Enter) to verify that your PATH now
       includes that directory.  (It should already include the Strawberry Perl stuff,
       since the Strawberry installer takes care of that.)
 3. Obtain a copy of the latest source.  I used git in WSL and just cloned the repo:
       git clone https://github.com/tsadok/nhfourk.git
    There are also other ways, e.g., the github web interface has a
    feature that will let you download a zipfile containing the
    current tip of any given branch.  In any case, be sure you get a
    version recent enough to include commit a820cf65090, from late
    September 2017.  Older versions than this will not build on
    Microsoft Windows.  (Note:  I've cherry-picked this commit to the
    4.3.0.3 branch as of October 4th.  If you are building master or
    4.3.0.4, you definitely need a copy from October 4th or later.)
 4. Optional: install SDL2 and its development stuff.
    A. Go to the SDL home page at https://www.libsdl.org/
    B. On the sidebar at the left, under Download, click on SDL 2.0
    C. Download the Runtime Binaries: at the time of this writing, SDL2-2.0.5-win32-x64.zip
    D. Also download the MinGW development package, currently SDL2-devel-2.0.5-mingw.tar.gz
    E: Untar the development package somewhere.  (I used WSL and did this:
       cd /mnt/c/Users/Public/Downloads
       tar -xvf SDL2-devel-2.0.5-mingw.tar.gz
    F. Where you untarred, look in x86_64-w64-mingw32/include (assuming you are on amd64;
       use the x86 one if you are using 32-bit Windows for some reason) and find the SDL2
       folder.  Copy it into the Strawberry Perl c/include folder (C:\Strawberry\c\include
       if you installed to C:\Strawberry, which is the default), so that you now have a
       folder called something like C:\Strawberry\c\include\SDL2, which contains a bunch
       of SDL-related header files.
    G. Similarly, where you untarred, look in x86_64-w64-mingw32 (or the 32-bit one if
       you are on 32-bit Windows), and find the lib folder.  It should contain several
       library files, each ending with a .a extension (libSDL2.a, libSDL2.dll.a, libSDL2main.a,
       and libSDL2_test.a).  Copy those to the Strawberry Perl c/lib folder
       (C:\Strawberry\c\lib\ if Strawberry Perl is in the default place).
    H. Open the SDL2 binary runtime zip file that you downloaded.  It should contain a
       README-SDL.txt file, which you can read if you want, and SDL2.dll.  Copy SLD2.dll
       to the prebuilt folder inside your source tree.
 5. Final preparations:
    A. Open Strawberry Perl's command line.
        * Note:  the console window used by Strawberry Perl has a
          tendency to freeze all activity in the window if you click
          on it at the wrong time, or in the wrong way.  I think it
          might also happen sometimes when switching between windows.
          I haven't worked out the exact trigger conditions.  The cure
          is to hit the Enter key a couple of times.  Pay attention to
          this, especially when running aimake.
    B. cd to wherever your source tree lives.  In my case:
       cd C:\Users\Public\Documents\nhfourk
    C. mkdir build  [NOTE: this MUST be a new clean entirely-empty directory.]
    D. mkdir ..\install
 6. Actually build (still in the Strawberry command line)
    A. cd build
    B. perl ..\nhfourk\aimake -i ..\..\install --without=server --with=tilecompile --with=playfield_utils \
                       --with=sourcecode --with=jansson --with=gui --directory-layout=single_directory
 7. Be patient.  aimake may take a long time (and is particularly slow
    on systems with not much physical RAM) but should automatically
    figure out everything and compile NetHack Fourk.  It may show a
    whole bunch of error messages.  This doesn't always mean that it
    is failing.  aimake can often work around errors that occur and
    still produce a working build, even if e.g. ld.exe has crashed
    during the build (it generally does on Windows; fortunately, it
    crashes after producing output that aimake needs and can use to
    proceed to the next step).  If you think the build is failing
    horribly, be patient, wait for it to stop producing error messages
    and return you to the prompt, then go ahead and check the install
    directory for nhfourk.exe and nhfourk-sdl.exe.  The build may
    actually have been successful, despite the errors.  I managed to
    successfully compile from the 4.3.0.3 branch on 2017 October 4th,
    using this very set of instructions, and later the same day got
    the tip of the master branch to compile after making a change to
    aimake.rules that causes NETCLIENT to be defined.
 8. Optional: build the .msi installer.
    A. Make sure your copy of the source is recent enough to have
       commit 9b17a9439a9, from 2017 October 17.
    B. Go to the WiX website, http://wixtoolset.org/
    C. Click on Downloads.  Under Recommended Build, click the download
       button (currently, Download WIX V3.11).  The link goes to github.
    D. On the github release page, scroll to near the bottom, and you
       will find the actual downloads.  I chose wix311.exe
    E. Run the wix installer as administrator, click Install (the middle
       row), wait for it to say Complete, then hit exit.
    F. Delete (or move) the icon files out of dist/common.  If there
       are icon files there, Win32::Exe will fail to add them to the
       executables, and aimake will consider this a fatal failure and
       stop the build process before it completes.
    G. Depending on your setup, if you want to distribute the .msi to
       other folks and have it work, you may need to add additional
       DLLs into the Prebuilt folder, e.g.,  libpng16-16.dll, zlib1__.dll
       If you have them installed, e.g., because you have Cygwin installed,
       aimake will find them and think they are system libraries and assume
       that they therefore don't need to be included (on the theory that
       everyone will have them already; which is not actually true).  
       Putting them in prebuilt fixes this.  I found libpng16-16.dll in 
       C:\cygwin64\usr\i686-w64-mingw32\sys-root\mingw\bin and
       zlib1__.dll in C:\Strawberry\c\bin  If you don't have Cygwin
       installed and don't have libpng16-16.dll in such a location, it
       may be that aimake will not get confused and think it's a system
       library when you build, so you may not need to do anything further
       for that library.  You probably do have the zlib dll, however,
       because you installed Strawberry Perl in order to do this build.
    H. At the Strawberry command prompt,
       SET PATH=%PATH%;"C:\Program Files (x86)\WiX Toolset v3.11\bin"
    I. Make sure you are using a clean, not-previously-used build directory.
       Delete and recreate it if necessary.
    J. perl ..\aimake --without=server --with=gui --with=playfield_utils \
                      --with=tilecompile --with=sourcecode --with=jansson \
                      --gen-installer=msi
 9. Super Optional:  build an .msi installer with the icons on the executables.
    A. Use aimake to do the build as in step 8 first.
    B. Go the the Resource Hacker website, http://www.angusj.com/resourcehacker/
       Download, install.
    C. Open nhfourk-sdl.exe in Resource Hacker
    D. Action->Add from a Resource file.
       Choose an existing .exe file that has an icon.
       Check "Icon Group" and hit import.
    E. Expand Icon.  Right-click on any size of the icon and choose Replace Icon.
       Open file with new icon.  Select fourk.ico and hit Replace.
    F. Save.
    G. Repeat steps B-F for nhfourk.exe if desired.
    H. Copy your shiny new icon-enabled .exe file(s) into
       build\aimake_wix_input\CSIDL_PROGRAM_FILES\NetHack Fourk
    I. candle -arch x64 -nologo aimake_wix_input\nhfourk.wxs
    J. light -nologo -exzt WixUIExtension -sice:ICE38 -sice:ICE43 -sice:ICE57 -sice:ICE64 -sice:ICE90 \
             -dWIXUILicenseRtf="aimake_wix_input\CSIDL_PROGRAM_FILES\NetHack Fourk\doc\license.rtf" \
             aimake_wix_input\nhfourk.wixobj
    K. You should now find an .msi file in the current directory.  If all has gone well, it
       should include the executables with the icon embedded in them.  This is the process I
       used to produce the official .msi release of version 4.3.0.4 of NetHack Fourk.


Building with Cygwin:
=====================

Cross-compiling from Cygwin to native Windows using the mingw
cross-compiler is believed to be possible, using a makefile for Gnu
make, although thus far I have not managed to get it to work,
personally, and so I do not have detailed working tested instructions.
However, see the mingw-build branch, which is based on work by Vincent
Ho, who seems to have succeeded where I, thus far, have not.  It is
likely also be worth checking out his git repository.  To add this as
a remote on an existing NetHack Fourk repository, do this:
  git remote add vince https://github.com/vinceho/nhfourk.git
  git checkout vince/mingw-build
If you just want a separate repository, you can do this instead:
  git clone https://github.com/vinceho/nhfourk.git
  git checkout origin/mingw-build

It may also be possible to get aimake to work under Cygwin, either
with the aforementioned mingw compiler (to create a native Windows
build) or possibly with standard Cygwin build tools (if you just want
to be able to run the game inside of Cygwin).  Again, I haven't yet
managed to demonstrate that this is possible, so I don't have complete
detailed working instructions.


Building with Windows Services for Linux (WSL / "Ubuntu on Ten"):
=================================================================

Experiments in getting the build to work under WSL have reached the
point where I can get a working makedefs.exe, which can be run from a
cmd.exe prompt and works (though the Makefile cannot call this
directly from within WSL, because it's a native Windows .exe, and
unlike Cygwin, WSL doesn't have the ability to run native Windows
programs from within the environment).  Efforts to get a complete
working build procedure for this setup are also ongoing.  If you
manage to get it to work, I'd love to hear about it (contact jonadab
either on the Libera.chat IRC network or via email at columbus.rr.com;
in the case of email, be sure to put the word "Fourk" in the subject
line, otherwise my filters won't notice the message and it'll land in
the middle of a spam-filled inbox and not get noticed).


Building with Visual Studio
===========================

I have no idea.

