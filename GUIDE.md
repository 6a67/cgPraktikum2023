# Quick start guide
## git
Git tracks incremental changes to files in a tree-like structure, allowing several users to synchronize their changes, undo code-breaking changes, and maintain different versions of the same files in parallel (each branch being a different parallel universe).

Most relevant commands:
```shell
git config --global user.name "YOUR NAME"    # Appears next to your commits in git logs
git config --global user.email "MAIL@uos.de"
git clone --recursive <repository>           # Creates a local copy of some remote repo
git branch <branch>                          # Creates a new branch from the current
git checkout <branch>                        # Switches to an existing branch
git add <file/folder>                        # Gather local changes for next commit
git commit -m <message>                      # Bundles and stores all gathered changes into a local commit
git push                                     # Copies commits on local branches to the same remote branches
git pull                                     # Copies commits on remote branches to the same local branches
git status                                   # Displays info about the local state
```

Potentially relevant to manage external dependencies:
```shell
cd extern                                    # Switch to the folder containing external dependencies
git submodule add --recursive <repository>   # Clones a repo to the current folder, marks it as submodule of main repo
git submodule update --init --recursive      # Pull the files from all submodules (including nested submodules)
```

## gitlab

If you can read this, you already have been given access to this gitlab-instance.
Think of gitlab as a decentralized alternative to github: individuals and organizations can host and manage their own instances on their own servers. Other than that, features are very similar to github.

For example you can use "Issues" to track bugs and features, the "Wiki" for documentation or check the "Repository Graph" for a graph-view of all branches and commits.

To synchronize with a gitlab instance via git, *ssh* is used. To authenticate yourself (i.e. when you do ```git clone``` or ```git push```, gitlab knows it's you and allows you to read and modify stuff) you need to generate an ssh key on your machine and register it on your gitlab instance account.

1) If the file ```~/.ssh/id_rsa.pub``` does not exist yet, do the following and use an empty passphrase if asked:
```shell
ssh-keygen
```
2) Copy the entire output of:
```shell
cat ~/.ssh/id_rsa.pub
```
3) On gitlab, click your Profile Icon -> Preferences -> SSH-Keys. Paste and save your SSH key there.

## Ubuntu
1) Learn to love the terminal.
2) If you need something installed, ask me and I will ask the system admin.

## VSCode

If you insist on using another editor or are already familiar with VSCode, you can ignore the following.

Otherwise I can really recommend using VSCode as a general-purpose code editor and IDE, for the following reasons:
- it is preinstalled on the CIP machines, saving me the effort to contact our system admin about installing your stuff :)
- get any functionality and language support via Plugins (accessible via the "Extension" tab in VSCode)
- it has an unobtrusive UI, integrating seamlessly with git
- unlike some IDEs, it does not abstract away the build pipeline, which you should learn about at least once during your CS career
- I (read as: your practical course instructor) am used to it and can help you if needed

## cmake & make

When working with bigger projects in compiled languages, it is really tedious to call compiler & linker yourself, calls would span over dozens of lines. Instead there are tools like cmake and make to automate the build & compile process for you, which have their own programming language.

Make is a tool that (for our purpose) just cares about knowing which parts of the project to recompile after you changed some files.
This avoids you having to either recompile the entire project after every small change or having to manually recompile individual parts yourself.

Cmake is the top-level build tool. You can configure every detail for your project in cmake and it will then generate the makefiles (used by make) automatically. It has a lot of functionality, of which you will likely use only very little because (a) this is only a small project and (b) the official documentation is quite bad. Luckily for you, we provide you with a fully working CMake configuration already set up.

All you have to do is say the magic words:
```shell
mkdir build
cd build
cmake ..
make -j
```

More advanced details usually only become relevant once you want to integrate your code with other libraries or vice versa.
Just remember that you need to call cmake after creating new source files (.cpp/.h) so that it registers them for compiling.

## C++

C++ gives you full control over almost anything: memory management, parallelization, etc.
This comes with a lot of potential to screw up. Which will happen. In every imaginable way. Trust me.

Because you will inevitably screw up, causing memory leaks, data races and the infamous *segmentation fault*s you need something better than the amateurs debugging routine, which usually looks like this:
1) Program exits unexpectedly with ```Segmentation fault (core dumped)```
2) You google "Segmentation fault (core dumped)"
3) You read on StackOverflow that this is usually caused by you trying to read some inaccessible memory
4) You wonder where such a thing might have happened, so you put somewhere in the code console logs that print ```Hello1``` to ```Hello18```.
5) You recompile your program, which might take anywhere from a few seconds to more than a minute (in extreme cases).
6) You execute your program again, it doesn't even print ```Hello1``` before ```Segmentation fault (core dumped)```.
7) You repeat steps 4-6 about 5 times until you pinned it down to a single line of code by binary search.
8) You see that you tried to access a 10-element array at index 53623451 in this line.
9) You fix your error.

Let me suggest you an alternative, by using ```gdb```, a simple command-line debugger present on all Linux systems:
1) Because you know you will screw up often, you always execute your program within the debugger like this:
```gdb <your_executable>```
2) Program exits unexpectedly with ```Segmentation fault (core dumped)```
3) You inspect the function call stack in gdb by calling ```backtrace``` and find the last function called from your code at position #3.
4) You jump into the scope of that function by calling ```frame 3```
5) You inspect the value of variable "index" by calling ```print index```, it shows 53623451
6) The line of code is shown in gdb, so you jump into the source file and fix your error.
7) ??????
8) Profit.

You know what's a primary reason for you screwing up?
That your code formatting is ugly and inconsistent, making your code harder to read and mistakes harder to spot.

You know whats better than manually formatting your code?
Using an autoformatter with a simple hotkey (Strg+Shift+I in VSCode).
A default *.clang-format* file, telling the autoformatter how you want your C++ to look like, lies in the root folder. Adjust it to your liking.

## pmp

pmp is short for *Polygon Mesh Processing*. It is a library providing you with a halfedge-mesh data structure and navigation routines, much like those encountered in the lecture on *Computer Graphics*. Moreover it has a basic mesh viewer, which we will use as a starting point for our own visualization, and several common mesh processing algorithms already implemented (like smoothing, subdividing, remeshing etc.).

Most important to you in the beginning will be functions for navigating on a mesh and associating properties to mesh elements.
A bunch of that is already explained [here](https://www.pmp-library.org/tutorial.html).
For more details you will later want to check the code documentation. For mesh navigation e.g. the file to look at is [surface_mesh.h](extern/pmp-library/src/pmp/surface_mesh.h). Strg+F is your helper. And I am too, so just ask me whenever you need help with pmp.

## ImGui

*ImGui* is the library used by pmp (and by us) to put some graphical user interface (GUI) on the screen.
It follows the "*immediate-mode*"-GUI paradigm, which makes it much more intuitive and easy to define GUI-elements on the fly within your code.
Creating a button that when clicked calls a function ```animate()``` is as easy as writing (in the correct place in the code)
```c++
if (ImGui::Button("Animate"))
{
    animate();
}
```
Creating a slider that lets you adjust a global integer ```level``` between values 1 and 10:
```c++
ImGui::SliderInt("Level", &level, 1, 10);
```
Creating a color picker for an RGBA ```float color[4];``` variable:
```c++
ImGui::ColorEdit4("Control Point Color", color);
```

Similar editing functions exist for many types of variables. The part that is trickier is finding out how to interconnect the rendering code and libraries such as *GLFW*, *GLEW*, etc. to *ImGui*.

Fortunately, much of the boiler plate is already handled by pmp. Later in the course, when you start digging into the graphics pipeline, it will be necessary to either extend the base viewer of pmp by inheritance (check file [window.h](extern/pmp-library/src/pmp/visualization/window.h) for what you need to implement) or create your own custom viewer (using files in [extern/pmp-library/src/pmp/visualization/](extern/pmp-library/src/pmp/visualization/) as inspiration).
