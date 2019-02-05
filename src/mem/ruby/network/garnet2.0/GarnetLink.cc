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


#include "mem/ruby/network/garnet2.0/GarnetLink.hh"

#include "debug/RubyNetwork.hh"
#include "mem/ruby/network/garnet2.0/CreditLink.hh"
#include "mem/ruby/network/garnet2.0/CLIP.hh"
#include "mem/ruby/network/garnet2.0/NetworkLink.hh"

GarnetIntLink::GarnetIntLink(const Params *p)
    : BasicIntLink(p)
{
    // Uni-directional

    m_network_link = p->network_link;
    m_credit_link = p->credit_link;

    txClipEn = p->tx_clip;
    rxClipEn = p->rx_clip;

    txNetBridge = p->tx_net_bridge;
    rxNetBridge = p->rx_net_bridge;

    txCredBridge = p->tx_cred_bridge;
    rxCredBridge = p->rx_cred_bridge;

}

void
GarnetIntLink::init()
{
    txNetBridge->init(txCredBridge, txClipEn);
    rxNetBridge->init(rxCredBridge, rxClipEn);
    txCredBridge->init(txNetBridge, txClipEn);
    rxCredBridge->init(rxNetBridge, rxClipEn);
}

void
GarnetIntLink::print(std::ostream& out) const
{
    out << name();
}

GarnetIntLink *
GarnetIntLinkParams::create()
{
    return new GarnetIntLink(this);
}

GarnetExtLink::GarnetExtLink(const Params *p)
    : BasicExtLink(p)
{
    // Bi-directional

    // In
    m_network_links[0] = p->network_links[0];
    m_credit_links[0] = p->credit_links[0];

    // Out
    m_network_links[1] = p->network_links[1];
    m_credit_links[1] = p->credit_links[1];

    nicClipEn = p->nic_clip;
    rtrClipEn = p->rtr_clip;

    rtrNetBridge[0] = p->rtr_net_bridge[0];
    nicNetBridge[0] = p->nic_net_bridge[0];

    rtrNetBridge[1] = p->rtr_net_bridge[1];
    nicNetBridge[1] = p->nic_net_bridge[1];

    rtrCredBridge[0] = p->rtr_cred_bridge[0];
    nicCredBridge[0] = p->nic_cred_bridge[0];

    rtrCredBridge[1] = p->rtr_cred_bridge[1];
    nicCredBridge[1] = p->nic_cred_bridge[1];

}

void
GarnetExtLink::init()
{
    nicNetBridge[0]->init(nicCredBridge[0], nicClipEn);
    rtrNetBridge[0]->init(rtrCredBridge[0], rtrClipEn);
    nicNetBridge[1]->init(nicCredBridge[1], nicClipEn);
    rtrNetBridge[1]->init(rtrCredBridge[1], rtrClipEn);

    nicCredBridge[0]->init(nicNetBridge[0], nicClipEn);
    rtrCredBridge[0]->init(rtrNetBridge[0], rtrClipEn);
    nicCredBridge[1]->init(nicNetBridge[1], nicClipEn);
    rtrCredBridge[1]->init(rtrNetBridge[1], rtrClipEn);
}

void
GarnetExtLink::print(std::ostream& out) const
{
    out << name();
}

GarnetExtLink *
GarnetExtLinkParams::create()
{
    return new GarnetExtLink(this);
}
