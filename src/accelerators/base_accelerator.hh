/**
 * Author: Shunning Jiang
 *         Khalid Al-Hawaj (mods)
 */

/**
 * @file
 *
 *  This file is adapted from Shreesha Srinath's xcel impl.
 *  The base accelerator implements necessary details for communicating
 *  with the processor. The hope is that developers should inherit this
 *  class and start to play around with the accelerator implementation
 *  without worrying about the details.
 *
 */

#ifndef ACCELERATORS_BASE_ACCELERATOR_HH
#define ACCELERATORS_BASE_ACCELERATOR_HH

/* Interfaces */
#include "rocc/packets.hh"
#include "rocc/types.hh"

/* gem5 imports */
#include "cpu/base.hh"
#include "mem/mem_object.hh"
#include "mem/port.hh"

/* Accelerator parameters */
#include "params/BaseAccelerator.hh"

/* Accelerator debug flags */
#include "debug/ACCEL.hh"

struct BaseAcceleratorParams;

class BaseAccelerator : public MemObject {
  protected:

    // Declare slave accelerator ports and master memory ports
    // Assume the accelerator access both icache (like lane-based xcel)
    // and dcache (all xcels)

    class RoccPort : public SlavePort {
      protected:
        BaseAccelerator *owner;

        bool recvTimingReq(PacketPtr pkt) override
        {
            return owner->recvRoccRequest((RoccCmd*)pkt);
        }
        void recvRespRetry() override {}

        /** Implement these pure functions */
        AddrRangeList getAddrRanges() const { return {}; }
        Tick recvAtomic(PacketPtr pkt) { return curTick(); };
        void recvFunctional(PacketPtr pkt) {}

      public:
        RoccPort(std::string name, BaseAccelerator *xcel_) :
          SlavePort(name, xcel_), owner(xcel_) { }
    };

    /* Event class to tick the MemObject
     * Accelerator event that wraps around xtick */
    class AcceleratorEvent : public Event
    {
      private:
        BaseAccelerator* owner;
      public:
        AcceleratorEvent(BaseAccelerator *owner);
        void process();
        const char *description() const;
    };

    AcceleratorEvent event;

  protected:
    /* Internal state for interface */
    bool ifcsNeedsRetry;

    /* Owner CPU */
    BaseCPU *cpu;

    /** API wrapper for the accelerator convinence */
    bool sendAcceleratorResp(void* id_,
                             bool reg_x_, uint64_t reg_id_, uint64_t data_);
    virtual
    bool recvAcceleratorReq(void* id_,
                                 uint64_t opcode_, uint64_t funct_,
                            bool rd_x,
                                 uint64_t rd_id,  uint64_t rd_data,
                            bool rs1_x,
                                 uint64_t rs1_id, uint64_t rs1_data,
                            bool rs2_x,
                                 uint64_t rs2_id, uint64_t rs2_data);

    /** Interface API */
    void resetInterface();

    /** RoCC Interface */
    bool recvRoccRequest (RoccCmdPtr  req);
    bool sendRoccResponse(RoccRespPtr resp);
    void recvRetry();

  public:
    RoccPort roccPort;

    BaseAccelerator(BaseAcceleratorParams *params);
    ~BaseAccelerator();

    void registerCPU(BaseCPU *_cpu) { cpu = _cpu; }

    BaseMasterPort& getMasterPort(const std::string &if_name,
                                  PortID idx) override;
    BaseSlavePort& getSlavePort(const std::string &if_name,
                                PortID idx) override;

    /** virtual functions that actual accelerators must implement
      * In the BaseAccelerator, the xtick stalls the accelerator for 100
      * cycles after go bit is set, and then sends out the response. */
    virtual void xtick();
};

#endif /*ACCELERATORS_ACCELERATOR_BASE_HH*/
