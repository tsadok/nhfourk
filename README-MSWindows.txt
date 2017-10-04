This README-MSWindows.txt file was last modified by Jonadab the Unsightly one, 2015-Sep-07.
Copyright (C) 2013 Alex Smith, 2015 Nathan Eady.

This README file is licensed under the NetHack General Public License.
See libnethack/dat/license for details.

How to Build NetHack Fourk on Windows:
======================================

In order to compile your own copy of NetHack Fourk, you will need what
developers call a "build chain", i.e., a set of tools for building
software.  Unless you happen to already have a complete build chain
installed (e.g., I suspect Cygwin can probably be made to work, though
I am still in the process of trying to prove it), we believe currently
the easiest way to do this is as follows:


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
 3. Obtain a copy of the latest source.  I used git in WSL and just cloned the repo.
    Be sure you get a version recent enough to include commit a820cf65090, from September 2017.
    Older versions than this will not build on Microsoft Windows.  (Note:  I've cherry-picked
    this commit to the 4.3.0.3 branch as of October 4th.)
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
       to the prebuild folder inside your source tree.
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
    C. mkdir ..\build  [NOTE: this MUST be a new clean entirely-empty directory.]
    D. mkdir ..\install
 6. Actually try to build (still in the Strawberry command line)
    A. cd ..\build
    B. perl ..\nhfourk\aimake -i ..\install --without=server --with=tilecompile --with=playfield_utils \
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
    using this very set of instructions.

