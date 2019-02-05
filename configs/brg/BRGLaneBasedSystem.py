# Copyright (c) 2012-2013 ARM Limited
# All rights reserved.
#
# The license below extends only to copyright in the software and shall
# not be construed as granting a license to any other intellectual
# property including but not limited to intellectual property relating
# to a hardware implementation of the functionality of the software
# licensed hereunder.  You may use the software subject to the license
# terms below provided that you ensure that this notice is replicated
# unmodified and in its entirety in all distributions of the software,
# modified or unmodified, in source code or in binary form.
#
# Copyright (c) 2006-2008 The Regents of The University of Michigan
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: Steve Reinhardt, Tuan Ta

# @tuan
# This script configures a lane-based system that consists of multiple
# general-purposed CPUs (GPPs) and lane-based accelerators. Each lane-based
# accelerator is composed of multiple light-weight execution lanes that
# share certain HW resource.
#
# Each computing tile consists of 1 GPP core (e.g., either in-order or
# out-of-order CPU) and a lane-based accelerator (or lane group) that is
# composed of multiple in-order light-weight execution lanes.
#
#   -------------------------------------------------------
#   | Computing Tile                                      |
#   |                                                     |
#   |   ------------  ---------------------------------   |
#   |   | GPP Core |  | Lane Group                    |   |
#   |   ------------  |                               |   |
#   |                 | Lane-0  Lane-1  Lane-2  ...   |   |
#   |                 ---------------------------------   |
#   -------------------------------------------------------
#

import optparse
import sys
import os

import m5
from m5.defines import buildEnv
from m5.objects import *
from m5.util import addToPath, fatal, warn

addToPath('../')

from ruby import Ruby

from common import Options
from common import Simulation
from common import CpuConfig
from common import CacheConfig
from common import MemConfig
from common.Caches import *
from common.cpu2000 import *

def addRgalsOptions(parser):
    # R-GALS project
    parser.add_option("--cache-latency-multiplier", action="store", type="int",
                      default='1',
                      help="Multiplier for L1/L2 hit/miss penalty latencies")

def get_processes(options):
    """Interprets provided options and returns a list of processes"""

    multiprocesses = []
    inputs = []
    outputs = []
    errouts = []
    pargs = []

    workloads = options.cmd.split(';')
    if options.input != "":
        inputs = options.input.split(';')
    if options.output != "":
        outputs = options.output.split(';')
    if options.errout != "":
        errouts = options.errout.split(';')
    if options.options != "":
        pargs = options.options.split(';')

    idx = 0
    for wrkld in workloads:
        process = Process(pid = 100 + idx)
        process.executable = wrkld
        process.cwd = os.getcwd()

        if options.env:
            with open(options.env, 'r') as f:
                process.env = [line.rstrip() for line in f]

        if len(pargs) > idx:
            process.cmd = [wrkld] + pargs[idx].split()
        else:
            process.cmd = [wrkld]

        if len(inputs) > idx:
            process.input = inputs[idx]
        if len(outputs) > idx:
            process.output = outputs[idx]
        if len(errouts) > idx:
            process.errout = errouts[idx]

        multiprocesses.append(process)
        idx += 1

    if options.smt:
        assert(options.cpu_type == "DerivO3CPU")
        return multiprocesses, idx
    else:
        return multiprocesses, 1

#
# BRG lane-based system only works with RISCV ISA
#
assert buildEnv['TARGET_ISA'] == 'riscv'

#-------------------------------------------------------------------------
# parse options
#-------------------------------------------------------------------------

parser = optparse.OptionParser()
Options.addCommonOptions(parser)
Options.addSEOptions(parser)
Options.addBRGOptions(parser)
Options.addLaneOptions(parser)
addRgalsOptions(parser)

if '--ruby' in sys.argv:
    Ruby.define_options(parser)

(options, args) = parser.parse_args()

if args:
    print "Error: script doesn't take any positional arguments"
    sys.exit(1)

# overwrite cpu_type to LaneCPU

options.cpu_type = "LaneCPU"

#-------------------------------------------------------------------------
# get workloads to run and create processes
#-------------------------------------------------------------------------

multiprocesses = []
numThreads = 1

if options.bench:
    fatal("Error: script doesn't support --bench option\n")
    sys.exit(1)
elif options.cmd:
    multiprocesses, numThreads = get_processes(options)
else:
    print >> sys.stderr, "No workload specified. Exiting!\n"
    sys.exit(1)

# Check -- do not allow SMT with multiple CPUs
if options.smt and options.num_cpus > 1:
    fatal("You cannot use SMT with multiple CPUs!")

#----------------------------------------------------------------------------
# @tuan: set up CPUClass for --brg-fast-forward option
#
#   If --brg-fast-forward is used, WarmupCPUClass CPUs are used before the
#   main timing region starts. CPUClass CPUs are used within the timing region
#   while CooldownCPUClass CPUs are used after the timing region for
#   verification and cleanup purpose.
#
#----------------------------------------------------------------------------

WarmupCPUClass = None
CPUClass = None
CooldownCPUClass = None

if options.brg_fast_forward:
    WarmupCPUClass = AtomicSimpleCPU
    CooldownCPUClass = AtomicSimpleCPU

CPUClass, main_mem_mode = Simulation.getCPUClass(options.cpu_type)

if options.brg_fast_forward:
    WarmupCPUClass.numThreads = numThreads
    CooldownCPUClass.numThreads = numThreads

CPUClass.numThreads = numThreads

#-------------------------------------------------------------------------
# Create CPU system
#-------------------------------------------------------------------------

# @tuan
# NOTE: for now, the script creates a system without GPP cores. The system
# has a number of tiles, each of which has only one lane group.
num_tiles       = options.num_tiles
lane_group_size = options.lane_group_size
num_lanes       = num_tiles * lane_group_size

# overwrite the options.num_cpus to reflect the total number of CPU lanes
# across all tiles
options.num_cpus = num_lanes

# select a CPU class used to start the simulation
FirstCPUClass = WarmupCPUClass if options.brg_fast_forward else CPUClass

system = System(cpu = [FirstCPUClass(cpu_id=i) for i in xrange(num_lanes)],
                mem_mode = FirstCPUClass.memory_mode(),
                mem_ranges = [AddrRange(options.mem_size)],
                cache_line_size = options.cacheline_size)

if FirstCPUClass.type == 'LaneCPU':
    for i in xrange(num_lanes):
        system.cpu[i].lane_id = i % lane_group_size

# Overwrite CPU clock to 1GHz
#options.cpu_clock = '1GHz'

# Create a top-level voltage domain
system.voltage_domain = VoltageDomain(voltage = options.sys_voltage)

# Create a source clock for the system and set the clock period
system.clk_domain = SrcClockDomain(clock =  options.sys_clock,
                                   voltage_domain = system.voltage_domain)

# Create a CPU voltage domain
system.cpu_voltage_domain = VoltageDomain()

# Create a separate clock domain for the CPUs
system.cpu_clk_domain = SrcClockDomain(clock = options.cpu_clock,
                                       voltage_domain =
                                       system.cpu_voltage_domain)

# Overwrite cache clock
#options.cache_clock = '1.1GHz'

# Create a cache voltage domain
system.cache_voltage_domain = VoltageDomain()

# Create a separate clock domain for the caches
system.cache_clk_domain = SrcClockDomain(clock = options.cache_clock,
                                       voltage_domain =
                                       system.cache_voltage_domain)

# All cpus belong to a common cpu_clk_domain, therefore running at a common
# frequency.
for cpu in system.cpu:
    cpu.clk_domain = system.cpu_clk_domain

# Turn on BRG activity trace
for cpu in system.cpu:
    cpu.activity_trace = options.activity_trace

# Sanity check
if options.fastmem:
    fatal("fastmem option is not supported!")
if options.simpoint_profile:
    fatal("simpoint_profile option is not supported!")
if options.checker:
    fatal("checker option is not supported!")

# Assign processes to CPUs
for i in xrange(num_lanes):
    if options.smt:
        system.cpu[i].workload = multiprocesses
    elif len(multiprocesses) == 1:
        system.cpu[i].workload = multiprocesses[0]
    else:
        system.cpu[i].workload = multiprocesses[i]

    system.cpu[i].createThreads()

#-------------------------------------------------------------------------
# Create memory system
#-------------------------------------------------------------------------

if options.ruby:
    fatal("Ruby memory system is not supported yet!")

if options.elastic_trace_en:
    fatal("Do not support elastic_trace_en")

if options.memchecker:
    fatal("Do not support memchecker")

MemClass = Simulation.setMemClass(options)
system.membus = SystemXBar()
system.system_port = system.membus.slave

#
# Config cache hierarchy
#

dcache_class, icache_class, l2_cache_class = L1_DCache, L1_ICache, L2Cache

# Set the cache line size of the system
system.cache_line_size = options.cacheline_size

# Override the cache latency multiplier
#options.cache_latency_multiplier = 1

# Create l2 cache
if options.l2cache:
    # Provide a clock for the L2 and the L1-to-L2 bus here as they
    # are not connected using addTwoLevelCacheHierarchy. Use the
    # same clock as the CPUs.
    system.l2 = l2_cache_class( \
        clk_domain       = system.cache_clk_domain,
        size             = options.l2_size,
        assoc            = options.l2_assoc,
        data_latency     = 20 * options.cache_latency_multiplier,
        tag_latency      = 20 * options.cache_latency_multiplier,
        response_latency = 2 * options.cache_latency_multiplier)

    system.tol2bus = L2XBar(clk_domain = system.cache_clk_domain)
    system.l2.cpu_side = system.tol2bus.master
    system.l2.mem_side = system.membus.slave

# Create icache, dcache and interrupt controller
for i in xrange(options.num_cpus):
    # @tuan
    # To model a blocking cache, mshrs is set to 1, and tgts_per_mshr is
    # set to 0, which makes cache process at most 1 request at a time.
    # A miss blocks any subsequent requests.
    if options.shared_icache:
        if i == 0:
            system.icache = icache_class( \
                clk_domain       = system.cache_clk_domain,
                size             = options.l1i_size,
                assoc            = options.l1i_assoc,
                tag_latency      = options.l1i_hit_lat * \
                                            options.cache_latency_multiplier,
                data_latency     = options.l1i_hit_lat * \
                                            options.cache_latency_multiplier,
                response_latency = 1 * options.cache_latency_multiplier,
                mshrs            = options.icache_mshrs,
                tgts_per_mshr    = 1)
    else:
        system.cpu[i].icache = icache_class( \
            clk_domain       = system.cache_clk_domain,
            size             = options.l1i_size,
            assoc            = options.l1i_assoc,
            tag_latency      = options.l1i_hit_lat * \
                                            options.cache_latency_multiplier,
            data_latency     = options.l1i_hit_lat * \
                                            options.cache_latency_multiplier,
            response_latency = 1 * options.cache_latency_multiplier,
            mshrs            = options.icache_mshrs,
            tgts_per_mshr    = 1)

    if options.shared_dcache:
        if i == 0:
            system.dcache = dcache_class( \
                clk_domain       = system.cache_clk_domain,
                size             = options.l1d_size,
                assoc            = options.l1d_assoc,
                tag_latency      = options.l1d_hit_lat * \
                                            options.cache_latency_multiplier,
                data_latency     = options.l1d_hit_lat * \
                                            options.cache_latency_multiplier,
                response_latency = 1 * options.cache_latency_multiplier,
                mshrs            = options.dcache_mshrs,
                tgts_per_mshr    = 1)

        if options.memchecker:
            system.cpu[i].dcache_mon = MemCheckerMonitor(warn_only = True)
            system.cpu[i].dcache_mon.memchecker = system.memchecker
    else:
        system.cpu[i].dcache = dcache_class( \
            clk_domain       = system.cache_clk_domain,
            size             = options.l1d_size,
            assoc            = options.l1d_assoc,
            tag_latency      = options.l1d_hit_lat * \
                                            options.cache_latency_multiplier,
            data_latency     = options.l1d_hit_lat * \
                                            options.cache_latency_multiplier,
            response_latency = 1 * options.cache_latency_multiplier,
            mshrs            = options.dcache_mshrs,
            tgts_per_mshr    = 1)

    system.cpu[i].createInterruptController()

# Create instruction L0 buffers, one per lane
if options.l0i:
    system.l0i_buffers = [L0Buffer(clk_domain=system.cpu_clk_domain,
                                   size = options.l0i_size,
                                   cache_line_size = \
                                                  options.cacheline_size) \
                          for i in xrange(num_lanes)]

# Create a cache allocator
if options.shared_icache:
    system.icache_allocator = CacheAllocator(clk_domain = \
                                                system.cache_clk_domain,
                                             num_cpu_ports = \
                                                num_lanes,
                                             cache_line_size = \
                                                options.cacheline_size,
                                             enable_coalescing = \
                                                options.coalescing_icache,
                                             request_latency = \
                                                options.allocator_req_latency,
                                             response_latency = \
                                                options.allocator_resp_latency)

if options.shared_dcache:
    system.dcache_allocator = CacheAllocator(clk_domain = \
                                                system.cache_clk_domain,
                                             num_cpu_ports = \
                                                num_lanes,
                                             cache_line_size = \
                                                options.cacheline_size,
                                             enable_coalescing = \
                                                options.coalescing_dcache,
                                             request_latency = \
                                                options.allocator_req_latency,
                                             response_latency = \
                                                options.allocator_resp_latency)

# Set up connections
for i in xrange(num_lanes):
    if options.l0i:
        if not options.shared_icache:
            # Connect cpu.icache_port <-> L0-I <-> cpu.icache
            system.cpu[i].icache_port = system.l0i_buffers[i].cpu_port;
            system.cpu[i].icache.cpu_side = system.l0i_buffers[i].cache_port;
        else:
            # Connect cpu.icache_port <-> L0-I <-> cache_allocator
            # <-> cpu.icache
            system.cpu[i].icache_port = system.l0i_buffers[i].cpu_port;
            system.l0i_buffers[i].cache_port = \
                                        system.icache_allocator.cpu_ports[i];
            if i == 0:
                system.icache.cpu_side = system.icache_allocator.cache_port;
    else:
        if not options.shared_icache:
            # Connect cpu.icache_port <-> cpu.icache
            system.cpu[i].icache_port = system.cpu[i].icache.cpu_side
        else:
            # Connect cpu.icache_port <-> cache_allocator <-> cpu.icache
            system.cpu[i].icache_port = system.icache_allocator.cpu_ports[i];
            if i == 0:
                system.icache.cpu_side = system.icache_allocator.cache_port;

    if options.shared_dcache:
        # Connect cpu.dcache_port <-> cache_allocator <-> cpu.dcache
        system.cpu[i].dcache_port = system.dcache_allocator.cpu_ports[i]
        if i == 0:
            system.dcache.cpu_side = system.dcache_allocator.cache_port
    else:
        # Connect cpu.dcache_port <-> cpu.dcache
        system.cpu[i].dcache_port = system.cpu[i].dcache.cpu_side

    # NOTE - assuming that we always have L2
    assert options.l2cache

    # Connect icache ports to system.tol2bus
    if options.shared_icache:
        if i == 0:
            system.icache.mem_side = system.tol2bus.slave
    elif not options.shared_icache:
        system.cpu[i].icache.mem_side = system.tol2bus.slave

    # Connect dcache ports to system.tol2bus
    if options.shared_dcache:
        if i == 0:
            system.dcache.mem_side = system.tol2bus.slave
    elif not options.shared_dcache:
        system.cpu[i].dcache.mem_side = system.tol2bus.slave

    # Connect cache ports to system.tol2bus/system.membus
    #system.cpu[i]._cached_ports = ['icache.mem_side', 'dcache.mem_side']
    #if options.l2cache:
    #    system.cpu[i].connectAllPorts(system.tol2bus, system.membus)
    #else:
    #    system.cpu[i].connectAllPorts(system.membus)

# Configure main memory
MemConfig.config_mem(options, system)

# Set brg fast-forward for the system
if options.brg_fast_forward:
    system.brg_fast_forward = True

#-------------------------------------------------------------------------
# Run the simulation
#-------------------------------------------------------------------------

root = Root(full_system = False, system = system)

if options.brg_fast_forward:
    Simulation.run_brg(options, root, system, CPUClass, CooldownCPUClass)
else:
    Simulation.run(options, root, system, None)
