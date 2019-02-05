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


#ifndef __MEM_RUBY_NETWORK_GARNET2_0_CLIP_HH__
#define __MEM_RUBY_NETWORK_GARNET2_0_CLIP_HH__

#include <iostream>
#include <queue>
#include <vector>

#include "mem/ruby/common/Consumer.hh"
#include "mem/ruby/network/garnet2.0/CommonTypes.hh"
#include "mem/ruby/network/garnet2.0/CreditLink.hh"
#include "mem/ruby/network/garnet2.0/GarnetLink.hh"
#include "mem/ruby/network/garnet2.0/NetworkLink.hh"
#include "mem/ruby/network/garnet2.0/flitBuffer.hh"
#include "params/CLIP.hh"

class GarnetNetwork;

class CLIP: public CreditLink
{
  public:
    typedef CLIPParams Params;
    CLIP(const Params *p);
    ~CLIP();

    void init(CLIP *coBrid, bool clip_en);

    void wakeup();
    void neutralize(int vc, int eCredit);

    void scheduleFlit(flit *t_flit, Cycles latency);
    void flitisizeAndSend(flit *t_flit);

    friend class GarnetNetwork;

  protected:
    // Pointer to co-existing bridge
    // CreditBridge for Network Bridge and vice versa
    CLIP *coBridge;

    // Link connected toBridge
    // could be a source or destination
    // depending on mType
    NetworkLink *nLink;

    // CLIP enable/disable
    bool enable;

    // Type of Bridge
    int mType;

    // Physical and Logical Interface
    Cycles phyIfcLatency;
    Cycles logIfcLatency;

    // Used by Credit Deserializer
    std::vector<int> lenBuffer;
    std::vector<std::queue<int>> extraCredit;

};

#endif // __MEM_RUBY_NETWORK_GARNET2_0_CLIP_HH__
