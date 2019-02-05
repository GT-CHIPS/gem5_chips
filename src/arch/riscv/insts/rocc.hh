/*
 * Authors: Khalid Al-Hawaj
 */

#ifndef __ARCH_RISCV_INST_ROCC_HH__
#define __ARCH_RISCV_INST_ROCC_HH__

#include <string>

#include "arch/riscv/insts/static_inst.hh"
#include "cpu/exec_context.hh"
#include "cpu/static_inst.hh"

namespace RiscvISA
{

class RoccOp : public RiscvStaticInst
{
  protected:
    /* Instruction fields */
    bool xd;
    bool xs1;
    bool xs2;

    uint64_t rd;
    uint64_t rs1;
    uint64_t rs2;

    uint64_t funct;
    uint64_t opcode;

  protected:
    RoccOp(const char *mnem, ExtMachInst _machInst, OpClass __opClass)
        : RiscvStaticInst(mnem, _machInst, __opClass)
    {}

    std::string generateDisassembly(
        Addr pc, const SymbolTable *symtab) const override;
};

}

#endif // __ARCH_RISCV_INST_MEM_HH__
