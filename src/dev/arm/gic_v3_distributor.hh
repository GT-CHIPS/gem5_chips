/*
 * Copyright (c) 2018 Metempsy Technology Consulting
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
 * Authors: Jairo Balart
 */

#ifndef __DEV_ARM_GICV3_DISTRIBUTOR_H__
#define __DEV_ARM_GICV3_DISTRIBUTOR_H__

#include "base/addr_range.hh"
#include "dev/arm/gic_v3.hh"
#include "sim/serialize.hh"

class Gicv3Distributor : public Serializable
{
  private:

    friend class Gicv3Redistributor;
    friend class Gicv3CPUInterface;

  protected:

    Gicv3 * gic;
    const uint32_t itLines;

    enum {
        // Control Register
        GICD_CTLR  = 0x0000,
        // Interrupt Controller Type Register
        GICD_TYPER = 0x0004,
        // Implementer Identification Register
        GICD_IIDR = 0x0008,
        // Error Reporting Status Register
        GICD_STATUSR = 0x0010,
        // Peripheral ID0 Register
        GICD_PIDR0 = 0xffe0,
        // Peripheral ID1 Register
        GICD_PIDR1 = 0xffe4,
        // Peripheral ID2 Register
        GICD_PIDR2 = 0xffe8,
        // Peripheral ID3 Register
        GICD_PIDR3 = 0xffec,
        // Peripheral ID4 Register
        GICD_PIDR4 = 0xffd0,
        // Peripheral ID5 Register
        GICD_PIDR5 = 0xffd4,
        // Peripheral ID6 Register
        GICD_PIDR6 = 0xffd8,
        // Peripheral ID7 Register
        GICD_PIDR7 = 0xffdc,
    };

    // Interrupt Group Registers
    static const AddrRange GICD_IGROUPR;
    // Interrupt Set-Enable Registers
    static const AddrRange GICD_ISENABLER;
    // Interrupt Clear-Enable Registers
    static const AddrRange GICD_ICENABLER;
    // Interrupt Set-Pending Registers
    static const AddrRange GICD_ISPENDR;
    // Interrupt Clear-Pending Registers
    static const AddrRange GICD_ICPENDR;
    // Interrupt Set-Active Registers
    static const AddrRange GICD_ISACTIVER;
    // Interrupt Clear-Active Registers
    static const AddrRange GICD_ICACTIVER;
    // Interrupt Priority Registers
    static const AddrRange GICD_IPRIORITYR;
    // Interrupt Processor Targets Registers
    static const AddrRange GICD_ITARGETSR; // GICv2 legacy
    // Interrupt Configuration Registers
    static const AddrRange GICD_ICFGR;
    // Interrupt Group Modifier Registers
    static const AddrRange GICD_IGRPMODR;
    // Non-secure Access Control Registers
    static const AddrRange GICD_NSACR;
    // SGI Clear-Pending Registers
    static const AddrRange GICD_CPENDSGIR; // GICv2 legacy
    // SGI Set-Pending Registers
    static const AddrRange GICD_SPENDSGIR; // GICv2 legacy
    // Interrupt Routing Registers
    static const AddrRange GICD_IROUTER;

    BitUnion64(IROUTER)
    Bitfield<63, 40> res0_1;
    Bitfield<39, 32> Aff3;
    Bitfield<31> IRM;
    Bitfield<30, 24> res0_2;
    Bitfield<23, 16> Aff2;
    Bitfield<15, 8> Aff1;
    Bitfield<7, 0> Aff0;
    EndBitUnion(IROUTER)

    static const uint32_t GICD_CTLR_ENABLEGRP0 = 1 << 0;
    static const uint32_t GICD_CTLR_ENABLEGRP1NS = 1 << 1;
    static const uint32_t GICD_CTLR_ENABLEGRP1S = 1 << 2;
    static const uint32_t GICD_CTLR_ENABLEGRP1 = 1 << 0;
    static const uint32_t GICD_CTLR_ENABLEGRP1A = 1 << 1;
    static const uint32_t GICD_CTLR_DS = 1 << 6;

    bool ARE;
    bool DS;
    bool EnableGrp1S;
    bool EnableGrp1NS;
    bool EnableGrp0;
    std::vector <uint8_t> irqGroup;
    std::vector <bool> irqEnabled;
    std::vector <bool> irqPending;
    std::vector <bool> irqActive;
    std::vector <uint8_t> irqPriority;
    std::vector <Gicv3::IntTriggerType> irqConfig;
    std::vector <uint8_t> irqGrpmod;
    std::vector <uint8_t> irqNsacr;
    std::vector <IROUTER> irqAffinityRouting;

  public:

    static const uint32_t ADDR_RANGE_SIZE = 0x10000;

    Gicv3Distributor(Gicv3 * gic, uint32_t it_lines);
    ~Gicv3Distributor();
    void init();
    void initState();

    uint64_t read(Addr addr, size_t size, bool is_secure_access);
    void write(Addr addr, uint64_t data, size_t size,
               bool is_secure_access);
    void serialize(CheckpointOut & cp) const override;
    void unserialize(CheckpointIn & cp) override;

    bool
    groupEnabled(Gicv3::GroupId group)
    {
        if (DS == 0) {
            switch (group) {
              case Gicv3::G0S:
                return EnableGrp0;

              case Gicv3::G1S:
                return EnableGrp1S;

              case Gicv3::G1NS:
                return EnableGrp1NS;

              default:
                panic("Gicv3Distributor::groupEnabled(): "
                        "invalid group!\n");
            }
        } else {
            switch (group) {
              case Gicv3::G0S:
                return EnableGrp0;

              case Gicv3::G1S:
              case Gicv3::G1NS:
                return EnableGrp1NS;

              default:
                panic("Gicv3Distributor::groupEnabled(): "
                        "invalid group!\n");
            }
        }
    }

    void sendInt(uint32_t int_id);
    void intDeasserted(uint32_t int_id);
    Gicv3::IntStatus intStatus(uint32_t int_id);
    void updateAndInformCPUInterfaces();
    void update();
    void fullUpdate();
    void activateIRQ(uint32_t int_id);
    void deactivateIRQ(uint32_t int_id);

    inline bool isNotSPI(uint8_t int_id)
    {
        if (int_id < (Gicv3::SGI_MAX + Gicv3::PPI_MAX) || int_id >= itLines) {
            return true;
        } else {
            return false;
        }
    }

    inline bool nsAccessToSecInt(uint8_t int_id, bool is_secure_access)
    {
        return !DS && !is_secure_access && getIntGroup(int_id) != Gicv3::G1NS;
    }

  protected:

    void reset();
    Gicv3::GroupId getIntGroup(int int_id);
};

#endif //__DEV_ARM_GICV3_DISTRIBUTOR_H__
