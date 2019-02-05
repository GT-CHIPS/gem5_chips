/*
 * Authors: Khalid Al-Hawaj
 */

#ifndef __ARCH_GENERIC_ROCCHELPERS_HH__
#define __ARCH_GENERIC_ROCCHELPERS_HH__

#include "rocc/ifcs.hh"
#include "rocc/packets.hh"
#include "rocc/types.hh"

/// Initiate a RoCC request.
template <class XC>
Fault
initiateRoccReq(XC *xc, Trace::InstRecord *traceData,
                void* inst, uint64_t opcode, uint64_t func,
                bool rd_x,  uint64_t rd_id,  uint64_t rd_data,
                bool rs1_x, uint64_t rs1_id, uint64_t rs1_data,
                bool rs2_x, uint64_t rs2_id, uint64_t rs2_data)
{
    /** Create a RoCC packet */
    RoccCmdPtr req = new RoccCmd(inst, opcode, func,
                                 rd_x , rd_id , rd_data ,
                                 rs1_x, rs1_id, rs1_data,
                                 rs2_x, rs2_id, rs2_data);

    /* Use the Execution Context implementation for performing
     * this action. */
    return xc->sendRoccRequest(req);
}

/// Parse a RoCC response.
template <class XC>
Fault
parseRoccResponse(XC *xc, Trace::InstRecord *traceData,
                  PacketPtr pkt,
                  bool &rd_x,  uint64_t &rd_id,  uint64_t &rd_data)
{
    /** Downcast */
    RoccRespPtr resp = (RoccRespPtr)pkt;

    /* Use the Execution Context implementation for performing
     * this action. */
    return xc->parseRoccResponse(resp, rd_x, rd_id, rd_data);
}

#endif /* __ARCH_GENERIC_ROCCHELPERS_HH__ */
