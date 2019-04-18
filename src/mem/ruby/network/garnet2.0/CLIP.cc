/*
 * Copyright (c) 2018 Advanced Micro Devices, Inc.
 * Copyright (c) 2018 Georgia Institute of Technology
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
 * Authors: Srikant Bharadwaj
 *          Tushar Krishna
 */


#include "mem/ruby/network/garnet2.0/CLIP.hh"

#include <cmath>

#include "debug/RubyNetwork.hh"
#include "params/GarnetIntLink.hh"

CLIP::CLIP(const Params *p)
    :CreditLink(p)
{
    enable = true;
    mType = p->vtype;

    phyIfcLatency = p->phys_latency;
    logIfcLatency = p->logic_latency;

    nLink = p->link;
    if (mType == FROM_LINK_) {
        nLink->setLinkConsumer(this);
        setSourceQueue(nLink->getBuffer(), nLink);
    } else if (mType == TO_LINK_) {
        nLink->setSourceQueue(linkBuffer, this);
        setLinkConsumer(nLink);
    } else {
        // CDC type must be set
        panic("CDC type must be set");
    }

    lenBuffer.resize(p->vcs_per_vnet * p->virt_nets);
    extraCredit.resize(p->vcs_per_vnet * p->virt_nets);
}

void
CLIP::init(CLIP *coBrid, bool clip_en)
{
    coBridge = coBrid;
    enable = clip_en;
}

CLIP::~CLIP()
{
}

void
CLIP::scheduleFlit(flit *t_flit, Cycles latency)
{
    Cycles totLatency = latency + phyIfcLatency;

    t_flit->set_time(link_consumer->getObject()->clockEdge(totLatency));
    linkBuffer->insert(t_flit);
    link_consumer->scheduleEvent(totLatency);
}

void
CLIP::neutralize(int vc, int eCredit)
{
    extraCredit[vc].push(eCredit);
}

void
CLIP::flitisizeAndSend(flit *t_flit)
{
    // Serialize-Deserialize only if CLIP is enabled
    if (enable) {
        // Calculate the target-width
        int target_width = bitWidth;
        int cur_width = nLink->bitWidth;
        if (mType == TO_LINK_) {
            target_width = nLink->bitWidth;
            cur_width = bitWidth;
        }

        DPRINTF(RubyNetwork, "Target width: %d Current: %d\n",
            target_width, cur_width);
        assert(target_width != cur_width);

        int vc = t_flit->get_vc();

        if (target_width > cur_width) {
            // Deserialize
            // This deserializer combines flits from the
            // same message together
            // TODO: Allow heterogenous flits
            int num_flits = 0;
            if (t_flit->get_type() == CREDIT_) {
                num_flits = (int)ceil((float)target_width/(float)cur_width);
            } else if ((t_flit->get_type() == TAIL_ ||
                       t_flit->get_type() == HEAD_TAIL_)
                       && lenBuffer[vc]==0) {
                num_flits = 1;
            } else {
                num_flits = (int)ceil((float)std::min((t_flit->msgSize)*8,
                                       target_width)/(float)cur_width);
            }
            assert(num_flits > 0);

            DPRINTF(RubyNetwork, "Deserialize :%d -----> %d "
                "num-flits-needed:%d vc:%d\n",
                cur_width, target_width, num_flits, vc);

            // lenBuffer acts as the buffer for deserialization

            lenBuffer[vc]++;
            flit *fl = t_flit->deserialize(lenBuffer[vc], num_flits,
                target_width);

            if (t_flit->get_type() != CREDIT_ && fl) {
                coBridge->neutralize(vc, num_flits);
            }

            // Schedule only if we are done deserializing
            if (fl) {
                DPRINTF(RubyNetwork, "Scheduling a flit\n");
                lenBuffer[vc] = 0;
                scheduleFlit(fl, logIfcLatency);
            }
            // Delete this flit, new flit is sent in any case
            delete t_flit;
        } else {
            // Serialize
            DPRINTF(RubyNetwork, "Serializing flit :%d -----> %d "
            "(vc:%d, Original Message Size: %d)\n",
                cur_width, target_width, vc, t_flit->msgSize);

            int num_parts = 0;
            if (t_flit->get_type() == CREDIT_) {
                assert(extraCredit[vc].front());
                num_parts = extraCredit[vc].front();
                extraCredit[vc].pop();
            } else {
                num_parts = (int)ceil((float)cur_width/
                            (float)target_width);
            }
            assert(num_parts > 0);

            DPRINTF(RubyNetwork, "Serialized into %d parts\n", num_parts);
            // Schedule all the flits
            // num_flits could be zero for credits
            for (int i = 0; i < num_parts; i++) {
                // Ignore neutralized credits
                flit *fl = t_flit->serialize(i, num_parts, target_width);
                scheduleFlit(fl, logIfcLatency);
            }
            // Delete this flit, new flit is sent in any case
            delete t_flit;
        }
        return;
    }

    // If only CDC is enabled schedule it
    scheduleFlit(t_flit, Cycles(0));
}
void
CLIP::wakeup()
{
    flit *t_flit;

    if (link_srcQueue->isReady(curTick())) {
        t_flit = link_srcQueue->getTopFlit();
        DPRINTF(RubyNetwork, "Recieved flit %s\n", *t_flit);
        flitisizeAndSend(t_flit);
        return;
    }
    assert(!link_srcQueue->getSize());
}

CLIP *
CLIPParams::create()
{
    return new CLIP(this);
}
