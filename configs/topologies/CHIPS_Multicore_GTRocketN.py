# Copyright (c) 2018 Georgia Institute of Technology
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
# Authors: Tushar Krishna

from m5.params import *
from m5.objects import *

from BaseTopology import SimpleTopology
import math

# Creates a Hierarchical Mesh Topology.
# If there are k^k CPUs, then k clusters, each with (sqrt k) CPUs are created.
# One L1 is connected to each CPU.
# Each cluster uses a Mesh NoC.
# XY routing is enforced (using link weights) to guarantee deadlock freedom.
# The k CPU clusters, k L2s and 4 Memory controllers are all
# connected as separate chiplets via a crossbar.

class CHIPS_Multicore_GTRocketN(SimpleTopology):
    description='CHIPS_Multicore_GTRocketN'

    def __init__(self, controllers):
        self.nodes = controllers

    def makeTopology(self, options, network, IntLink, ExtLink, Router):
        # Default values for link latency and router latency.
        # Can be over-ridden on a per link/router basis
        chiplet_link_latency = options.chiplet_link_latency
        chiplet_link_width = options.chiplet_link_width
        interp_link_latency = options.interposer_link_latency
        interp_link_width = options.interposer_link_width
        router_latency = options.router_latency

        nodes = self.nodes

        # First determine which nodes are cache cntrls vs. dirs vs. dma
        cpu_nodes = []
        l2_nodes = []
        mc_nodes = []
        dma_nodes = []
        for node in nodes:
            if node.type == 'L1Cache_Controller':
                cpu_nodes.append(node)
            elif node.type == 'L2Cache_Controller':
                l2_nodes.append(node)
            elif node.type == 'Directory_Controller':
                mc_nodes.append(node)
            elif node.type == 'DMA_Controller':
                dma_nodes.append(node)


        # Compute the configuration:
        num_cpu_chiplets = int(math.sqrt(len(cpu_nodes)))
        num_cpus_per_chiplet = int(len(cpu_nodes) / num_cpu_chiplets)
        num_l2_mc_dma_chiplets = len(l2_nodes) + len(mc_nodes) + len(dma_nodes)
        num_chiplets = num_cpu_chiplets + num_l2_mc_dma_chiplets + 1 #(xbar)


        # Mesh rows and columns
        num_rows = int(math.sqrt(num_cpus_per_chiplet))
        num_columns = int(num_cpus_per_chiplet / num_rows)
        assert((num_rows*num_columns*num_cpu_chiplets) == options.num_cpus)

        num_routers = len(cpu_nodes) + len(l2_nodes) \
                        + len(mc_nodes) + len(dma_nodes) \
                        + 1 # (for xbar)

        # Print configuration
        print "Configuration:\nNum CPU Chiplets = " + str(num_cpu_chiplets) + \
              "\nNum L2 Chiplets = " + str(len(l2_nodes)) + \
              "\nNum MC Chiplets = " + str(len(mc_nodes)) + \
              "\nNum DMA Chiplets = " + str(len(dma_nodes)) + \
              "\nCPU Chiplet Configuration: " + str(num_rows) + " x " + \
                    str(num_columns) + " Mesh"

        # Create the routers
        routers = [Router(router_id=i, latency = router_latency) \
            for i in range(num_routers)]
        network.routers = routers

        # link counter to set unique link ids
        link_count = 0

        # start from router 0
        router_id = 0

        # Connect each CPU to a unique router
        ext_links = []
        for (i, n) in enumerate(cpu_nodes):
            routers[router_id].width = chiplet_link_width # nominal flit size
            ext_links.append(ExtLink(link_id=link_count, ext_node=n,
                                    int_node=routers[router_id],
                                    latency = chiplet_link_latency,
                                    width = chiplet_link_width))
            print_connection("CPU", n.version, "Router", router_id, link_count,\
                              chiplet_link_latency, chiplet_link_width)
            link_count += 1
            router_id += 1

        l2c_router_start_id = router_id
        # Connect each L2 to a router
        for (i, n) in enumerate(l2_nodes):
            routers[router_id].width = chiplet_link_width # nominal flit size
            ext_links.append(ExtLink(link_id=link_count, ext_node=n,
                                    int_node=routers[router_id],
                                    latency = chiplet_link_latency,
                                    width = chiplet_link_width))
            print_connection("L2", n.version, "Router", router_id, link_count,\
                              chiplet_link_latency, chiplet_link_width)
            link_count += 1
            router_id += 1

        mcc_router_start_id = router_id
        # Connect the MC nodes to routers
        for (i, n) in enumerate(mc_nodes):
            routers[router_id].width = chiplet_link_width # nominal flit size
            ext_links.append(ExtLink(link_id=link_count, ext_node=n,
                                    int_node=routers[router_id],
                                    latency = chiplet_link_latency,
                                    width = chiplet_link_width))
            print_connection("MC", n.version, "Router", router_id, link_count,\
                              chiplet_link_latency, chiplet_link_width)
            link_count += 1
            router_id += 1

        dmac_router_start_id = router_id

        # Connect the DMA nodes to routers
        for (i, n) in enumerate(dma_nodes):
            routers[router_id].width = chiplet_link_width # nominal flit size
            ext_links.append(ExtLink(link_id=link_count, ext_node=n,
                                    int_node=routers[router_id],
                                    latency = chiplet_link_latency,
                                    width = chiplet_link_width))
            print_connection("DMA", n.version, "Router", router_id, link_count,\
                              chiplet_link_latency, chiplet_link_width)
            link_count += 1
            router_id += 1

        network.ext_links = ext_links

        ## All routers except xbar have been connected
        assert(router_id == num_routers - 1)
        xbar_id = router_id # This is the last remaining router

        routers[xbar_id].latency=4 # Assume 4-cycle high-radix xbar

        # Create the mesh links inside each chiplet
        int_links = []
        
        test_num_cpus = 0
        for cc in xrange(num_cpu_chiplets):
            print "Topology for CPU Chiplet " + str(cc) + ":"
            # East output to West input links (weight = 1)
            for row in xrange(num_rows):
                for col in xrange(num_columns):
                    test_num_cpus += 1
                    if (col + 1 < num_columns):
                        east_out = (cc*num_cpus_per_chiplet) + col + (row * num_columns)
                        west_in = (cc*num_cpus_per_chiplet) + (col + 1) + (row * num_columns)
                        int_links.append(IntLink(link_id=link_count,
                                                 src_node=routers[east_out],
                                                 dst_node=routers[west_in],
                                                 src_outport="East",
                                                 dst_inport="West",
                                                 latency = chiplet_link_latency,
                                                 width = chiplet_link_width,
                                                 weight=1))
    
                        print_connection("Router", get_router_id(routers[east_out]),
                                         "Router", get_router_id(routers[west_in]),
                                          link_count,
                                          chiplet_link_latency, chiplet_link_width)
    
                        link_count += 1
    
            # West output to East input links (weight = 1)
            for row in xrange(num_rows):
                for col in xrange(num_columns):
                    if (col + 1 < num_columns):
                        east_in = (cc*num_cpus_per_chiplet) + col + (row * num_columns)
                        west_out = (cc*num_cpus_per_chiplet) +  (col + 1) + (row * num_columns)
                        int_links.append(IntLink(link_id=link_count,
                                                 src_node=routers[west_out],
                                                 dst_node=routers[east_in],
                                                 src_outport="West",
                                                 dst_inport="East",
                                                 latency = chiplet_link_latency,
                                                 width = chiplet_link_width,
                                                 weight=1))
    
                        print_connection("Router", get_router_id(routers[west_out]),
                                         "Router", get_router_id(routers[east_in]),
                                          link_count,
                                          chiplet_link_latency, chiplet_link_width)
    
                        link_count += 1
    
            # North output to South input links (weight = 2)
            for col in xrange(num_columns):
                for row in xrange(num_rows):
                    if (row + 1 < num_rows):
                        north_out = (cc*num_cpus_per_chiplet) + col + (row * num_columns)
                        south_in = (cc*num_cpus_per_chiplet) + col + ((row + 1) * num_columns)
                        int_links.append(IntLink(link_id=link_count,
                                                 src_node=routers[north_out],
                                                 dst_node=routers[south_in],
                                                 src_outport="North",
                                                 dst_inport="South",
                                                 latency = chiplet_link_latency,
                                                 width = chiplet_link_width,
                                                 weight=2))
    
                        print_connection("Router", get_router_id(routers[north_out]),
                                         "Router", get_router_id(routers[south_in]),
                                          link_count,
                                          chiplet_link_latency, chiplet_link_width)
    
                        link_count += 1
    
            # South output to North input links (weight = 2)
            for col in xrange(num_columns):
                for row in xrange(num_rows):
                    if (row + 1 < num_rows):
                        north_in = (cc*num_cpus_per_chiplet) + col + (row * num_columns)
                        south_out = (cc*num_cpus_per_chiplet) + col + ((row + 1) * num_columns)
                        int_links.append(IntLink(link_id=link_count,
                                                 src_node=routers[south_out],
                                                 dst_node=routers[north_in],
                                                 src_outport="South",
                                                 dst_inport="North",
                                                 latency = chiplet_link_latency,
                                                 width = chiplet_link_width,
                                                 weight=2))
    
                        print_connection("Router", get_router_id(routers[south_out]),
                                         "Router", get_router_id(routers[north_in]),
                                          link_count,
                                          chiplet_link_latency, chiplet_link_width)
    
                        link_count += 1
    
        ## Added all CPU chiplet links
        assert(test_num_cpus == len(cpu_nodes))

        ## Connect all chiplets to Xbar
        print "Connecting all Chiplets to Xbar Chiplet:"

        # First connect all CPU chiplets via their Router "0"
        cc_router_id = 0
        for cc in xrange(num_cpu_chiplets):
            # CPU Chiplet to Rtr
            int_links.append(IntLink(link_id=link_count,
                                     src_node=routers[cc_router_id],
                                     dst_node=routers[xbar_id],
                                     latency = interp_link_latency,
                                     width = interp_link_width,
                                     tx_clip = True,
                                     rx_clip = True,
                                     weight=1))
            print_connection("Router", get_router_id(routers[cc_router_id]),
                             "Router", get_router_id(routers[xbar_id]),
                             link_count,
                             interp_link_latency, interp_link_width)
            link_count += 1
    
            # Rtr to CPU chiplet
            int_links.append(IntLink(link_id=link_count,
                                     src_node=routers[xbar_id],
                                     dst_node=routers[cc_router_id],
                                     latency = interp_link_latency,
                                     width = interp_link_width,
                                     tx_clip = True,
                                     rx_clip = True,
                                     weight=1))
            print_connection("Router", get_router_id(routers[xbar_id]),
                             "Router", get_router_id(routers[cc_router_id]),
                             link_count,
                             interp_link_latency, interp_link_width)
            link_count += 1

            cc_router_id += num_cpus_per_chiplet

        # Next, connect all other chiplets to xbar
        # Router id of first L2 chiplet should be same as num_cpus
        assert(l2c_router_start_id == len(cpu_nodes))
        ncc_router_id = l2c_router_start_id

        # Chiplet to Xbar
        for ncc in xrange(num_l2_mc_dma_chiplets):
            int_links.append(IntLink(link_id=link_count,
                                     src_node=routers[ncc_router_id],
                                     dst_node=routers[xbar_id],
                                     latency = interp_link_latency,
                                     width = interp_link_width,
                                     tx_clip = True,
                                     rx_clip = True,
                                     weight=1))
            print_connection("Router", get_router_id(routers[ncc_router_id]),
                             "Router", get_router_id(routers[xbar_id]),
                             link_count,
                             interp_link_latency, interp_link_width)
            link_count += 1

            # Xbar to chiplet
            int_links.append(IntLink(link_id=link_count,
                                     src_node=routers[xbar_id],
                                     dst_node=routers[ncc_router_id],
                                     latency = interp_link_latency,
                                     width = interp_link_width,
                                     tx_clip = True,
                                     rx_clip = True,
                                     weight=1))
            print_connection("Router", get_router_id(routers[xbar_id]),
                             "Router", get_router_id(routers[ncc_router_id]),
                             link_count,
                             interp_link_latency, interp_link_width)
            link_count += 1

            ncc_router_id += 1

        # At the end ncc_router_id should be same as last chiplet, namely xbar
        assert(ncc_router_id == xbar_id)

        network.int_links = int_links


def get_router_id(node) :
    return str(node).split('.')[3].split('routers')[1]

def print_connection(src_type, src_id, dst_type, dst_id, link_id, lat, bw):
    print str(src_type) + "-" + str(src_id) + " connected to " + \
          str(dst_type) + "-" + str(dst_id) + " via Link-" + str(link_id) + \
         " with latency=" + str(lat) + " (cycles)" \
         " and bandwidth=" + str(bw) + " (bits)"
