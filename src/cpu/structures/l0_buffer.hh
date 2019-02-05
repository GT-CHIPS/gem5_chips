/*========================================================================
 * L0_buffer
 *========================================================================
 *
 * Authors: Tuan Ta
 */

#ifndef L0_BUFFER_HH
#define L0_BUFFER_HH

#include <deque>
#include <vector>

#include "mem/mem_object.hh"
#include "mem/port.hh"
#include "params/L0Buffer.hh"

/*
 * This buffer acts like a tiny cache that stores a few already fetched
 * cache lines. The buffer is blocking, which means that a miss blocks
 * subsequent requests from accessing the buffer.
 */
class L0Buffer : public MemObject {
  public:
    typedef L0BufferParams Params;

    L0Buffer(const Params *params);
    ~L0Buffer();

  private:
    /*
     * L0Entry stores a cache line of data.
     */
    struct L0Entry {
      // base address of the cache line
      Addr base_addr;
      // data stored in the entry
      PacketDataPtr data;

      L0Entry(Addr _addr, PacketDataPtr _data, unsigned int size)
          : base_addr(_addr)
      {
        data = new uint8_t[size];
        std::memcpy(data, _data, size);
      }

      ~L0Entry()
      {
        delete[] data;
      }
    };

    /*
     * CpuSidePort - This port talks to CPU.
     */
    class CpuSidePort : public SlavePort {
      private:
        // L0 buffer associated with this port
        L0Buffer* l0_buffer;

      public:
        CpuSidePort(L0Buffer* _l0_buffer, const std::string& name);

        // if true, need to send a retry request to CPU in the future
        bool retry;

      protected:
        virtual bool recvTimingReq(Packet *pkt);
        virtual void recvFunctional(Packet *pkt)
        { panic("Not implemented!\n"); }
        virtual Tick recvAtomic(Packet *pkt)
        { panic("Not implemented!\n"); }
        virtual void recvRespRetry()
        { panic("Not implemented!\n"); }
        virtual AddrRangeList getAddrRanges() const
        { panic("Not implemented!\n"); }
    };

    /*
     * CacheSidePort - This port talks to downstream cache.
     */
    class CacheSidePort : public MasterPort {
      private:
        // L0 buffer associated with this port
        L0Buffer* l0_buffer;

      public:
        // Request packet that has not been issued yet to the cache
        // Waiting for a retry request from the cache to send it out
        PacketPtr pending_packet;

        CacheSidePort(L0Buffer* _l0_buffer, const std::string& name);
        inline bool isBlocked() const
        { return (pending_packet != nullptr); }

      protected:
        // Receive response packet from the cache
        virtual bool recvTimingResp(Packet *pkt);
        // Handles doing a retry of a failed timing request
        virtual void recvReqRetry();
        // Forward snoop requests to a CpuSidePort
        virtual void recvTimingSnoopReq(Packet *pkt)
        { panic("Not implemented!\n"); }
        // Forward snoop requests to CpuSidePort
        virtual Tick recvAtomicSnoop(Packet *pkt)
        { panic("Not implemented!\n"); }
    };

  public:
    // Return CpuSidePort
    BaseSlavePort& getSlavePort(const std::string &if_name, PortID idx)
    {
      return cpu_port;
    }

    // Return CacheSidePort
    BaseMasterPort& getMasterPort(const std::string &if_name, PortID idx)
    {
      return cache_port;
    }

    /*
     * Process the current request (if any). This is called every cycle.
     */
    void tick();

    /*
     * Receive an incoming request in to process in the next cycle
     * Return false if the buffer is busy and unable to process the
     * request in the next cycle
     */
    bool recvRequest(PacketPtr pkt);

    /*
     * Handle a cache response
     */
    void handleCacheResponse(PacketPtr pkt);

    // Register some stats
    virtual void regStats();

  private:
    enum RequestStatus {
      Invalid,    // no request for the current cycle
      Pending,    // not yet be processed
      Issued,     // issued to cache and waiting for a cache response
    };

    // CPU-side port
    CpuSidePort cpu_port;

    // Cache-side port
    CacheSidePort cache_port;

    // Pointer to the current request packet
    PacketPtr req_packet;

    // Current request's status
    RequestStatus req_status;

    // A queue storing all cache entries in L0
    std::deque<L0Entry*> l0_entries;

    // The number of entries in L0 buffer
    int size;

    // cache line size
    unsigned int cache_line_size;

    // Tick event
    EventFunctionWrapper tickEvent;

    // Forward progress check event
    EventFunctionWrapper forwardCheckEvent;

    // Forward check period
    Tick forward_check_period;

    // Some stats variables
    Stats::Scalar hit_count;
    Stats::Scalar miss_count;
    Stats::Formula hit_rate;

    // Return the line address of a word
    inline Addr makeLineAddress(Addr word_addr) const
    {
      Addr mask = (Addr)~0 << floorLog2(cache_line_size);
      return (word_addr & mask);
    }

    // Check if we're moving forward?
    void checkForwardProgress();
};

#endif /* L0_BUFFER_HH */
