# gem5_chips

This is a modified version of gem5 with support to model and study CHIPS systems:
![alt text](https://github.com/GT-CHIPS/gem5_chips/blob/master/images/chips_examples.png)



How to install gem5:
-------------------------------------

`git clone https://github.com/GT-CHIPS/gem5_chips.git`


Software packages to install gem5:
-------------------------------------

On Ubuntu, you can install all of the required dependencies with the following command.

```sudo apt install build-essential git m4 scons zlib1g zlib1g-dev libprotobuf-dev protobuf-compiler libprotoc-dev libgoogle-perftools-dev python-dev python```


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


Build command
--------------
```scons build/RISCV_MESI_Two_Level/gem5.opt```
(you can add -j N for a faster N-threaded build)


Example run command
--------------
### Hello World
```
./build/RISCV_MESI_Two_Level/gem5.opt configs/example/se.py \
--cpu-type TimingSimpleCPU \
--num-cpus=64 \
--l1d_size=16kB \
--l1i_size=16kB \
--num-l2caches=64 \
--l2_size=128kB \
--num-dirs=4 \
--mem-size=4096MB \
--ruby \
--network=garnet2.0 \
--topology=CHIPS_Multicore_MemCtrlChiplet4 \
-c tests/test-progs/hello/bin/riscv/linux/hello
```

### BFS:
```
./build/RISCV_MESI_Two_Level/gem5.opt configs/example/se.py \
--cpu-type TimingSimpleCPU \
--num-cpus=64 \
--l1d_size=16kB \
--l1i_size=16kB \
--num-l2caches=64 \
--l2_size=128kB \
--num-dirs=4 \
--mem-size=4096MB \
--ruby \
--network=garnet2.0 \
--topology=CHIPS_Multicore_MemCtrlChiplet4 \
-c workloads/ligra/binaries/BFS -o '-n 64 workloads/ligra/input/rMatGraph_J_5_100'
```

Example CHIPS Topologies
-----------------
```
configs/topologies/CHIPS_Multicore_Monolithic.py 
configs/topologies/CHIPS_Multicore_MemCtrlChiplet4.py
configs/topologies/CHIPS_Multicore_GTRocketN.py
```

Commandline options to modify CLIP parameters
-----------------
![alt text](https://github.com/GT-CHIPS/gem5_chips/blob/master/images/clip.png)

- `--chiplet-link-latency`
- `--chiplet-link-width`
- `--interposer-link-latency`
- `--interposer-link-width`
- `--clip-logic-ifc-delay`
- `--clip-phys-ifc-delay`
- `--buffers-per-ctrl-vc`
- `--buffers-per-data-vc`


More details about gem5
-----------------
- [Public Repo](www.gem5.org)
- [Garnet Network Model](www.gem5.org/Garnet2.0)
- [Learning gem5](http://learning.gem5.org/)
- [Dependencies](http://gem5.org/Dependencies)

Contact
-------------------------------------
Tushar Krishna: tushar@ece.gatech.edu

Acknowledgments
-----------------
- This work was supported by DARPA CHIPS.
- Srikant Bharadwaj (AMD Research)
- Christopher Batten, Tuan Ta (Cornell University)
