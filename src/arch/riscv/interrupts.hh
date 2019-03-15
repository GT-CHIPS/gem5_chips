/*
 * Copyright (c) 2011 Google
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
 * Authors: Gabe Black
 */

#ifndef __ARCH_RISCV_INTERRUPT_HH__
#define __ARCH_RISCV_INTERRUPT_HH__

#include <bitset>
#include <memory>

#include "arch/riscv/faults.hh"
#include "arch/riscv/registers.hh"
#include "base/logging.hh"
#include "cpu/thread_context.hh"
#include "debug/Interrupt.hh"
#include "params/RiscvInterrupts.hh"
#include "sim/sim_object.hh"

class BaseCPU;
class ThreadContext;

namespace RiscvISA {

/*
 * This is based on version 1.10 of the RISC-V privileged ISA reference,
 * chapter 3.1.14.
 */
class Interrupts : public SimObject
{
  private:
    BaseCPU * cpu;
    std::bitset<NumInterruptTypes> ip;
    std::bitset<NumInterruptTypes> ie;

  public:
    typedef RiscvInterruptsParams Params;

    const Params *
    params() const
    {
        return dynamic_cast<const Params *>(_params);
    }

    Interrupts(Params * p) : SimObject(p), cpu(nullptr), ip(0), ie(0) {}

    void setCPU(BaseCPU * _cpu) { cpu = _cpu; }

    std::bitset<NumInterruptTypes>
    globalMask(ThreadContext *tc) const
    {
        INTERRUPT mask = 0;
        STATUS status = tc->readMiscReg(MISCREG_STATUS);
        if (status.mie)
            mask.mei = mask.mti = mask.msi = 1;
        if (status.sie)
            mask.sei = mask.sti = mask.ssi = 1;
        if (status.uie)
            mask.uei = mask.uti = mask.usi = 1;
        return std::bitset<NumInterruptTypes>(mask);
    }

    bool checkInterrupt(int num) const { return ip[num] && ie[num]; }
    bool checkInterrupts(ThreadContext *tc) const
    {
        return (ip & ie & globalMask(tc)).any();
    }

    Fault
    getInterrupt(ThreadContext *tc) const
    {
        assert(checkInterrupts(tc));
        std::bitset<NumInterruptTypes> mask = globalMask(tc);
        for (int c = 0; c < NumInterruptTypes; c++)
            if (checkInterrupt(c) && mask[c])
                return std::make_shared<InterruptFault>(c);
        return NoFault;
    }

    void updateIntrInfo(ThreadContext *tc) {}

    void
    post(int int_num, int index)
    {
        DPRINTF(Interrupt, "Interrupt %d:%d posted\n", int_num, index);
        ip[int_num] = true;
    }

    void
    clear(int int_num, int index)
    {
        DPRINTF(Interrupt, "Interrupt %d:%d cleared\n", int_num, index);
        ip[int_num] = false;
    }

    void
    clearAll()
    {
        DPRINTF(Interrupt, "All interrupts cleared\n");
        ip = 0;
    }

    uint64_t readIP() const { return (uint64_t)ip.to_ulong(); }
    uint64_t readIE() const { return (uint64_t)ie.to_ulong(); }
    void setIP(const uint64_t& val) { ip = val; }
    void setIE(const uint64_t& val) { ie = val; }

    void
    serialize(CheckpointOut &cp) const
    {
        SERIALIZE_SCALAR(ip.to_ulong());
        SERIALIZE_SCALAR(ie.to_ulong());
    }

    void
    unserialize(CheckpointIn &cp)
    {
        long reg;
        UNSERIALIZE_SCALAR(reg);
        ip = reg;
        UNSERIALIZE_SCALAR(reg);
        ie = reg;
    }
};

} // namespace RiscvISA

#endif // __ARCH_RISCV_INTERRUPT_HH__
