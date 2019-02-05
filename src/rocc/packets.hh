#ifndef __ROCC_PACKETS_HH_
#define __ROCC_PACKETS_HH_

/*
 * Author: Khalid Al-Hawaj
 */

/** gem5 includes */
#include "mem/packet.hh"

/** Generic Types */
#include "rocc/types.hh"

/** Generic Class */
class RoccPacket : public Packet {

  public:
    /* Internal fields */
    void* inst;

    /* Constructor */
    RoccPacket(void* inst_, Command type_) :
      Packet(new Request(), type_),
      inst(inst_)
      {}

    /* Print thingy ... */
    std::string print() const {
      std::ostringstream o;

      ccprintf(o, ":)");

      std::string ret = o.str();

      return ret;
    }

};

/** Actual Definition */
class RoccCmd : public RoccPacket {

  public:
    typedef uint64_t funct_t ;
    typedef uint64_t opcode_t;

    /* Internal fields */
    rocc_reg_t rs1;
    rocc_reg_t rs2;
    rocc_reg_t rd;

    funct_t    funct;
    opcode_t   opcode;

    /* Constructor */
    RoccCmd(void* inst_,
            opcode_t   _opcode, funct_t    _funct,
            rocc_reg_t _rd,     rocc_reg_t _rs1,
            rocc_reg_t _rs2) :
      RoccPacket(inst_, MemCmd::GenericRequest)
      {
        // Stuff
        opcode = _opcode;
        funct  = _funct;

        // Other stuff
        rd.x     = _rd.x;
        rd.id    = _rd.id;
        rd.data  = _rd.data;

        rs1.x    = _rs1.x;
        rs1.id   = _rs1.id;
        rs1.data = _rs1.data;

        rs2.x    = _rs2.x;
        rs2.id   = _rs2.id;
        rs2.data = _rs2.data;
      }

    RoccCmd(void* inst_,
            opcode_t   _opcode, funct_t    _funct,
            bool _rd_x , uint64_t _rd_id , uint64_t _rd_data ,
            bool _rs1_x, uint64_t _rs1_id, uint64_t _rs1_data,
            bool _rs2_x, uint64_t _rs2_id, uint64_t _rs2_data) :
      RoccPacket(inst_, MemCmd::GenericRequest)
      {
        // Stuff
        opcode = _opcode;
        funct  = _funct;

        // Other stuff
        rd.x     = _rd_x;
        rd.id    = _rd_id;
        rd.data  = _rd_data;

        rs1.x    = _rs1_x;
        rs1.id   = _rs1_id;
        rs1.data = _rs1_data;

        rs2.x    = _rs2_x;
        rs2.id   = _rs2_id;
        rs2.data = _rs2_data;
      }

    /* Print thingy ... */
    std::string print() const {
      std::ostringstream o;

      ccprintf(o, ":)");

      std::string ret = o.str();

      return ret;
    }

};

/* Pointer definition */
typedef RoccCmd* RoccCmdPtr;

/* Actual Definition */
class RoccResp : public RoccPacket {

  public:
    /* Internal fields */
    rocc_reg_t rd;

    /* Constructor */
    RoccResp(void* inst_, rocc_reg_t _rd) :
      RoccPacket(inst_, MemCmd::GenericResponse)
      {
        // Other stuff
        rd.x     = _rd.x;
        rd.id    = _rd.id;
        rd.data  = _rd.data;
      }

    /* Constructor */
    RoccResp(void* inst_,
             bool _rd_x , uint64_t _rd_id , uint64_t _rd_data) :
      RoccPacket(inst_, MemCmd::GenericResponse)
      {
        // Other stuff
        rd.x     = _rd_x;
        rd.id    = _rd_id;
        rd.data  = _rd_data;
      }

    /* Print thingy ... */
    std::string print() const {
      std::ostringstream o;

      ccprintf(o, ":)");

      std::string ret = o.str();

      return ret;
    }

};

/* Pointer definition */
typedef RoccResp* RoccRespPtr;

#endif /* __ROCC_PACKETS_HH_ */
