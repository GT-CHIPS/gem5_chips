/*
 * Copyright (c) 2011-2014, 2016-2017 ARM Limited
 * Copyright (c) 2013 Advanced Micro Devices, Inc.
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2002-2005 The Regents of The University of Michigan
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
 * Authors: Steve Reinhardt
 *          Dave Greene
 *          Nathan Binkert
 *          Andrew Bardsley
 */

/**
 * @file
 *
 *  ExecContext bears the exec_context interface for Minor.
 */

#ifndef __CPU_MINOR_EXEC_CONTEXT_HH__
#define __CPU_MINOR_EXEC_CONTEXT_HH__

#include "cpu/exec_context.hh"
#include "cpu/minor/execute.hh"
#include "cpu/minor/pipeline.hh"
#include "cpu/base.hh"
#include "cpu/simple_thread.hh"
#include "mem/request.hh"
#include "debug/MinorExecute.hh"

namespace Minor
{

/* Forward declaration of Execute */
class Execute;

/** ExecContext bears the exec_context interface for Minor.  This nicely
 *  separates that interface from other classes such as Pipeline, MinorCPU
 *  and DynMinorInst and makes it easier to see what state is accessed by it.
 */
class ExecContext : public ::ExecContext
{
  public:
    MinorCPU &cpu;

    /** ThreadState object, provides all the architectural state. */
    SimpleThread &thread;

    /** The execute stage so we can peek at its contents. */
    Execute &execute;

    /** Instruction for the benefit of memory operations and for PC */
    MinorDynInstPtr inst;

    ExecContext (
        MinorCPU &cpu_,
        SimpleThread &thread_, Execute &execute_,
        MinorDynInstPtr inst_) :
        cpu(cpu_),
        thread(thread_),
        execute(execute_),
        inst(inst_)
    {
        DPRINTF(MinorExecute, "ExecContext setting PC: %s\n", inst->pc);
        pcState(inst->pc);
        setPredicate(true);
        thread.setIntReg(TheISA::ZeroReg, 0);
#if THE_ISA == ALPHA_ISA
        thread.setFloatReg(TheISA::ZeroReg, 0);
#endif
    }

    Fault
    initiateMemRead(Addr addr, unsigned int size,
                    Request::Flags flags) override
    {
        execute.getLSQ().pushRequest(inst, true /* load */, nullptr,
            size, addr, flags, NULL, nullptr);
        return NoFault;
    }

    Fault
    writeMem(uint8_t *data, unsigned int size, Addr addr,
             Request::Flags flags, uint64_t *res) override
    {
        execute.getLSQ().pushRequest(inst, false /* store */, data,
            size, addr, flags, res, nullptr);
        return NoFault;
    }

    Fault
    initiateMemAMO(Addr addr, unsigned int size, Request::Flags flags,
                   AtomicOpFunctor *amo_op) override
    {
        // AMO requests are pushed through the store path
        execute.getLSQ().pushRequest(inst, false /* amo */, nullptr,
            size, addr, flags, nullptr, amo_op);
        return NoFault;
    }

    RegVal
    readIntRegOperand(const StaticInst *si, int idx) override
    {
        const RegId& reg = si->srcRegIdx(idx);
        assert(reg.isIntReg());
        return thread.readIntReg(reg.index());
    }

    RegVal
    readFloatRegOperandBits(const StaticInst *si, int idx) override
    {
        const RegId& reg = si->srcRegIdx(idx);
        assert(reg.isFloatReg());
        return thread.readFloatReg(reg.index());
    }

    const TheISA::VecRegContainer &
    readVecRegOperand(const StaticInst *si, int idx) const override
    {
        const RegId& reg = si->srcRegIdx(idx);
        assert(reg.isVecReg());
        return thread.readVecReg(reg);
    }

    TheISA::VecRegContainer &
    getWritableVecRegOperand(const StaticInst *si, int idx) override
    {
        const RegId& reg = si->destRegIdx(idx);
        assert(reg.isVecReg());
        return thread.getWritableVecReg(reg);
    }

    TheISA::VecElem
    readVecElemOperand(const StaticInst *si, int idx) const override
    {
        const RegId& reg = si->srcRegIdx(idx);
        assert(reg.isVecElem());
        return thread.readVecElem(reg);
    }

    const TheISA::VecPredRegContainer&
    readVecPredRegOperand(const StaticInst *si, int idx) const override
    {
        const RegId& reg = si->srcRegIdx(idx);
        assert(reg.isVecPredReg());
        return thread.readVecPredReg(reg);
    }

    TheISA::VecPredRegContainer&
    getWritableVecPredRegOperand(const StaticInst *si, int idx) override
    {
        const RegId& reg = si->destRegIdx(idx);
        assert(reg.isVecPredReg());
        return thread.getWritableVecPredReg(reg);
    }

    void
    setIntRegOperand(const StaticInst *si, int idx, RegVal val) override
    {
        const RegId& reg = si->destRegIdx(idx);
        assert(reg.isIntReg());
        thread.setIntReg(reg.index(), val);
    }

    void
    setFloatRegOperandBits(const StaticInst *si, int idx, RegVal val) override
    {
        const RegId& reg = si->destRegIdx(idx);
        assert(reg.isFloatReg());
        thread.setFloatReg(reg.index(), val);
    }

    void
    setVecRegOperand(const StaticInst *si, int idx,
                     const TheISA::VecRegContainer& val) override
    {
        const RegId& reg = si->destRegIdx(idx);
        assert(reg.isVecReg());
        thread.setVecReg(reg, val);
    }

    void
    setVecPredRegOperand(const StaticInst *si, int idx,
                         const TheISA::VecPredRegContainer& val) override
    {
        const RegId& reg = si->destRegIdx(idx);
        assert(reg.isVecPredReg());
        thread.setVecPredReg(reg, val);
    }

    /** Vector Register Lane Interfaces. */
    /** @{ */
    /** Reads source vector 8bit operand. */
    ConstVecLane8
    readVec8BitLaneOperand(const StaticInst *si, int idx) const
                            override
    {
        const RegId& reg = si->srcRegIdx(idx);
        assert(reg.isVecReg());
        return thread.readVec8BitLaneReg(reg);
    }

    /** Reads source vector 16bit operand. */
    ConstVecLane16
    readVec16BitLaneOperand(const StaticInst *si, int idx) const
                            override
    {
        const RegId& reg = si->srcRegIdx(idx);
        assert(reg.isVecReg());
        return thread.readVec16BitLaneReg(reg);
    }

    /** Reads source vector 32bit operand. */
    ConstVecLane32
    readVec32BitLaneOperand(const StaticInst *si, int idx) const
                            override
    {
        const RegId& reg = si->srcRegIdx(idx);
        assert(reg.isVecReg());
        return thread.readVec32BitLaneReg(reg);
    }

    /** Reads source vector 64bit operand. */
    ConstVecLane64
    readVec64BitLaneOperand(const StaticInst *si, int idx) const
                            override
    {
        const RegId& reg = si->srcRegIdx(idx);
        assert(reg.isVecReg());
        return thread.readVec64BitLaneReg(reg);
    }

    /** Write a lane of the destination vector operand. */
    template <typename LD>
    void
    setVecLaneOperandT(const StaticInst *si, int idx, const LD& val)
    {
        const RegId& reg = si->destRegIdx(idx);
        assert(reg.isVecReg());
        return thread.setVecLane(reg, val);
    }
    virtual void
    setVecLaneOperand(const StaticInst *si, int idx,
            const LaneData<LaneSize::Byte>& val) override
    {
        setVecLaneOperandT(si, idx, val);
    }
    virtual void
    setVecLaneOperand(const StaticInst *si, int idx,
            const LaneData<LaneSize::TwoByte>& val) override
    {
        setVecLaneOperandT(si, idx, val);
    }
    virtual void
    setVecLaneOperand(const StaticInst *si, int idx,
            const LaneData<LaneSize::FourByte>& val) override
    {
        setVecLaneOperandT(si, idx, val);
    }
    virtual void
    setVecLaneOperand(const StaticInst *si, int idx,
            const LaneData<LaneSize::EightByte>& val) override
    {
        setVecLaneOperandT(si, idx, val);
    }
    /** @} */

    void
    setVecElemOperand(const StaticInst *si, int idx,
                      const TheISA::VecElem val) override
    {
        const RegId& reg = si->destRegIdx(idx);
        assert(reg.isVecElem());
        thread.setVecElem(reg, val);
    }

    bool
    readPredicate() const override
    {
        return thread.readPredicate();
    }

    void
    setPredicate(bool val) override
    {
        thread.setPredicate(val);
    }

    TheISA::PCState
    pcState() const override
    {
        return thread.pcState();
    }

    void
    pcState(const TheISA::PCState &val) override
    {
        thread.pcState(val);
    }

    RegVal
    readMiscRegNoEffect(int misc_reg) const
    {
        return thread.readMiscRegNoEffect(misc_reg);
    }

    RegVal
    readMiscReg(int misc_reg) override
    {
        return thread.readMiscReg(misc_reg);
    }

    void
    setMiscReg(int misc_reg, RegVal val) override
    {
        thread.setMiscReg(misc_reg, val);
    }

    RegVal
    readMiscRegOperand(const StaticInst *si, int idx) override
    {
        const RegId& reg = si->srcRegIdx(idx);
        assert(reg.isMiscReg());
        return thread.readMiscReg(reg.index());
    }

    void
    setMiscRegOperand(const StaticInst *si, int idx, RegVal val) override
    {
        const RegId& reg = si->destRegIdx(idx);
        assert(reg.isMiscReg());
        return thread.setMiscReg(reg.index(), val);
    }

    Fault
    hwrei() override
    {
#if THE_ISA == ALPHA_ISA
        return thread.hwrei();
#else
        return NoFault;
#endif
    }

    bool
    simPalCheck(int palFunc) override
    {
#if THE_ISA == ALPHA_ISA
        return thread.simPalCheck(palFunc);
#else
        return false;
#endif
    }

    void
    syscall(int64_t callnum, Fault *fault) override
    {
        if (FullSystem)
            panic("Syscall emulation isn't available in FS mode.\n");

        thread.syscall(callnum, fault);
    }

    ThreadContext *tcBase() override { return thread.getTC(); }

    /* @todo, should make stCondFailures persistent somewhere */
    unsigned int readStCondFailures() const override { return 0; }
    void setStCondFailures(unsigned int st_cond_failures) override {}

    ContextID contextId() { return thread.contextId(); }
    /* ISA-specific (or at least currently ISA singleton) functions */

    /* X86: TLB twiddling */
    void
    demapPage(Addr vaddr, uint64_t asn) override
    {
        thread.getITBPtr()->demapPage(vaddr, asn);
        thread.getDTBPtr()->demapPage(vaddr, asn);
    }

    RegVal
    readCCRegOperand(const StaticInst *si, int idx) override
    {
        const RegId& reg = si->srcRegIdx(idx);
        assert(reg.isCCReg());
        return thread.readCCReg(reg.index());
    }

    void
    setCCRegOperand(const StaticInst *si, int idx, RegVal val) override
    {
        const RegId& reg = si->destRegIdx(idx);
        assert(reg.isCCReg());
        thread.setCCReg(reg.index(), val);
    }

    void
    demapInstPage(Addr vaddr, uint64_t asn)
    {
        thread.getITBPtr()->demapPage(vaddr, asn);
    }

    void
    demapDataPage(Addr vaddr, uint64_t asn)
    {
        thread.getDTBPtr()->demapPage(vaddr, asn);
    }

    BaseCPU *getCpuPtr() { return &cpu; }

    /* MIPS: other thread register reading/writing */
    RegVal
    readRegOtherThread(const RegId &reg, ThreadID tid=InvalidThreadID)
    {
        SimpleThread *other_thread = (tid == InvalidThreadID
            ? &thread : cpu.threads[tid]);

        switch (reg.classValue()) {
            case IntRegClass:
                return other_thread->readIntReg(reg.index());
                break;
            case FloatRegClass:
                return other_thread->readFloatReg(reg.index());
                break;
            case MiscRegClass:
                return other_thread->readMiscReg(reg.index());
            default:
                panic("Unexpected reg class! (%s)",
                      reg.className());
                return 0;
        }
    }

    void
    setRegOtherThread(const RegId &reg, RegVal val,
                      ThreadID tid=InvalidThreadID)
    {
        SimpleThread *other_thread = (tid == InvalidThreadID
            ? &thread : cpu.threads[tid]);

        switch (reg.classValue()) {
            case IntRegClass:
                return other_thread->setIntReg(reg.index(), val);
                break;
            case FloatRegClass:
                return other_thread->setFloatReg(reg.index(), val);
                break;
            case MiscRegClass:
                return other_thread->setMiscReg(reg.index(), val);
            default:
                panic("Unexpected reg class! (%s)",
                      reg.className());
        }
    }

  public:
    // monitor/mwait funtions
    void armMonitor(Addr address) override
    { getCpuPtr()->armMonitor(inst->id.threadId, address); }

    bool mwait(PacketPtr pkt) override
    { return getCpuPtr()->mwait(inst->id.threadId, pkt); }

    void mwaitAtomic(ThreadContext *tc) override
    { return getCpuPtr()->mwaitAtomic(inst->id.threadId, tc, thread.dtb); }

    AddressMonitor *getAddrMonitor() override
    { return getCpuPtr()->getCpuAddrMonitor(inst->id.threadId); }
};

}

#endif /* __CPU_MINOR_EXEC_CONTEXT_HH__ */
