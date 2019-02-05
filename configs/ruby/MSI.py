# -*- coding: utf-8 -*-
# Copyright (c) 2017 Jason Power
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
# Authors: Jason Power

import math
import m5
from m5.objects import *
from m5.defines import buildEnv
from Ruby import create_topology, create_directories
from Ruby import send_evicts

#
# Declare caches used by the protocol
#
class L1Cache(RubyCache): pass

def define_options(parser):
    return

def create_system(options, full_system, system, dma_ports, ruby_system):

    if buildEnv['PROTOCOL'] != 'MSI':
        panic("This script requires the MSI protocol to be built.")

    cpu_sequencers = []

    #
    # The ruby network creation expects the list of nodes in the system
    # to be consistent with the NetDest list.  Therefore the l1
    # controller nodes must be listed before the directory nodes and
    # directory nodes before dma nodes, etc.
    #
    l1_cntrl_nodes = []

    #
    # Must create the individual controllers before the network to ensure the
    # controller constructors are called before the network constructor
    #
    block_size_bits = int(math.log(options.cacheline_size, 2))

    for i in xrange(options.num_cpus):
        #
        # First create the Ruby objects associated with this cpu
        # Only one cache exists for this protocol, so by default use the L1D
        # config parameters.
        #
        l1i_cache = L1Cache(size = options.l1i_size,
                            assoc = options.l1i_assoc,
                            start_index_bit = block_size_bits,
                            is_icache = True)
        l1d_cache = L1Cache(size = options.l1d_size,
                            assoc = options.l1i_assoc,
                            start_index_bit = block_size_bits,
                            is_icache = False)

        # the ruby random tester reuses num_cpus to specify the
        # number of cpu ports connected to the tester object, which
        # is stored in system.cpu. because there is only ever one
        # tester object, num_cpus is not necessarily equal to the
        # size of system.cpu; therefore if len(system.cpu) == 1
        # we use system.cpu[0] to set the clk_domain, thereby ensuring
        # we don't index off the end of the cpu list.
        if len(system.cpu) == 1:
            clk_domain = system.cpu[0].clk_domain
        else:
            clk_domain = system.cpu[i].clk_domain

        # Only one unified L1 cache exists. Can cache instructions and data.
        l1_cntrl = L1Cache_Controller(version=i, L1Icache=l1i_cache,
                                      L1Dcache = l1d_cache,
                                      send_evictions=send_evicts(options),
                                      transitions_per_cycle=options.ports,
                                      clk_domain=clk_domain,
                                      ruby_system=ruby_system)

        cpu_seq = RubySequencer(version=i, icache=l1i_cache, dcache=l1d_cache,
                                clk_domain=clk_domain, ruby_system=ruby_system)

        l1_cntrl.sequencer = cpu_seq
        exec("ruby_system.l1_cntrl%d = l1_cntrl" % i)

        # Add controllers and sequencers to the appropriate lists
        cpu_sequencers.append(cpu_seq)
        l1_cntrl_nodes.append(l1_cntrl)

        # Connect the L1 controllers and the network
        l1_cntrl.mandatoryQueue = MessageBuffer()
        l1_cntrl.triggerQueue = MessageBuffer()
        l1_cntrl.requestToDir = MessageBuffer(ordered = True)
        l1_cntrl.requestToDir.master = ruby_system.network.slave
        l1_cntrl.responseToDirOrSibling = MessageBuffer(ordered = True)
        l1_cntrl.responseToDirOrSibling.master = ruby_system.network.slave
        l1_cntrl.forwardFromDir = MessageBuffer(ordered = True)
        l1_cntrl.forwardFromDir.slave = ruby_system.network.master
        l1_cntrl.responseFromDirOrSibling = MessageBuffer(ordered = True)
        l1_cntrl.responseFromDirOrSibling.slave = ruby_system.network.master

    phys_mem_size = sum(map(lambda r: r.size(), system.mem_ranges))
    assert(phys_mem_size % options.num_dirs == 0)
    mem_module_size = phys_mem_size / options.num_dirs

    # Run each of the ruby memory controllers at a ratio of the frequency of
    # the ruby system.
    # clk_divider value is a fix to pass regression.
    ruby_system.memctrl_clk_domain = DerivedClockDomain(
                                          clk_domain=ruby_system.clk_domain,
                                          clk_divider=3)

    dir_cntrl_nodes = create_directories(options, system.mem_ranges,
                                         ruby_system)
    for dir_cntrl in dir_cntrl_nodes:
        # Connect the directory controllers and the network
        dir_cntrl.requestFromCache = MessageBuffer(ordered = True)
        dir_cntrl.requestFromCache.slave = ruby_system.network.master
        dir_cntrl.responseFromCache = MessageBuffer(ordered = True)
        dir_cntrl.responseFromCache.slave = ruby_system.network.master

        dir_cntrl.responseToCache = MessageBuffer(ordered = True)
        dir_cntrl.responseToCache.master = ruby_system.network.slave
        dir_cntrl.forwardToCache = MessageBuffer(ordered = True)
        dir_cntrl.forwardToCache.master = ruby_system.network.slave
        dir_cntrl.responseFromMemory = MessageBuffer()


    all_cntrls = l1_cntrl_nodes + dir_cntrl_nodes

    # Create the io controller and the sequencer
    if full_system:
        panic("MSI protocol does not support full system mode.")

    ruby_system.network.number_of_virtual_networks = 5
    topology = create_topology(all_cntrls, options)
    return (cpu_sequencers, dir_cntrl_nodes, topology)
