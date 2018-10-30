
# Metamath in C++

`mmpp` stands for [Metamath](http://us.metamath.org/index.html) in
C++. It is a library and a collection of tools for checking, writing
and generating mathematical proofs in the Metamath format. For the
moment it is very experimental and changing code, and probably also
quite buggy! Also, you will probably not able to understand much of it
if you do not already know what is Metamath and how it works, so in
that case please go to the original Metamath site, which has a lot of
nice introductory material.

## Building `mmpp`

At the moment the codes support building on Linux, Windows and
macOS. Most of the code is actually standard C++17, with a few
multiplatform libraries. The few platform specific code is essentially
concentrated in the file `platform.cpp`. If you want to add support
for a new platform, you have to start from there.

That said, all main development happens on Linux, so in general that
is expected to be supported better than the other platforms.

Beside the standard C++ library, the code depends on the Boost
collection of libraries. If you use `webmmpp` you will also need
`libmicrohttpd` and the TypeScript to JavaScript transpiler `tsc`. If
you use the proving routines depending on Z3, you will also need
libz3. In any case, you need `qmake` to generate the build script.

You can customize the build by editing the file `mmpp.pro`. At this
point it is not possible to install `mmpp`: you need to run it from
the source code checkout.

### Linux

First you need to install the dependencies. On Debian-based systems
this is usually as hard as giving this command to a terminal:

    sudo apt-get install git build-essential libz3-dev libmicrohttpd-dev qt5-default libboost-all-dev node-typescript binutils-dev pkg-config

Other Linux distributions will require some similar command, depending
on the distribution package manager. You need GCC version at least 7,
which for Debian and Ubuntu was introduced respectively in buster and
in artful. If your distribution is older than that and you cannot or
do not want to upgrade, then I suggest to either use a virtual machine
or a container, or to use an external repository; Ubuntu users can
benefit from [this
PPA](https://launchpad.net/~jonathonf/+archive/ubuntu/gcc-7.3).

The dependency on `binutils-dev`, which was not mentioned above, is
required only under Linux to create beautiful stack traces when there
is some problem.

Then you create a new direcory for the build and run `qmake` and then
`make` there:

    git clone --recursive https://github.com/giomasce/mmpp.git
    cd mmpp
    mkdir build
    cd build
    qmake ..
    make

This will create the main executable `mmpp` in the build directory.

Then, if you want to use `webmmpp`, you need to compile the JavaScript
files:

    cd ..  # Return to the source code root
    tsc -p resources/static/ts

### macOS

Installing all the dependencies is a bit more complicated here. This
process was tested on pristine macOS Sierra and High Sierra systems. I
have no idea of which level of compatibility to expect for previous
versions.

First you have to install the Apple command line developer tools. The
operating system automatically proposes you to do that if you try to
use the compiler: if you open a terminal and give the command `gcc`, a
dialog will open proposing you to install the command line developers
tools. You just need to click "Install" and follow the dialogs. You do
not need to install the whole XCode.

For all the other dependencies, I used [Homebrew](https://brew.sh):
you simply need to go to their website and copy and paste the
installation command in a terminal, then follow the instructions. With
`brew` installed, things are nearly as simple as with `apt-get`:

    brew install z3 libmicrohttpd qt boost typescript

There is only a small catch: the `qt` package does not install
binaries in any default path. If you want to use `qmake` directly, you
need to add it manually to your terminal path:

    echo 'export PATH="/usr/local/opt/qt/bin:$PATH"' >> $HOME/.bash_profile

Then you need to close and reopen the terminal (to use the new path)
and you can use the same commands as Linux:

    git clone --recursive https://github.com/giomasce/mmpp.git
    cd mmpp
    mkdir build
    cd build
    qmake ..
    make

This will create the main executable `mmpp` in the build directory.

To compile the JavaScript files:

    cd ..  # Return to the source code root
    tsc -p resources/static/ts

### Windows

This is unfortunately more complicated! I find installing things on
Windows really unfriendly, and there is no shortcut like Homebrew on
macOS: you really need a lot of time, manual downloads and clicks. And
at times it is even complicated to understand which are the right
clicks. This is how I managed to get `mmpp` to compile on Windows 8 on
64 bits with Visual Studio Community 2017. I had also tried Windows 7,
but could not get around an error when installing C++ runtime
components. I have not tried any other Windows or Visual Studio
version.

First you have to download [Visual Studio Community
2017](http://www.visualstudio.com/) and run the installer. In the
Visual Studio Installer program you need to install the following
components: "Desktop development with C++", with at least the optional
components "VC++ 2017 v141 toolset" and "Windows 10 SDK"; and "Node.js
development". At the end of this long process you will have to restart
the computer.

Then you have to download the [Qt
environment](https://www.qt.io/download). Go to the website, choose
the open source version and click your way through until you run the
installer. Again select the open source version by not providing any
account detail and install (together with Qt Creator, which is
mandatory) "MSVC 2017 64-bit" for the most recent Qt version available
(5.10.1 at the time of writing). However, avoid the preview versions,
which might be unstable.

Going forth, you need to install
[Git](https://git-scm.com/downloads). Well, technically speaking this
is not really required, but it is comparatively easy and it has some
advantages. As usual go to the website and execute the installer; you
will be asked some strange questions about some behaviour of Git: you
can always leave the default setting. You finally have all the
development tools installed, congratulations! But the path is still
long: we have to install library dependencies.

Let us begin with
[libmicrohttpd](https://www.gnu.org/software/libmicrohttpd/). Go to
the website and download the binary Windows release, which in the form
of a ZIP file. Extract the content of the archive and then copy the
content of the directory `x86_64/VS2017/Release-static` in `c:/libs`
(you will probably need to create the target directory first). Such
directory is hardcoded in `mmpp.pro`, so if you want to choose another
directory you have to update the file. Unfortunately I do not know of
a standard library directory for Windows. So now `c:/libs` should
contain `libmicrohttpd.lib` (the static library) and `microhttpd.h`
(the header file).

Then there is [Z3](https://github.com/Z3Prover/z3/releases). Download
the most recent version for Windows x64. Extract the content of the
subdirectories `bin` and `include` directly in `c:/libs` (i.e., so
that `c:/libs` directly contains `libz3.lib`, `z3.h` and the other
files, not the directories `bin` and `include`).

Finally, you need to install
[Boost](http://www.boost.org/users/download/), which is quite possibly
the most difficult and time-consuming step. Go to the website and
download the package, in ZIP or 7z format (Windows integrated
extractor is terribly slow; consider installing
[7z](https://www.7-zip.org/) and using it to unarchive Boost). Bad
news is that this time you have to build from the sources. There are
some prebuilt packages for Windows, but unfortunately none targets
MSVC 2017, so they are not usable in our case. Compiling Boost for
Windows is not completely trivial: I hope these instructions work for
you; if they do not, please check out the [upstream
documentation](http://www.boost.org/doc/libs/1_66_0/more/getting_started/windows.html).

Once you have unpacked the Boost distribution, using the Windows
starter screen open "x86 Native Tools Command Prompt for VS 2017",
which is basically the usual cmd shell with the right path to use
Visual Studio tools (including the C++ compiler). Go to the directory
where you have unpacked Boost and give these commands:

    bootstrap
    b2           # This will take a lot of time
    b2 install   # This will take less time, but still something

The last command will cause Boost headers and binaries to be installed
in `c:/Boost`. This path is again hardcoded in `mmpp.pro`.

Ok, we are finally at the point where we can compile `mmpp`. Open Qt
Creator from the Windows starter screen and click "New project". Then
select "Import project" and "Git clone". Insert
"https://github.com/giomasce/mmpp.git" as repository, mark the
"Recursive" check and then wait for the clone to finish. If all the
installation above went correctly, Qt Creator will automatically
propose to configure the project with the "Desktop Qt MSVC2017 64 bit"
kit, which you can accept. If it does not propore any kit, then
probably you have to check that you installed all the development
tools appropriately. Once the project is configured, you can build it
via the "Build" menu. By default Qt Creator will build in "Debug"
mode, which, as the name says, is useful for debugging, but it is
rather slow. I suggest you to move to "Release" mode, except for when
you really need to debug the program.

Even after having compiled `mmpp`, if you try to run it it will abort
with an error saying that `libz3.dll` is not available. In order to
make it available, you need to copy `libz3.dll`, `msvcp110.dll`,
`msvcr110.dll` and `vcomp110.dll` from the Z3 archive (or from
`c:/libs`) to the directory where the executable `mmpp` is situated.

To compile the JavaScript files, you need to open another (or use the
same as before) terminal with VS2017 development tools and go to the
directory where you checked out the repository content (by default
this will be "mmpp" in your Documents directory), then give the
command:

    tsc -p resources/static/ts

If you are stuck with an old version of Internet Explorer, you might
be unable to see anything in `webmmpp`, because on insufficient
JavaScript compatibility. In that case it is advisable to install a
more modern browser, like Firefox or Chrome.

# Preparing theory data

For doing nearly anything with `mmpp` you will need a `set.mm` theory
file to work with. While some parts of `mmpp` support generic Metamath
files, most of it is designed to work with `set.mm`. Actually, many
algorithms require some specific theorems that are not (yet) available
in the standard `set.mm`, so I suggest to use my personal fork, which
you can find
[here](https://raw.githubusercontent.com/giomasce/set.mm/develop/set.mm).

You have to put the downloaded file in the `resources` directory,
retaining the name `set.mm`. The first run of `mmpp` will take some
time to generate the cache for the LR parser and save it in the file
`set.mm.cache`. Each subsequent run will reuse the same cache file, so
the startup time will be much quicker (unless you modify `set.mm` in a
way that invalidates the cache: in such case `mmpp` will automatically
detect the need of rebuilding the cache, and this will take some time
again).

# Running `mmpp`

There are many different subcommands that you can execute when you run
`mmpp`. If you run `mmpp` without any subcommand or with the
subcommand `help`, it will list all the available subcomamnds. Most of
them are actually experimental or do nothing useful. Here I present
only those that can be at least something helpful to a casual
user. Some subcommands might not be available if you disabled its
dependencies in `mmpp.pro` before building.

## Web MMPP (`webmmpp`)

This subcommand is currently the main development target inside
`mmpp`: it aims to be a proving environment, similar in its philosophy
to `mmj2`, but more powerful and eye candy (it is not, so far). It
requires libmicrohttpd and the JavaScript files obtained by
transpiling the TypeScript sources. When started, the program spawns a
local webserver and opens a browser, which is then used as user
interface (it communicates with `mmpp` via AJAX requests). All the
heavy computations are still done in the `mmpp` process, but the
JavaScript code handles all the frontend interface. See below for more
information on how to use the interface.

## Unificator (`unificator`)

A simple tool to search for propositions in the theory that unify to a
given one. On each line you have to write the hypotheses and the
thesis you want to search for, separated by the character "$". The
program will reply with all the knowm propositions that unify to what
you asked. For example:

    |- ph $ |- ( ph -> ps ) $ |- ps
    Found 1 matching assertions:
     * ax-mp: & |- ph & |- ( ph -> ps ) => |- ps
    |- ( A = B -> A = B )
    Found 1 matching assertions:
     * id: => |- ( ph -> ph )
    |- 1 = 2
    Found 0 matching assertions:

## Verifier (`verify` and `verify_adv`)

Check that a Metamath theory file is correct. You have to specify the
filename on the command line. If you use `verify`, than a simple
correctness check will be ran. If you use `verify_adv`, then `mmpp`
will not only check the correctness of the database, but will also
test the proof compressions and decompression algorithms. This last
test is mostly to test `mmpp` algorithms, than to check whether your
files are correct.

## Generalizable theorems (`generalizable_theorems`)

Search and list all theorems in the theory for which the proof
actually proves a more general fact. Thus such theorems could be made
more general for virtually no price. There are many resons for which a
more specific form can be more desirable in some cases, so not all of
them are to be considered bugs. However, it might be interesting to
look for instances in which the excessive specificity is actually not
wanted.

## Substitution rules searcher (`subst_search`)

In `set.mm` it is expected that you can substitute a subformula for
another formula, provided that they are equal. In order to implement
this operation automatically, you need to know, for each syntax
builder and each Metamath variable appearing in it, an inference rule
that allows to bring an equality (or biimplication) outside of the
syntax builder. Unfortunately the database is not complete, meaning
that some of these inference rules are not proved. This program
searchs automatically for them. Each possible substitution rule is
searched in its three "distinguished" formats: as an actual inference
rule (which is the weaker format), as an implication theorem and as a
deduction rule. This command takes no arguments.

## Resolver (`resolver`)

This is mostly useful when debugging `mmpp`: internally all Metamath
symbols and labels are represented as numbers. During debugging it is
often useful to understand which symbol or label corresponds to a
certain number: the resolver tool is able to answer this type of
queries (provided it works on the same theory file is the program
being debugged, since otherwise the numbers change). You can pass on
the command line any number of strings or numbers: for each of them
the program will print the corresponding number or string,
interpreting the input both as a symbol and as a label (while the
Metmath specifications require that no symbol is equal to any label,
internally in `mmpp` they use two independent numberings, so the same
number can represent both a symbol and a label).

# The Web MMPP interface

In the Web interface you can create a number of worksets, each of
which is independent from the others and makes you able to work on
proving a single theorem. The upper row of buttons in the interface
lets you create a workset or access one already existing. The second
row of buttons gives access to the various functionalities of the
interface: you can load the `set.mm` database (without that, the
workset is almost completely useless), destroy the workset (currently
not implemented) or access the proof navigator or the proof editor.

The proof navigator just exposes an interface similar to those in the
Metamath Proof Explorer site: you can type a label and ask to print
its proof. If you click on the labels appearing in the proof, you will
be redirected to their proof in turn. For the moment it does not do
much more.

The proof editor is where the actual fun begins. You can edit a proof
by creating a tree of steps, where each node logically follow from its
children. As in `mmj2`, a node can be proved from its children by
unification from a previously proved theorem or from an
axiom. However, differently from `mmj2`, more complicated strategies
can be implemented, which are then resolved to possibly many steps
when the proof is generated. Each time a node or one of its chilren is
modified, all the available proving strategies are launched in
background on them: as soon as a strategy manages to find a proof, the
node is marked as proved.

## Editing the tree

You can create empty nodes at the top level with the button "Create
node". Each node as a number of small buttons on its left; in their
order:

 * The first button toggles the visibility of the children of the
   current node.

 * The second button toggles the visibility of the edit field.

 * The third button shows the children of the current node and hides
   in turn their children. This is helpful if you want to concentrate
   on a single step.

 * The fourth button create a new, empty children.

 * The fifth button destroys the node and all its children.

 * The sixth and seventh buttons move the node up and down among its
   siblings.

 * The eighth button permits to reparent a step as a child of any
   other step (not descending from it). First you press it (it is
   marked by the letter "R", as in "reparent"); all the "R" buttons
   becomes "H" (as in "here"), and you can choose any of it: the first
   step you clicked will be orphaned and reparented under the second
   one. You can cancel the operation by doing any other operation, or
   by clicking the "H" on the same node where you clicked the "R".

The last two buttons are not related to tree editing:

 * The "P" button generates a proof for the step; all the unproved
   children are considered hypotheses. The resulting proof is
   formatted in the Metamath language and written in the text area at
   the bottom of the page. You can copy and paste it wherever you
   want.

 * The "D" button generates a dump of the step and all its
   children. Currently this is the only way to save a proof when
   closing `mmpp`. Given the current stability of the program, or more
   precisely the lack thereof, it is advisable to do this frequently
   and copy the dump somewhere more stable, so you do not lose your
   work. You can also dump all the steps in the workset with the
   button "Dump whole tree". To restore a dump, copy it in the
   textarea at the bottom and click the button "Create node from
   dump". This is also the only current way to duplicate a subtree,
   which often comes in handy.

The content of a step can be edited by writing in the text field that
appears when clicking the second button. During editing, the pretty
printed version of the step is rendered on the right of the
buttons. Also, the proof status is written in the rectangle with
yellow background: if a proof is found for the node, then it writes
the name of the proving strategy and possibly other data; if not, the
work "searching" is written if some strategy is still searching for a
proof, and "failed" if all of them have failed.

## Available strategies

There are currently three available strategies:

 * `Unif`: This is the usual simple unification step, the basis of the
   Metamath language. The label will indicate the label of the user
   theorem (or axiom), and if you move the mouse over it a baloon will
   appear detailing the used substitution map.

 * `Wff`: The name is a bit of a misnomer, because of course this
   strategy is not able to prove general wffs. However, it is able to
   prove steps which follows from their children by purely
   propositional reasoning. The generated proof is, in general, much
   longer than the equivalent proof written by a human being; however,
   having this strategy at hands frees said human being from having to
   think too much about those that are usually considered technical
   details, enabling them to spend their mental resources on deeper
   thoughts.

 * `Uct`: This strategy tries to prove a step by repeatedly visiting
   and expanding a partial proof tree, in a manner similar to that
   described by Daniel Whalen in
   [arXiv:1608.02644](https://arxiv.org/abs/1608.02644). However, the
   implementation is very incomplete at this point and there is no
   machine learning, so do not expect much from it; still, it might be
   able to save typing a couple of easy steps every now and then. The
   number appearing in the label of a proved step is the number of
   visits there it took to build a proof.

Internally there are two different algorithms implementing the `Wff`
strategy: in any case, the formula to be proved is broken on its atoms
(its minimal subformulae that are only joined by logic operations), so
that it can be treated as a propositional formula. Then, the first
(and oldest) algorithm evaluates it assigning every possible
combination of the values true and false to its atoms. If all of such
evaluations return true, then a proof can be devised for the original
formula; such proof always has exponential length in the number of
atoms. The second algorithm, which is the default at the moment,
converts the formula to a Conjunctive Normal Form using Tseitin's
algorithm and then uses a generic CNF solver (minisat, here) to find a
proof (technically a refutation of its negation). The length of the
generated proof depends by the ability of the CNF solver to find a
short refutation: in general it cannot be expected to be less than
exponential, but for many something better can be hoped.

More strategies can definitely be implemented to further simplify the
proof editor's job. Although general theorem provers are notoriously
difficult to write (at least if you want to know the answer before the
Earth is swallowed by the Sun), there are many repetitive tasks in
writing Metamath proofs, which are often much more easier then general
theorems. By implementing more strategies, I believe that we will be
able to write proofs much quickier.

# How to contribute

However you want. Use GitHub pull requests, send me emails, patches,
opinions, whatever. There are a lot of things to do, and even beside I
see `mmpp` as an experiment playground. The nice thing of Metamath
proof is that you can experiment with them without risk: if the final
proof is validated by a good checker (not necessarily `mmpp`), it is
good, irrelevant of how funny is the code that generated it.

There are not specific conding conventions. The only thing I ask you
is to try to keep internal interfaces as clean and consistent as
possible. If you find some inconsistency, please fix it, instead of
writing one even worse. As a final suggestion, I use Qt Creator for
working on C++ code. It looks nice and clean to me and it is
multiplatform. If you use it, you can directly import the project in
`mmpp.pro` and have virtually nothing to configure.

# License

The code is copyright © 2016-2018 Giovanni Mascellani
<g.mascellani@gmail.com>. It is distributed under the terms of the
General Public License, [version
2](https://www.gnu.org/licenses/old-licenses/gpl-2.0.html) or
[later](https://www.gnu.org/licenses/old-licenses/gpl-3.0.html).

There are some exceptions: the files in the `libs` directory are
external libraries; each of them has a header detailing their
licensing status. Also, throughout the code there are small snippets
taken from various sources, most notably StackOverflow. They are noted
by comments that indicate their origin and their licensing status.
