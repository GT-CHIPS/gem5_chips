/*
 * Authors: Khalid Al-Hawaj
 */

/**
 * @file
 *
 *  RoCC Interface.
 *
 */

#ifndef __ROCC_IFCS_HH__
#define __ROCC_IFCS_HH__

/** gem5 standard includes */
#include "base/trace.hh"
#include "mem/port.hh"

/** BRG standard components */
#include "components/queue.hh"

/** RoCC packets */
#include "rocc/packets.hh"

/** RoCC types */
#include "rocc/types.hh"

/** Debugging */
#include "debug/ROCC.hh"

/* Forward declaration */
class MemObject;

namespace ROCC
{

class RoccInterface : public Named
{
  protected:
    /** My owner */
    MemObject& owner;

  protected:
    /** State of memory access for head access. */
    enum InterfaceState
    {
        Running,   /* Default.                                 */
        NeedsRetry /* Request rejected, will be asked to retry */
    };

    InterfaceState state;

    /** Make sure we only use the sending part once. */
    enum UsageState
    {
        Not_Used,  /* Default                        */
        Used       /* Already sent a request         */
    };

    /** Channel state */
    UsageState channel_state;

    /** Number of inflight requests */
    uint64_t inflights;

    /** Exposable data port */
    class RoccPort : public MasterPort
    {
      protected:
        /** My owner */
        RoccInterface &ifcs;

      public:
        RoccPort(std::string name, RoccInterface &ifcs_,
                 MemObject &owner) :
            MasterPort(name, &owner),
            ifcs(ifcs_)
        { }

      protected:
        bool recvTimingResp(PacketPtr pkt) override
        { return ifcs.recvTimingResp(pkt); }

        void recvReqRetry() override { ifcs.recvReqRetry(); }
    };

    /** The actual RoCC port */
    RoccPort roccPort;

    /** Queues */
    Components::Queue<RoccCmdPtr>  requests;
    Components::Queue<RoccRespPtr> responses;

  public:
    RoccInterface(std::string name_, std::string rocc_port_name_,
        MemObject& owner_,
        unsigned int requests_queue_size,
        unsigned int responses_queue_size);

    virtual ~RoccInterface();

  public:
    /** Ticking the queues
     *  So far, I don't know whether this will ever be _actually_ needed */
    void step();

    /** Easy interface to check if a request can be pushed */
    bool canRequest() { return !requests.isFull(); }

    /** Search for a response with a specific ID */
    RoccRespPtr findResponse(void* inst);

    /** Sanity check and pop the head response */
    void popResponse(RoccRespPtr response);

    /** Is there nothing left in-flight? */
    bool isDrained();

    /** Is there anything worth ticking for? */
    bool needsToTick();

    /** Single interface for pushing requests */
    bool pushRequest(RoccCmdPtr request);

    /** RoCC port interface */
    bool recvTimingResp(PacketPtr pkt);
    void recvReqRetry();
    void sendRespRetry();
    void recvTimingSnoopReq(PacketPtr pkt);

    /** Return the raw-bindable port */
    MasterPort &getRoccPort() { return roccPort; }

  protected:
    /** Internal function to send requests */
    void sendQueuedRequests();
    bool trySendingRequest(RoccCmdPtr req);
};

}
#endif /* __ROCC_IFCS_HH__ */
