/*========================================================================
 * Cache Allocator
 *========================================================================
 *
 * Authors: Tuan Ta
 */

#ifndef CACHE_ALLOCATOR_HH
#define CACHE_ALLOCATOR_HH

#include <deque>
#include <unordered_map>
#include <vector>

#include "cpu/structures/arbiter.hh"
#include "mem/mem_object.hh"
#include "mem/port.hh"
#include "params/CacheAllocator.hh"

/*
 * Cache allocator forwards CPU requests to a shared cache. It also tries to
 * coalesce requests that target the same cache line together into a single
 * cache access.
 */

class CacheAllocator : public MemObject {

  public:
    typedef CacheAllocatorParams Params;
    CacheAllocator(const Params *params);
    ~CacheAllocator();

  private:
    /*
     * CpuSidePort - interface between a CPU and this cache allocator
     * This CpuSidePort models an N-stage pipeline to account for the cache
     * allocator's request latency.
     */

    class CpuSidePort : public SlavePort {

      public:
        CpuSidePort(CacheAllocator* _allocator,
                    const std::string& name,
                    int _port_id,
                    Cycles _req_latency);

        // Allocator associated with this port
        CacheAllocator* allocator;
        // Port ID
        const int port_id;
        // Request latency
        const Cycles req_latency;
        // Request queue's capacity
        const unsigned int req_queue_size;
        // If true, need to send a retry request to CPU in the future when
        // cache resources are available again
        bool need_retry;
        // The current head packet in this port is being locked (i.e., by a
        // load-locked request), so it can't be processed until being
        // explicitly unlocked
        bool locked;

        // Is the head request ready in this cycle?
        bool isHeadReady() const;
        // Get head packet
        PacketPtr headPkt() const;
        // Pop head packet
        void popHeadPkt();
        // Is this CPU port manking any forward progress in the given period?
        bool isMakingProgress(Tick period) const;

      protected:
        virtual bool recvTimingReq(Packet *pkt);
        virtual void recvFunctional(Packet *pkt);
        virtual Tick recvAtomic(Packet *pkt);
        virtual void recvRespRetry()
        { panic("recvRespRetry Not implemented!\n"); }
        virtual AddrRangeList getAddrRanges() const
        { panic("getAddrRanges Not implemented!\n"); }

      private:
        // FIFO request queue. Each entry is a pair of a request packet and
        // its arriving cycle.
        std::deque<std::pair<Cycles, PacketPtr>> req_queue;
    };

    /*
     * CacheSidePort - interface between this cache allocator and a cache
     */

    class CacheSidePort : public MasterPort {

      public:
        CacheSidePort(CacheAllocator* _allocator, const std::string& name,
                      Cycles _resp_latency);

        // Allocator associated with this port
        CacheAllocator* allocator;
        // Response latency
        const Cycles resp_latency;
        // Response queue's capacity
        const unsigned int resp_queue_size;
        // If true, need to send a retry request to the cache in the future
        // when a CPU port is available to take responses again.
        bool need_retry;
        // If true, the cache is blocking. We need to wait until recvReqRetry
        // to issue further cache requests.
        bool is_blocked;
        // Is the head response ready in this cycle?
        bool isHeadReady() const;
        // Get head packet
        PacketPtr headPkt() const;
        // Pop head packet
        void popHeadPkt();

      protected:
        // Receive response packet from the cache
        virtual bool recvTimingResp(Packet *pkt);
        // Handles doing a retry of a failed timing request
        virtual void recvReqRetry();
        // Forward snoop requests to a CpuSidePort
        virtual void recvTimingSnoopReq(Packet *pkt)
        { panic("recvTimingSnoopReq Not implemented!\n"); }
        // Forward snoop requests to CpuSidePort
        virtual Tick recvAtomicSnoop(Packet *pkt)
        { panic("recvAtomicSnoop Not implemented!\n"); }

      private:
        // FIFO response queue. Each entry is a pair of a response packet and
        // its arriving cycle.
        std::deque<std::pair<Cycles, PacketPtr>> resp_queue;
    };

    /*
     * SenderState for cache requests
     */

    struct SenderState : public Packet::SenderState
    {
      unsigned int request_id;
      SenderState(unsigned int _request_id)
          : request_id(_request_id)
      { }
    };

    /*
     * CacheRequest has information about requests already issued to the
     * cache. This structure keeps track of which CPU port(s) made a cache
     * request so that when a response to the request is available, the
     * cache alloactor can forward it to the right requester.
     */

    class CacheRequest {

      public:
        CacheRequest(CacheAllocator* allocator,
                     PacketPtr pkt,
                     CpuSidePort* cpu_port);

        // define zero-constructor here just to satisfy std::unordered_map::[]
        // operator which requires it.
        CacheRequest();

        ~CacheRequest();

        // Coalesce packets from other CPU ports with this cache request in
        // space. This coalescing requires that all coalesced packets are
        // processed in the same cycle.
        // @params
        //    pkt - new packet
        //    cpu_port - port sending the new packet
        // @return
        //    true if the new packet can be coalesced with the cache request
        //    false otherwise
        bool coalesceInSpace(PacketPtr pkt, CpuSidePort* cpu_port);

        // Coalesce two cache requests together in time
        // This type of coalescing happens when a cache request can be
        // coalesced with an earlier outstanding cache request.
        // @params
        //    other - new cache request
        // @return
        //    true if the new cache request can be coalesced with this request
        //    false otherwise
        bool coalesceInTime(std::shared_ptr<CacheRequest> other);

        // Return a request packet to be issued to the cache
        // This request packet represents all CPU requests coalesced into this
        // single cache request.
        PacketPtr makeCachePacket();

        // Clear packets in all CPU ports associated with this cache request
        void clearCpuPorts();

        // Make and send CPU response packets
        // @params
        //    cache_response - cache response packet
        // @return
        //    true if all CPU responses are sent successfully
        //    false otherwise
        bool sendCpuResponses(PacketPtr cache_response);

        // Print this cache request's info in a given string
        void print(std::string& str);

        // Size of the aligned access
        int getSize();

      private:
        // Pointer to cache allocator making this request
        CacheAllocator* allocator;

        // List of CPU request packets and their CPU port ID
        std::vector<std::pair<PacketPtr,CpuSidePort*>> cpu_pkt_list;

        // Cache request packet
        PacketPtr cache_pkt;

        // Cache line address of the cache request
        const Addr line_addr;

        // Request command
        const MemCmd cmd;

        // Offset
        const Addr offset;

        // Packet Size
        const int size;
    };

  public:
    // Return CpuSidePort
    BaseSlavePort& getSlavePort(const std::string &if_name, PortID idx)
    {
      return *cpu_ports[idx];
    }

    // Return CacheSidePort
    BaseMasterPort& getMasterPort(const std::string &if_name, PortID idx)
    {
      return cache_port;
    }

    // Tick by advancing the allocator's pipeline
    void tick();

    // Process the current request (if any).
    void handleRequests();

    // Handle a cache response
    void handleResponses();

    // Handle a atomic request
    Tick handleAtomicReq(PacketPtr pkt, int req_port_id);

    // Handle a functional request
    void handleFunctionalReq(PacketPtr pkt, int req_port_id);

    // Register some stats
    virtual void regStats();

    // Return cache line's size in bytes
    inline unsigned int getCacheLineSize() const {
      return cache_line_size;
    }

    // Return the line address of a word
    Addr makeLineAddr(Addr word_addr) const {
      Addr mask = (Addr)~0 << floorLog2(cache_line_size);
      return (word_addr & mask);
    }

    // Check if a request is about to access a locked address
    bool isAddrLocked(PacketPtr pkt, int port_id);

    // Update wait latency stats
    void updateWaitLatency(Cycles latency) {
      request_count++;
      wait_latency += latency;
    }

    // Record the last active tick
    void updateActivity() { last_active_tick = curTick(); }

  private:
    // Cache line's size
    const unsigned int cache_line_size;

    // Number of CPU ports
    const unsigned int num_cpu_ports;

    // Simple round-robin arbiter
    Arbiter arbiter;

    // Maximum number of requests this allocator can send to cache in 1 cycle
    // This is used to model the number of cache ports
    const unsigned int max_num_requests;

    // Enable coalescing?
    const bool enable_coalescing;

    // List of CPU-side ports
    std::vector<CpuSidePort*> cpu_ports;

    // Cache-side port
    CacheSidePort cache_port;

    // Next cache request ID
    unsigned int next_cache_req_id;

    // List of all outstanding requests. Each request is indexed by a unique
    // request ID
    typedef std::shared_ptr<CacheRequest> CacheRequestPtr;
    typedef std::unordered_map<unsigned int, CacheRequestPtr> RequestMap;
    RequestMap outstanding_request_map;

    // A map of locked addresses and their owners
    typedef std::unordered_map<Addr, std::pair<int, Cycles>> LockedAddrMap;
    LockedAddrMap locked_addr_map;

    // Load-locked timeout. This is the maximum number of cycles in which
    // a port can lock an address
    const Cycles load_locked_timeout;

    // Request event handles CPU requests
    EventFunctionWrapper tickEvent;

    // Forward progress check event
    EventFunctionWrapper forwardCheckEvent;

    // Forward check period
    Tick forward_check_period;

    // If this is set to true, we monitor the last tick when this allocator had
    // any activity, and then check if this allocator has been inactive for
    // more than the timeout threshold. By default, this check is off. It's
    // used only for extra progress check.
    bool check_global_progress;

    // Tick when the last activity is recorded
    Tick last_active_tick;

    // Some stats variables
    Stats::Scalar cache_blocked_count;
    Stats::Scalar request_count;
    Stats::Scalar wait_latency;
    Stats::Formula avg_wait_latency;

    // Check if we're moving forward?
    void checkForwardProgress();
};

#endif /* CACHE_ALLOCATOR_HH */
