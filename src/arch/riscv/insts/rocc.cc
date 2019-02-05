/*
 * Authors: Khalid Al-Hawaj
 */

#include "arch/riscv/insts/rocc.hh"

#include <sstream>
#include <string>

#include "arch/riscv/utility.hh"
#include "cpu/static_inst.hh"

namespace RiscvISA
{

std::string
RoccOp::generateDisassembly(Addr pc, const SymbolTable *symtab) const
{
    std::stringstream ss;

    /* Print mnemonic */
    ss << "rocc";

    /* Do we have a destination? */
    if (xd) {
        /* Print that */
        ss << " " << registerName(_destRegIdx[0]) << ",";
    } else {
        /* Destination in the accelerator space */
        ss << " " << RoccRegNames[rd] << ",";
    }

    /* We always have two sources */
    auto incr = 0;

    if (xs1) {
        /* Processor side */
        ss << " " << registerName(_srcRegIdx[incr++]) << ",";
    } else {
        /* Accelerator side */
        ss << " " << RoccRegNames[rs1] << ",";
    }

    if (xs2) {
        /* Processor side */
        ss << " " << registerName(_srcRegIdx[incr++]) << ",";
    } else {
        /* Accelerator side */
        ss << " " << RoccRegNames[rs2] << ",";
    }

    /* Funct */
    ss << " " << funct;

    return ss.str();
}

}
