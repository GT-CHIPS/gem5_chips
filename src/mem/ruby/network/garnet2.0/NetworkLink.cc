/*
 * Copyright (c) 2008 Princeton University
 * Copyright (c) 2016 Georgia Institute of Technology
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Niket Agarwal
 *          Tushar Krishna
 */


#include "mem/ruby/network/garnet2.0/NetworkLink.hh"

#include "mem/ruby/network/garnet2.0/CreditLink.hh"

NetworkLink::NetworkLink(const Params *p)
    : ClockedObject(p), Consumer(this), m_id(p->link_id),
      m_type(NUM_LINK_TYPES_),
      m_latency(p->link_latency),
      linkBuffer(new flitBuffer()), link_consumer(nullptr),
      link_srcQueue(nullptr), m_link_utilized(0),
      m_vc_load(p->vcs_per_vnet * p->virt_nets)
{
    int num_vnets = (p->supported_vnets).size();
    assert(num_vnets > 0);
    mVnets.resize(num_vnets);
    bitWidth = p->width;
    for (int i = 0; i < num_vnets; i++) {
        mVnets[i] = p->supported_vnets[i];
    }
    DPRINTF(RubyNetwork,"Created with bitwidth:%d\n", bitWidth);
}

NetworkLink::~NetworkLink()
{
    delete linkBuffer;
}

void
NetworkLink::setLinkConsumer(Consumer *consumer)
{
    link_consumer = consumer;
}

void
NetworkLink::setSourceQueue(flitBuffer *srcQueue, ClockedObject *srcClockObj)
{
    link_srcQueue = srcQueue;
    src_object = srcClockObj;
}

void
NetworkLink::wakeup()
{
    DPRINTF(RubyNetwork, "Woke up to transfer from %s to %s\n",
    src_object->name(), link_consumer->getName());
    assert(curTick() == clockEdge());
    if (link_srcQueue->isReady(curTick())) {
        flit *t_flit = link_srcQueue->getTopFlit();
        if (m_type != NUM_LINK_TYPES_) {
            // Only for assertions and debug messages
            DPRINTF(RubyNetwork, "Transmitting at %ld :%s\n",
                clockEdge(m_latency), *t_flit);
            assert(t_flit->m_width == bitWidth);
            assert((std::find(mVnets.begin(), mVnets.end(),
                t_flit->get_vnet()) != mVnets.end()) ||
                (std::find(mVnets.begin(), mVnets.end(), -1) != mVnets.end()));
        }
        t_flit->set_time(clockEdge(m_latency));
        linkBuffer->insert(t_flit);
        link_consumer->scheduleEventAbsolute(clockEdge(m_latency));
        m_link_utilized++;
        m_vc_load[t_flit->get_vc()]++;
    }

    if (!link_srcQueue->isEmpty()) {
        scheduleEvent(Cycles(1));;
    }
}

void
NetworkLink::resetStats()
{
    for (int i = 0; i < m_vc_load.size(); i++) {
        m_vc_load[i] = 0;
    }

    m_link_utilized = 0;
}

NetworkLink *
NetworkLinkParams::create()
{
    return new NetworkLink(this);
}

CreditLink *
CreditLinkParams::create()
{
    return new CreditLink(this);
}

uint32_t
NetworkLink::functionalWrite(Packet *pkt)
{
    return linkBuffer->functionalWrite(pkt);
}
