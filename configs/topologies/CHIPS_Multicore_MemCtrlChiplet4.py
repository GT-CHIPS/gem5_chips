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

# Creates a Mesh topology.
# One L1 (and L2, depending on the protocol) are connected to each router.
# XY routing is enforced (using link weights) to guarantee deadlock freedom.
# 4 Memory controllers are connected to the corners

class CHIPS_Multicore_MemCtrlChiplet4(SimpleTopology):
    description='CHIPS_Multicore_MemCtrlChiplet4'

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


        # Number of routers
        num_routers = len(cpu_nodes) + len(mc_nodes)
        num_rows = options.mesh_rows

        ## Make sure the numbers are correct for a mesh.
        # Obviously the number or rows must be <= the number of cpu routers
        # and evenly divisible.  Also the number of cpus must be a
        # multiple of the number of routers and the number of directories
        # must be four.
        assert(num_rows > 0 and num_rows <= options.num_cpus)
        num_columns = int(len(cpu_nodes) / num_rows)
        assert(num_columns * num_rows == options.num_cpus)
        cpus_per_router, remainder = divmod(len(cpu_nodes), options.num_cpus)
        assert(cpus_per_router == 1)
        assert(remainder == 0)
        assert(len(mc_nodes) == 4)

        # Create the routers
        routers = [Router(router_id=i, latency = router_latency) \
            for i in range(num_routers)]
        network.routers = routers

        # link counter to set unique link ids
        link_count = 0

        # Connect each CPU to the appropriate router
        ext_links = []
        for (i, n) in enumerate(cpu_nodes):
            cntrl_level, router_id = divmod(i, num_routers)
            assert(cntrl_level < cpus_per_router)
            routers[router_id].width = chiplet_link_width # nominal flit size
            ext_links.append(ExtLink(link_id=link_count, ext_node=n,
                                    int_node=routers[router_id],
                                    latency = chiplet_link_latency,
                                    width = chiplet_link_width))
            print_connection("CPU", n.version, "Router", router_id, link_count,\
                              chiplet_link_latency, chiplet_link_width)
            link_count += 1

        # Connect each L2 to the appropriate router
        for (i, n) in enumerate(l2_nodes):
            cntrl_level, router_id = divmod(i, num_routers)
            assert(cntrl_level < cpus_per_router)
            routers[router_id].width = chiplet_link_width # nominal flit size
            ext_links.append(ExtLink(link_id=link_count, ext_node=n,
                                    int_node=routers[router_id],
                                    latency = chiplet_link_latency,
                                    width = chiplet_link_width))
            print_connection("L2", n.version, "Router", router_id, link_count,\
                              chiplet_link_latency, chiplet_link_width)
            link_count += 1


        # Connect the MC nodes to routers
        mc_rtr0_idx = len(cpu_nodes);
        mc_rtr1_idx = len(cpu_nodes)+1;
        mc_rtr2_idx = len(cpu_nodes)+2;
        mc_rtr3_idx = len(cpu_nodes)+3;
        
        ext_links.append(ExtLink(link_id=link_count, ext_node=mc_nodes[0],
                                int_node=routers[mc_rtr0_idx],
                                width = chiplet_link_width,
                                latency = chiplet_link_latency))
        print_connection("MC", mc_nodes[0].version,
                         "Router", get_router_id(routers[mc_rtr0_idx]),
                         link_count, chiplet_link_latency, chiplet_link_width)
        link_count += 1

        ext_links.append(ExtLink(link_id=link_count, ext_node=mc_nodes[1],
                                int_node=routers[mc_rtr1_idx],
                                width = chiplet_link_width,
                                latency = chiplet_link_latency))
        print_connection("MC", mc_nodes[1].version,
                         "Router", get_router_id(routers[mc_rtr1_idx]),
                         link_count, chiplet_link_latency, chiplet_link_width)

        link_count += 1
        ext_links.append(ExtLink(link_id=link_count, ext_node=mc_nodes[2],
                                int_node=routers[mc_rtr2_idx],
                                width = chiplet_link_width,
                                latency = chiplet_link_latency))
        print_connection("MC", mc_nodes[2].version,
                         "Router", get_router_id(routers[mc_rtr2_idx]),
                         link_count, chiplet_link_latency, chiplet_link_width)
        link_count += 1
        ext_links.append(ExtLink(link_id=link_count, ext_node=mc_nodes[3],
                                int_node=routers[mc_rtr3_idx],
                                width = chiplet_link_width,
                                latency = chiplet_link_latency))
        print_connection("MC", mc_nodes[3].version,
                         "Router", get_router_id(routers[mc_rtr3_idx]),
                         link_count, chiplet_link_latency, chiplet_link_width)
        link_count += 1


        # Connect the dma nodes to router connected to MC 0.
        # These should only be DMA nodes.

        dma_rtr_idx = len(cpu_nodes)

        for (i, node) in enumerate(dma_nodes):
            assert(node.type == 'DMA_Controller')
            ext_links.append(ExtLink(link_id=link_count, ext_node=node,
                                     int_node=routers[dma_rtr_idx],
                                     width = chiplet_link_width,
                                     latency = chiplet_link_latency))
            print_connection("DMA", node.version,
                         "Router", get_router_id(routers[dma_rtr_idx]),
                         link_count, chiplet_link_latency, chiplet_link_width)

        network.ext_links = ext_links

        # Create the mesh links.
        int_links = []

        # East output to West input links (weight = 1)
        for row in xrange(num_rows):
            for col in xrange(num_columns):
                if (col + 1 < num_columns):
                    east_out = col + (row * num_columns)
                    west_in = (col + 1) + (row * num_columns)
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
                    east_in = col + (row * num_columns)
                    west_out = (col + 1) + (row * num_columns)
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
                    north_out = col + (row * num_columns)
                    south_in = col + ((row + 1) * num_columns)
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
                    north_in = col + (row * num_columns)
                    south_out = col + ((row + 1) * num_columns)
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


        ## Connect MC routers to Mesh routers
        # MC 0 to Rtr
        int_links.append(IntLink(link_id=link_count,
                                 src_node=routers[mc_rtr0_idx],
                                 dst_node=routers[0],
                                 latency = interp_link_latency,
                                 width = interp_link_width,
                                 tx_clip = True,
                                 rx_clip = True,
                                 weight=1))
        print_connection("Router", get_router_id(routers[mc_rtr0_idx]),
                         "Router", get_router_id(routers[0]),
                         link_count,
                         interp_link_latency, interp_link_width)
        link_count += 1

        # Rtr to MC 0
        int_links.append(IntLink(link_id=link_count,
                                 src_node=routers[0],
                                 dst_node=routers[mc_rtr0_idx],
                                 latency = interp_link_latency,
                                 width = interp_link_width,
                                 tx_clip = True,
                                 rx_clip = True,
                                 weight=1))
        print_connection("Router", get_router_id(routers[0]),
                         "Router", get_router_id(routers[mc_rtr0_idx]),
                         link_count,
                         interp_link_latency, interp_link_width)
        link_count += 1

        # MC 1 to Rtr
        int_links.append(IntLink(link_id=link_count,
                                 src_node=routers[mc_rtr1_idx],
                                 dst_node=routers[num_columns-1],
                                 latency = interp_link_latency,
                                 width = interp_link_width,
                                 tx_clip = True,
                                 rx_clip = True,
                                 weight=1))
        print_connection("Router", get_router_id(routers[mc_rtr1_idx]),
                         "Router", get_router_id(routers[num_columns-1]),
                         link_count,
                         interp_link_latency, interp_link_width)
        link_count += 1

        # Rtr to MC 1
        int_links.append(IntLink(link_id=link_count,
                                 src_node=routers[num_columns - 1],
                                 dst_node=routers[mc_rtr1_idx],
                                 latency = interp_link_latency,
                                 width = interp_link_width,
                                 tx_clip = True,
                                 rx_clip = True,
                                 weight=1))
        print_connection("Router", get_router_id(routers[num_columns - 1]),
                         "Router", get_router_id(routers[mc_rtr1_idx]),
                         link_count,
                         interp_link_latency, interp_link_width)
        link_count += 1

        # MC 2 to Rtr
        int_links.append(IntLink(link_id=link_count,
                                 src_node=routers[mc_rtr2_idx],
                                 dst_node=routers[len(cpu_nodes) - num_columns],
                                 latency = interp_link_latency,
                                 width = interp_link_width,
                                 tx_clip = True,
                                 rx_clip = True,
                                 weight=1))
        print_connection("Router", get_router_id(routers[mc_rtr2_idx]),
                         "Router", get_router_id(routers[len(cpu_nodes) - num_columns]),
                         link_count,
                         interp_link_latency, interp_link_width)
        link_count += 1

        # Rtr to MC 2
        int_links.append(IntLink(link_id=link_count,
                                 src_node=routers[len(cpu_nodes) - num_columns],
                                 dst_node=routers[mc_rtr2_idx],
                                 latency = interp_link_latency,
                                 width = interp_link_width,
                                 tx_clip = True,
                                 rx_clip = True,
                                 weight=1))
        print_connection("Router", get_router_id(routers[len(cpu_nodes) - num_columns]),
                         "Router", get_router_id(routers[mc_rtr2_idx]),
                         link_count,
                         interp_link_latency, interp_link_width)
        link_count += 1

        # MC 3 to Rtr
        int_links.append(IntLink(link_id=link_count,
                                 src_node=routers[mc_rtr3_idx],
                                 dst_node=routers[len(cpu_nodes) - 1],
                                 latency = interp_link_latency,
                                 width = interp_link_width,
                                 tx_clip = True,
                                 rx_clip = True,
                                 weight=1))
        print_connection("Router", get_router_id(routers[mc_rtr3_idx]),
                         "Router", get_router_id(routers[len(cpu_nodes) - 1]),
                         link_count,
                         interp_link_latency, interp_link_width)
        link_count += 1

        # Rtr to MC 3
        int_links.append(IntLink(link_id=link_count,
                                 src_node=routers[len(cpu_nodes) - 1],
                                 dst_node=routers[mc_rtr3_idx],
                                 latency = interp_link_latency,
                                 width = interp_link_width,
                                 tx_clip = True,
                                 rx_clip = True,
                                 weight=1))
        print_connection("Router", get_router_id(routers[len(cpu_nodes) - 1]),
                         "Router", get_router_id(routers[mc_rtr3_idx]),
                         link_count,
                         interp_link_latency, interp_link_width)
        link_count += 1


        network.int_links = int_links


def get_router_id(node) :
    return str(node).split('.')[3].split('routers')[1]

def print_connection(src_type, src_id, dst_type, dst_id, link_id, lat, bw):
    print str(src_type) + "-" + str(src_id) + " connected to " + \
          str(dst_type) + "-" + str(dst_id) + " via Link-" + str(link_id) + \
         " with latency=" + str(lat) + " (cycles)" \
         " and bandwidth=" + str(bw) + " (bits)"
