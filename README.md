# gem5_chips

Requirements for gem5
See gem5 requirements for more details: (http://gem5.org/Compiling_M5#Required_Software)

For information about gem5: http://gem5.org/Main_Page

For information about garnet2.0: http://www.gem5.org/Garnet2.0


On Ubuntu, you can install all of the required dependencies with the following command.

The requirements are detailed below.
-------------------------------------

sudo apt install build-essential git m4 scons zlib1g zlib1g-dev libprotobuf-dev protobuf-compiler libprotoc-dev libgoogle-perftools-dev python-dev python


### git (Git):

The gem5 project uses Git for version control. Git is a distributed version control system. More information about Git can be found by following the link. Git should be installed by default on most platforms. However, to install Git in Ubuntu use

sudo apt-get install git

### gcc 4.8+

You may need to use environment variables to point to a non-default version of gcc.

On Ubuntu, you can install a development environment with

sudo apt-get install build-essential

### SCons
gem5 uses SCons as its build environment. SCons is like make on steroids and uses Python scripts for all aspects of the build process. This allows for a very flexible (if slow) build system.

To get SCons on Ubuntu use

sudo apt-get install scons

### Python 2.7+
gem5 relies on the Python development libraries. To install these on Ubuntu use

sudo apt-get install python-dev

### protobuf 2.1+
“Protocol buffers are a language-neutral, platform-neutral extensible mechanism for serializing structured data.” In gem5, the protobuf library is used for trace generation and playback. protobuf is not a required package, unless you plan on using it for trace generation and playback.

sudo apt-get install libprotobuf-dev python-protobuf protobuf-compiler libgoogle-perftools-dev

Getting the code
Change directories to where you want to download the gem5 source. Then, to clone the repository, use the git clone command.


build command
--------------
scons -j64 build/RISCV_MESI_Two_Level/gem5.opt



run command
--------------
### BFS:

 ./build/RISCV_MESI_Two_Level/gem5.opt -d results/BFS configs/example/se.py --cpu-type DerivO3CPU -n 64 -c binaries/BFS -o '-n 64 input/rMatGraph_J_5_100' --ruby --network=garnet2.0 --num-l2caches=64 --mem-size=4096MB --mesh-rows=8 --topology=Mesh_XY
