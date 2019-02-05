//========================================================================
// Cache Allocator
//========================================================================
// Authors: Tuan Ta

#include "cache_allocator.hh"
#include "debug/CacheAllocator.hh"

//---------------------------------------------------------------------------
// CpuSidePort
//---------------------------------------------------------------------------

CacheAllocator::CpuSidePort::CpuSidePort(CacheAllocator* _allocator,
                                         const std::string& name,
                                         int _port_id,
                                         Cycles _req_latency)
    : SlavePort(name, _allocator),
      allocator(_allocator),
      port_id(_port_id),
      req_latency(_req_latency),
      // queue size is set to (req_latency + 1) to model a N-stage pipeline
      // (N is equal to the req_latency). We need to add 1 more entry than
      // req_latency because we attempt to push a new pkt in the beginning of
      // cache allocator's cycle while popping a pkt and freeing a queue slot
      // in the end of its cycle.
      // This way helps avoid a buble in the pipeline due to a limitted queue
      // size.
      req_queue_size(uint64_t(_req_latency) + 1),
      need_retry(false),
      locked(false)
{ }

bool
CacheAllocator::CpuSidePort::recvTimingReq(PacketPtr new_pkt)
{
  DPRINTF(CacheAllocator, "Receiving timing request "
                          "[port_id = %d, addr = 0x%x, pkt = %s]\n",
          port_id, new_pkt->getAddr(), new_pkt->print());

  assert(!need_retry);

  if (req_queue.size() == req_queue_size) {
    // this CPU port is busy, so CPU needs to retry later
    DPRINTF(CacheAllocator, "CPU port %d is busy\n", port_id);
    need_retry = true;
    return false;
  }

  // we can take this request into the pipeline
  req_queue.emplace_back(std::make_pair(allocator->curCycle(), new_pkt));

  return true;
}

void
CacheAllocator::CpuSidePort::recvFunctional(PacketPtr pkt)
{
  DPRINTF(CacheAllocator, "Received functional request "
                          "[port_id = %d, addr = 0x%x, pkt = %s]\n",
          port_id, pkt->getAddr(), pkt->print());
  allocator->updateActivity();
  allocator->handleFunctionalReq(pkt, port_id);
}

Tick
CacheAllocator::CpuSidePort::recvAtomic(PacketPtr pkt)
{
  DPRINTF(CacheAllocator, "Received atomic request "
                          "[port_id = %d, addr = 0x%x, pkt = %s]\n",
          port_id, pkt->getAddr(), pkt->print());
  allocator->updateActivity();
  return allocator->handleAtomicReq(pkt, port_id);
}

bool
CacheAllocator::CpuSidePort::isHeadReady() const
{
  if (locked) {
    assert(!req_queue.empty());
    return false;
  }

  if (req_queue.empty()) {
    return false;
  }

  // check if the head request is due now
  Cycles issuing_cycle = req_queue.front().first;
  return (issuing_cycle + req_latency <= allocator->curCycle());
}

PacketPtr
CacheAllocator::CpuSidePort::headPkt() const
{
  assert(isHeadReady());
  return req_queue.front().second;
}

void
CacheAllocator::CpuSidePort::popHeadPkt()
{
  assert(isHeadReady());

  // update how long did this packet wait in the allocator
  Cycles wait_lat = allocator->curCycle() -
          allocator->ticksToCycles((req_queue.front().second)->req->time());
  allocator->updateWaitLatency(wait_lat);

  req_queue.pop_front();
}

bool
CacheAllocator::CpuSidePort::isMakingProgress(Tick period) const
{
  if (isHeadReady()) {
    Tick last_issue_tick = req_queue.front().first * allocator->clockPeriod();
    if (curTick() - last_issue_tick > period) {
      warn("cpu_port %d pkt %s is not making forward progress",
                              port_id, (req_queue.front().second)->print());
      return false;
    }
  }

  if (need_retry)
    warn("cpu_port %d needs retry", port_id);

  return true;
}

//---------------------------------------------------------------------------
// CacheSidePort
//---------------------------------------------------------------------------

CacheAllocator::CacheSidePort::CacheSidePort(CacheAllocator* _allocator,
                                             const std::string& name,
                                             Cycles _resp_latency)
    : MasterPort(name, _allocator),
      allocator(_allocator),
      resp_latency(_resp_latency),
      // queue size is set to (resp_latency + 1) to model a N-stage pipeline
      // (N is equal to the resp_latency). We need to add 1 more entry than
      // resp_latency because we attempt to push a new pkt in the beginning of
      // cache allocator's cycle while popping a pkt and freeing a queue slot
      // in the end of its cycle.
      // This way helps avoid a buble in the pipeline due to a limitted queue
      // size.
      resp_queue_size(uint64_t(_resp_latency) + 1),
      need_retry(false),
      is_blocked(false)
{ }

bool
CacheAllocator::CacheSidePort::isHeadReady() const
{
  if (resp_queue.empty()) {
    return false;
  }

  // check if the head response is due now
  Cycles issuing_cycle = resp_queue.front().first;
  return (issuing_cycle + resp_latency <= allocator->curCycle());
}

PacketPtr
CacheAllocator::CacheSidePort::headPkt() const
{
  assert(isHeadReady());
  return resp_queue.front().second;
}

void
CacheAllocator::CacheSidePort::popHeadPkt()
{
  assert(isHeadReady());
  resp_queue.pop_front();
}

bool
CacheAllocator::CacheSidePort::recvTimingResp(PacketPtr pkt)
{
  DPRINTF(CacheAllocator, "Receiving timing response [pkt = %s]\n",
          pkt->print());

  assert(!need_retry);

  if (resp_queue.size() == resp_queue_size) {
    // this cache port is busy, so the cache needs to retry later
    DPRINTF(CacheAllocator, "Cache port is busy\n");
    need_retry = true;
    return false;
  }

  // we can take this request into the pipeline
  resp_queue.emplace_back(std::make_pair(allocator->curCycle(), pkt));

  return true;
}

void
CacheAllocator::
CacheSidePort::recvReqRetry()
{
  assert(is_blocked);
  allocator->handleRequests();
  is_blocked = false;
}

//---------------------------------------------------------------------------
// CacheRequest
//---------------------------------------------------------------------------

CacheAllocator::
CacheRequest::CacheRequest(CacheAllocator* _allocator,
                           PacketPtr _pkt,
                           CpuSidePort* cpu_port)
  : allocator(_allocator),
    cpu_pkt_list(1, std::make_pair(_pkt, cpu_port)),
    cache_pkt(nullptr),
    line_addr(allocator->makeLineAddr(cpu_pkt_list[0].first->getAddr())),
    cmd(cpu_pkt_list[0].first->cmd),
    offset(_pkt->getAddr() & ~line_addr),
    // hawajkm: Calculating the effective length of the request.
    //          Since we enforce cacheline-aligned accesses, the
    //          effective length will be whatever offset the request
    //          has with a cacheline on-top of its own size.
    size(offset + _pkt->getSize())
{ }

CacheAllocator::
CacheRequest::CacheRequest()
  : allocator(nullptr),
    cpu_pkt_list(0),
    cache_pkt(nullptr),
    line_addr(0),
    cmd(MemCmd::InvalidCmd),
    offset(0),
    size(0)
{ }

CacheAllocator::
CacheRequest::~CacheRequest()
{
  // For ReadReq, we allocated a new cache_pkt which is different from the
  // original CPU packet, so we need to delete it here.
  // For other request types, we simply forward CPU packets to the cache, so
  // someone else will eventually delete the cache_pkt.
  if (cmd == MemCmd::ReadReq && cache_pkt) {
    delete cache_pkt;
  }
}

bool
CacheAllocator::
CacheRequest::coalesceInSpace(PacketPtr new_pkt, CpuSidePort* cpu_port)
{
  // For now, only ReadReq requests can be coalesced

  if (new_pkt->cmd == MemCmd::LoadLockedReq ||
      new_pkt->cmd == MemCmd::StoreCondReq ||
      new_pkt->cmd == MemCmd::SwapReq ||
      new_pkt->cmd == MemCmd::WriteReq) {
    return false;
  }

  assert(new_pkt->cmd == MemCmd::ReadReq);

  // Coalesce packets having the same command and cache line address
  if (new_pkt->cmd == cmd &&
      allocator->makeLineAddr(new_pkt->getAddr()) == line_addr) {

    // put the new packet into this request's cpu_pkt_list
    cpu_pkt_list.emplace_back(std::make_pair(new_pkt, cpu_port));
    return true;
  }

  return false;
}

bool
CacheAllocator::
CacheRequest::coalesceInTime(const CacheRequestPtr other)
{
  // For now, only ReadReq requests can be coalesced

  if (other->cmd == MemCmd::LoadLockedReq ||
      other->cmd == MemCmd::StoreCondReq ||
      other->cmd == MemCmd::SwapReq ||
      other->cmd == MemCmd::WriteReq) {
    return false;
  }

  // only ReadReq requests can be coalesced across cycle
  assert(other->cmd == MemCmd::ReadReq);

  if (cmd == other->cmd && line_addr == other->line_addr) {
    // coalescing two requests by moving the other request's cpu_pkt_list
    // into this request's cpu_pkt_list
    for (auto& entry : other->cpu_pkt_list) {
      cpu_pkt_list.push_back(entry);
    }
    return true;
  }

  return false;
}

PacketPtr
CacheAllocator::
CacheRequest::makeCachePacket()
{
  if (cmd == MemCmd::ReadReq) {
    // multiple CPU packets are possibly coalesced either in time or in space
    // into this cache request, so we need a new cache_pkt representing all of
    // these CPU packets
    cache_pkt = new Packet((cpu_pkt_list[0].first)->req, cmd,
                           allocator->getCacheLineSize());
    cache_pkt->dataDynamic(new uint8_t[allocator->getCacheLineSize()]);
  } else {
    // non-coalesced request can reuse the cpu_pkt instead of allocating a
    // new cache packet
    assert(cpu_pkt_list.size() == 1);
    cache_pkt = cpu_pkt_list[0].first;
  }

  return cache_pkt;
}

void
CacheAllocator::
CacheRequest::clearCpuPorts()
{
  CpuSidePort* port;

  for (auto& entry : cpu_pkt_list) {
    port = entry.second;
    assert(port && port->isHeadReady());

    // retire the head packet
    port->popHeadPkt();

    // if any CPU packet was blocked, it's time to wake up the CPU
    if (port->need_retry) {
      port->need_retry = false;
      port->sendRetryReq();
    }
  }
}

bool
CacheAllocator::
CacheRequest::sendCpuResponses(PacketPtr cache_response)
{
  bool success = true;

  for (auto& entry : cpu_pkt_list) {
    PacketPtr pkt = entry.first;
    CpuSidePort* cpu_port = entry.second;

    if (cmd == MemCmd::ReadReq) {
      // turn the original request packet into a response packet
      pkt->makeResponse();

      std::memcpy(pkt->getPtr<uint8_t>(),
                  cache_response->getPtr<uint8_t>() + offset,
                  pkt->getSize());
      if (!cpu_port->sendTimingResp(pkt)) {
        success = false;
      }
    } else {
      if (!cpu_port->sendTimingResp(cache_response)) {
        success = false;
      }
    }
  }

  return success;
}

void
CacheAllocator::
CacheRequest::print(std::string& str)
{
  for (auto& entry : cpu_pkt_list) {
    str += "( " + (entry.first)->print() + ", " +
           std::to_string((entry.second)->port_id) + " ), ";
  }
}

int
CacheAllocator::
CacheRequest::getSize()
{
  return size;
}

//---------------------------------------------------------------------------
// CacheAllocator
//
//    This allocator arbitrates between multiple requests from different CPU
//    ports and optionally coalesce them together in a few actual cache
//    requests. This allocator has 1-cycle request latency and 0-cycle
//    response latency.
//
//---------------------------------------------------------------------------

CacheAllocator::CacheAllocator(const Params* params)
    : MemObject(params),
      cache_line_size(params->cache_line_size),
      num_cpu_ports(params->num_cpu_ports),
      arbiter(num_cpu_ports),
      max_num_requests(1),
      enable_coalescing(params->enable_coalescing),
      cache_port(this, this->name() + ".cache_port",
                 Cycles(params->response_latency)),
      next_cache_req_id(0),
      load_locked_timeout(Cycles(30)),
      // tickEvent has lower priority than CPU_Tick_Pri so that this allocator
      // can process CPU packets arriving earlier in a cycle in the same cycle
      // if req_latency is set to 0 cycle.
      tickEvent([this]{ tick(); }, "cache allocator tick event", false,
                Event::Progress_Event_Pri),
      forwardCheckEvent([this]{ checkForwardProgress(); },
                        "Cache allocator checks forward progress"),
      forward_check_period(50000000),
      check_global_progress(params->check_global_progress)
{
  // create CPU ports
  for (int i = 0; i < num_cpu_ports; ++i) {
    std::string port_name = csprintf(".cpu_ports[%d]", i);
    CpuSidePort *port = new CpuSidePort(this, this->name() + port_name, i,
                                        Cycles(params->request_latency));
    cpu_ports.push_back(port);
  }

  // schedule the first tick event
  schedule(tickEvent, curTick());

  // schedule the first forward check event
  schedule(forwardCheckEvent, curTick() + forward_check_period);
}

CacheAllocator::~CacheAllocator()
{
  for (auto port : cpu_ports)
    delete port;
}

void
CacheAllocator::tick()
{
  // handle cache responses
  handleResponses();

  // handle CPU requests
  handleRequests();

  // schedule tick event in the next cycle
  schedule(tickEvent, clockEdge(Cycles(1)));
}

void
CacheAllocator::handleRequests()
{
  // next granted CPU port
  int granted_port_id = -1;

  // a bitset representing which CPU ports are making requests
  std::vector<bool> request_bitset(num_cpu_ports);

  int num_issued_requests = 0;

  // loop until all current packets are issued or the cache is busy
  while (!cache_port.is_blocked &&
         num_issued_requests < max_num_requests &&
         std::count_if(cpu_ports.begin(),
                       cpu_ports.end(),
                       [](CpuSidePort* port) { return port->isHeadReady(); }))
  {
    // set request bitset
    std::string request_bitset_str;
    for (int i = 0; i < num_cpu_ports; ++i) {
      request_bitset[i] = (cpu_ports[i]->isHeadReady()) ? true : false;
      request_bitset_str += request_bitset[i] ? "1 " : "0 ";
    }

    // find the next granted CPU port
    granted_port_id = arbiter.arbitrate(request_bitset);
    assert(granted_port_id != -1);

    DPRINTF(CacheAllocator, "Requesting port [ %s] - Granted port %d\n",
                            request_bitset_str, granted_port_id);

    // check if the granted port is attempting to access a locked address
    PacketPtr pkt = cpu_ports[granted_port_id]->headPkt();
    if (isAddrLocked(pkt, granted_port_id)) {
      // lock the CPU port until it's explicitly unlocked
      cpu_ports[granted_port_id]->locked = true;
    } else {
      // make a new cache request for the new packet
      CacheRequestPtr cache_request =
                  std::make_shared<CacheRequest> (this, pkt,
                                                  cpu_ports[granted_port_id]);

      // hawajkm: Sanit check! Make sure we can satisfy this request.
      assert (cache_request->getSize() <= cache_line_size);

      bool done = false;

      if (enable_coalescing) {
        // try to coalesce in space with packets from other CPU ports
        // in this cycle
        for (auto port : cpu_ports) {
          if (port->port_id != granted_port_id && pkt) {
            cache_request->coalesceInSpace(pkt, port);
          }
        }

        // try to coalesce in time with outstanding cache requests
        for (auto& entry : outstanding_request_map) {
          CacheRequestPtr outstanding_request = entry.second;
          if (outstanding_request->coalesceInTime(cache_request)) {
            // no need to issue this cache_request since it has been coalesced
            // with an outstanding request
            done = true;
            break;
          }
        }
      }

      // try to issue cache_request to the cache
      if (!done) {
        // make a new cache packet for this request
        PacketPtr cache_pkt = cache_request->makeCachePacket();
        Addr addr = cache_pkt->getAddr();

        // assign an ID to the cache_pkt so that we can retrieve its entry in
        // outstanding_request_map when the cache responds.
        CacheAllocator::SenderState* sender_state =
                          new CacheAllocator::SenderState(next_cache_req_id);
        cache_pkt->pushSenderState(sender_state);

        // For LLSC/Write/AtomicOp rquests, we need to snoop other lanes to
        // invalidate any potential copy of the target cache line.
        MemCmd snoop_cmd(MemCmd::InvalidCmd);
        if (cache_pkt->cmd == MemCmd::LoadLockedReq ||
            cache_pkt->cmd == MemCmd::StoreCondReq ||
            cache_pkt->cmd == MemCmd::WriteReq ||
            cache_pkt->cmd == MemCmd::SwapReq) {
          snoop_cmd = cache_pkt->isLLSC() ? MemCmd::SCUpgradeReq :
                                            MemCmd::UpgradeReq;
        }

        bool is_load_locked_req = (cache_pkt->cmd == MemCmd::LoadLockedReq);

        if (cache_port.sendTimingReq(cache_pkt)) {
          // debugging info
          std::string request_info;
          cache_request->print(request_info);
          DPRINTF(CacheAllocator, "Sent cache request [id = %d, pkt = %s]\n",
                  next_cache_req_id, request_info);

          if (is_load_locked_req) {
            if (locked_addr_map.count(addr) > 0) {
              // sanity check
              assert(locked_addr_map[addr].first == granted_port_id);
              assert((curCycle() - locked_addr_map[addr].second) <
                                                          load_locked_timeout);
            } else {
              // register a lock for this port
              locked_addr_map[addr] = std::make_pair(granted_port_id,
                                                     curCycle());
            }
          }

          // send snoop requests to other ports. These snoop requests will
          // invalidate any private copy of the target address in each lane
          if (snoop_cmd != MemCmd::InvalidCmd) {
            PacketPtr snoop_pkt = new Packet(cache_pkt->req, snoop_cmd,
                                             cache_pkt->getSize());

            for (int i = 0; i < num_cpu_ports; ++i) {
              if (i != granted_port_id) {
                DPRINTF(CacheAllocator, "Port %d is snooping port %d\n",
                                        granted_port_id, i);
                cpu_ports[i]->sendTimingSnoopReq(snoop_pkt);
              }
            }

            delete snoop_pkt;
          }

          // add the cache_request to the outstanding request map, and we're
          // done with this request
          assert(outstanding_request_map.count(next_cache_req_id) == 0);
          outstanding_request_map[next_cache_req_id] = cache_request;
          done = true;
          next_cache_req_id++;

          // increment number of requests issued to cache in this cycle
          num_issued_requests++;

        } else {
          // the cache is now busy, so we need to wait until the cache is
          // available again.
          DPRINTF(CacheAllocator, "Cache is busy. Can't issue now\n");
          cache_port.is_blocked = true;
          cache_blocked_count++;

          // clean up
          cache_pkt->popSenderState();
          delete sender_state;
        }
      }

      if (done) {
        // clear CPU ports making this cache_request since their requests
        // have been either issued or coalesced
        cache_request->clearCpuPorts();

        // update the arbiter
        arbiter.update(granted_port_id);

        updateActivity();
      }
    }
  }
}

void
CacheAllocator::handleResponses()
{
  PacketPtr pkt = nullptr;
  bool send_retry = false;

  while (cache_port.isHeadReady()) {
    pkt = cache_port.headPkt();

    // retrieve the packet's ID
    CacheAllocator::SenderState* sender_state =
                safe_cast<CacheAllocator::SenderState*>(pkt->popSenderState());
    unsigned int request_id = sender_state->request_id;

    // there must be exactly one outstanding request waiting for this
    // cache response
    assert(outstanding_request_map.count(request_id) == 1);

    CacheRequestPtr cache_request = outstanding_request_map[request_id];

    // debugging info
    std::string request_info;
    cache_request->print(request_info);
    DPRINTF(CacheAllocator, "Responding [id = %d, %s]\n",
                            request_id, request_info);

    // broadcast the response to all requesting CPUs
    // assuming no back pressure from CPUs, this function should
    // always succeed
    assert(cache_request->sendCpuResponses(pkt));

    // we're done with this request
    outstanding_request_map.erase(request_id);
    cache_port.popHeadPkt();
    delete sender_state;
    send_retry = true;

    updateActivity();
  }

  if (cache_port.need_retry && send_retry) {
    cache_port.need_retry = false;
    cache_port.sendRetryResp();
  }
}

Tick
CacheAllocator::handleAtomicReq(PacketPtr pkt, int req_port_id)
{
  MemCmd snoop_cmd(MemCmd::InvalidCmd);
  if (pkt->cmd == MemCmd::LoadLockedReq ||
      pkt->cmd == MemCmd::StoreCondReq ||
      pkt->cmd == MemCmd::WriteReq ||
      pkt->cmd == MemCmd::SwapReq) {
    snoop_cmd = pkt->isLLSC() ? MemCmd::SCUpgradeReq : MemCmd::UpgradeReq;
    // send snoop requests to other ports. These snoop requests will
    // invalidate any private copy of the target address in each lane
    PacketPtr snoop_pkt = new Packet(pkt->req, snoop_cmd, pkt->getSize());

    for (int i = 0; i < num_cpu_ports; ++i) {
      if (i != req_port_id) {
        DPRINTF(CacheAllocator, "Port %d is snooping port %d\n",
                                req_port_id, i);
        cpu_ports[i]->sendAtomicSnoop(snoop_pkt);
      }
    }

    delete snoop_pkt;
  }

  return cache_port.sendAtomic(pkt);
}

void
CacheAllocator::handleFunctionalReq(PacketPtr pkt, int req_port_id)
{
  MemCmd snoop_cmd(MemCmd::InvalidCmd);
  if (pkt->cmd == MemCmd::LoadLockedReq ||
      pkt->cmd == MemCmd::StoreCondReq ||
      pkt->cmd == MemCmd::WriteReq ||
      pkt->cmd == MemCmd::SwapReq) {
    snoop_cmd = pkt->isLLSC() ? MemCmd::SCUpgradeReq : MemCmd::UpgradeReq;
    // send snoop requests to other ports. These snoop requests will
    // invalidate any private copy of the target address in each lane
    PacketPtr snoop_pkt = new Packet(pkt->req, snoop_cmd, pkt->getSize());

    for (int i = 0; i < num_cpu_ports; ++i) {
      if (i != req_port_id) {
        DPRINTF(CacheAllocator, "Port %d is snooping port %d\n",
                                req_port_id, i);
        cpu_ports[i]->sendFunctionalSnoop(snoop_pkt);
      }
    }

    delete snoop_pkt;
  }

 cache_port.sendFunctional(pkt);
}
bool
CacheAllocator::isAddrLocked(PacketPtr pkt, int port_id)
{
  Addr addr = pkt->getAddr();
  bool is_addr_locked = (locked_addr_map.count(addr) > 0);

  if (is_addr_locked) {
    std::pair<int, Cycles>& lock_owner = locked_addr_map[addr];

    // this is the only case in which a locked address can prevent a request
    // from being issued
    if (pkt->cmd == MemCmd::LoadLockedReq &&
        lock_owner.first != port_id &&
        (curCycle() - lock_owner.second) < load_locked_timeout) {
      warn("port %d: pkt %s hits a LR lock", port_id, pkt->print());
      return true;
    }

    // unlock the address if the lock expired or this is a store or atomic
    // request
    if ((curCycle() - lock_owner.second) >= load_locked_timeout ||
        pkt->cmd == MemCmd::StoreCondReq ||
        pkt->cmd == MemCmd::WriteReq ||
        pkt->cmd == MemCmd::SwapReq) {
      // unlock the address
      DPRINTF(CacheAllocator, "Lock for address 0x%x expired\n", addr);
      locked_addr_map.erase(addr);

      // unlock any CPU port waiting on this lock
      for (auto port : cpu_ports) {
        if (port->locked) {
          port->locked = false;
        }
      }

      return false;
    }
  }

  return false;
}

void
CacheAllocator::checkForwardProgress()
{
  DPRINTF(CacheAllocator, "Checking forward progress\n");

  for (auto port : cpu_ports) {
    if (!port->isMakingProgress(forward_check_period)) {
      panic("%d: %s No forward progress in a while\n", curTick(), name());
    }
  }

  if (check_global_progress &&
      curTick() - last_active_tick > forward_check_period) {
    panic("%d: %s is idle for a while\n", curTick(), name());
  }

  schedule(forwardCheckEvent, curTick() + forward_check_period);
}

void
CacheAllocator::regStats()
{
  MemObject::regStats();
  cache_blocked_count.name(name() + ".cache_blocked_count")
                     .desc("Number of times the cache is blocked")
  ;

  request_count.name(name() + ".request_count")
                     .desc("Number of requests arriving in the allocator")
  ;

  wait_latency.name(name() + ".wait_latency")
                     .desc("Accumulated wait latency of all requests")
  ;

  avg_wait_latency.name(name() + ".avg_wait_latency")
                          .desc("Average wait latency")
                          .precision(2)
  ;
  avg_wait_latency = wait_latency / request_count;
}

CacheAllocator*
CacheAllocatorParams::create()
{
  return new CacheAllocator(this);
}
