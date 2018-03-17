
# Metamath in C++

`mmpp` stands for [Metamath](http://us.metamath.org/index.html) in C++. It is a library and a collection of tools for checking, writing and generating mathematical proofs in the Metamath format. For the moment it is very experimental and changing code, and probably also quite buggy! Also, you will probably not able to understand much of it if you do not already know what is Metamath and how it works, so in that case please go to the original Metamath site, which has a lot of nice introductory material.

## Building `mmpp`

At the moment the codes support building on Linux, Windows and macOS. Most of the code is actually standard C++17, with a few multiplatform libraries. The few platform specific code is essentially concentrated in the file `platform.cpp`. If you want to add support for a new platform, you basically have to start from there.

That said, all main development happens on Linux, so in general that is expected to be supported better than the other platforms.

Beside the standard C++17 library, the code depends on the Boost collection of libraries. If you use `webmmpp` you will also need libmicrohttpd and the TypeScript to JavaScript transpiler. If you use the proving routines depending on Z3, you will also need libz3. In any case, you need qmake to generate the build script.

You can customize the build by editing the file `mmpp.pro`. At this point it is not possible to install `mmpp`: you need to run it from the source code checkout.

### Linux

First you need to install the dependencies. On Debian-based systems this is usually as hard as giving the command

    sudo apt-get install git build-essential libz3-dev libmicrohttpd-dev qt5-default libboost-all-dev node-typescript

Then you create a new direcory for the build and run qmake and then make there:

    git clone https://github.com/giomasce/mmpp.git
    cd mmpp
    mkdir build
    cd build
    qmake ..
    make

This will create the main executable `mmpp` in the build directory.

Then, if you want to use `webmmpp`, you need to compile the JavaScript files:

    cd ..  # Return to the source code root
    tsc -p resources/static/ts

### macOS

This is a bit more difficult. Unfortunately I am not a very good Mac user, so there might be a better way to do all of this. Feel free to suggest improvements and consider that all of this was done on macOS Sierra.

First you have to install the Apple C++ compiler. It happened by magic on my computer: I wrote g++ in a console, and the operating system proposed me to install the compiler, and I said yes. For all the other dependencies, I resorted to [Homebrew](https://brew.sh), which you have to install following the instructions on their website. Then you have again a nice environment where you can install things by just writing their name. Open a terminal and give this command:

    brew install z3 libmicrohttpd qt boost typescript

Probably in order not to screw up the system, the qt package does not install binaries in any default path. If you want to use qmake directly, you need to add it manually to your terminal path:

    echo 'export PATH="/usr/local/opt/qt/bin:$PATH"' >> $HOME/.bash_profile

Then you need to close and reopen the terminal and you can use the same commands as Linux:

    git clone https://github.com/giomasce/mmpp.git
    cd mmpp
    mkdir build
    cd build
    qmake ..
    make

And to compile the JavaScript files:

    cd ..
    tsc -p resources/static/ts

### Windows

Here is where things become really complicated! I have really no idea of how Windows developers manage to retain their sanity! It is actually so much complicated that I do not remember anymore what I actually did, so I will just write the basic steps and hope you will manage to find your way. As usual, patches are welcome! (and sorry for the little rants, but I really spent a lot of time on this...)

I used Windows 8. Before I tried a few different copies of Windows 7, but they all failed when installing Microsoft C++ runtime components needed to run Qt Creator. So, not only Microsoft does not deliver standard runtime components for one of the most mainstream languages around, but it even fails at providing a working installer for its own operating system!

First you have to install Microsoft C++ compiler. I installed Visual Studio 2017 from Microsoft's website, executing the installer a lot of times to find the components that I actually needed. Then I installed Qt Creator from Qt's website, again having a few rounds to find the right components. By the way, one could also try with MinGW, but I do not think it would make anything easier. Maybe even more difficult. Also, in theory you do not really need Qt Creator (you just need to install the Qt components to have qmake), but in practice having it spares you from having to hand craft the correct execution path to be able to call qmake and the C++ compiler in the same terminal.

Then you need to install the dependencies. For libmicrohttpd and libz3 this is comparatively easy: you just go to the website, download the binary package that some gentle soul has made you available and you copy its content in `c:\libs` (the path is hardcoded in `mmpp.pro`: you can change it if you want; unfortunately I do not know of a standard location for header and libraries on Windows). But then you have to install Boost, and there the real pain begins: you need to follow the instructions on the website and hope it will work. Install the library in `c:\boost` (or, again, fix the path hardcoded in `mmpp.pro`).

Finally, you need to install the TypeScript transpiler. You can install the Node package manager from the Visual Studio installer, then open a console and use:

    npm install typescript

Ok, you are finally at the point where you can actually compile `mmpp`.

TODO

# Preparing theory data

For doing nearly anything with `mmpp` you will need a `set.mm` theory file to work with. While some parts of `mmpp` support generic Metamath files, most of it is designed to work with `set.mm`. Actually, many algorithms actually require some specific theorems that are not (yet) available in the standard `set.mm`, so I suggest to use my personal fork, which you can find in https://github.com/giomasce/set.mm, in the `develop` branch. You have to download it and put it in the `resources` directory, retaining the name `set.mm`. The first run of `mmpp` will take some time to generate the cache for the LR parser and save it in the file `set.mm.cache`. Each subsequent run will reuse the same cache file, so the startup time will be much quicker (unless you modify `set.mm` in a way that invalidates the cache: in such case `mmpp` will automatically detect the need of rebuilding the cache, and this will take some time again).

# Running `mmpp`

There are many different subcommands that you can execute when you run `mmpp`. If you run `mmpp` without any subcommand or with the subcommand `help`, it will list all the available subcomamnds. Most of them are actually experimental or do nothing useful. Here I present only those that can be at least something helpful to a casual user. Some subcommands might not be available if you disabled its dependencies in `mmpp.pro` before building.

## Webmmpp

TODO

## Unificator

TODO

## Generalizable theorems

TODO

## Substitution rules searcher

TODO

## Resolver

TODO

# License

The code is copyright © 2016-2018 Giovanni Mascellani <g.mascellani@gmail.com>. It is distributed under the terms of the General Public License, [version 2](https://www.gnu.org/licenses/old-licenses/gpl-2.0.html) or [later](https://www.gnu.org/licenses/old-licenses/gpl-3.0.html).

There are some exceptions: the files in the `libs` directory are external libraries; each of them has a header detailing their licensing status. Also, throughout the code there are small snippets taken from various sources, most notably StackOverflow. They are noted by comments that indicate their origin and their licensing status.