/*========================================================================
 * L0_buffer
 *========================================================================
 * Authors: Tuan Ta
 */

#include "L0Buffer.hh"
#include "debug/L0Buffer.hh"

/*
 * CpuSidePort
 */
L0Buffer::
CpuSidePort::CpuSidePort(L0Buffer* _l0_buffer, const std::string& name)
    : SlavePort(name, _l0_buffer),
      l0_buffer(_l0_buffer),
      retry(false)
{ }

bool
L0Buffer::
CpuSidePort::recvTimingReq(PacketPtr pkt)
{
  assert(!retry);

  // try to receive the packet
  // if failed, set retry
  if (!l0_buffer->recvRequest(pkt)) {
    retry = true;
    return false;
  }

  return true;
}

/*
 * CacheSidePort
 */
L0Buffer::
CacheSidePort::CacheSidePort(L0Buffer* _l0_buffer, const std::string& name)
    : MasterPort(name, _l0_buffer),
      l0_buffer(_l0_buffer),
      pending_packet(nullptr)
{ }

bool
L0Buffer::
CacheSidePort::recvTimingResp(PacketPtr pkt)
{
  l0_buffer->handleCacheResponse(pkt);
  return true;
}

void
L0Buffer::
CacheSidePort::recvReqRetry()
{
  assert(pending_packet != nullptr);
  if (sendTimingReq(pending_packet)) {
    pending_packet = nullptr;
  }
}

/*
 * L0Buffer
 */
L0Buffer::L0Buffer(const Params* params)
    : MemObject(params),
      cpu_port(this, ".cpu_port"),
      cache_port(this, ".cache_port"),
      req_packet(nullptr),
      size(params->size),
      cache_line_size(params->cache_line_size),
      // tickEvent comes before CPU_Tick_Pri events. The idea is to have
      // process all CPU requests of the previous cycle before the CPU
      // ticks and produces more requests.
      tickEvent([this]{ tick(); }, "L0 buffer ticks", false),
      forwardCheckEvent([this]{ checkForwardProgress(); },
                        "L0 buffer checks forward progress", false),
      forward_check_period(10000000)
{
  // schedule the first forward check event
  schedule(forwardCheckEvent, curTick() + forward_check_period);
}

L0Buffer::~L0Buffer()
{
  while (!l0_entries.empty()) {
    delete l0_entries.front();
    l0_entries.pop_front();
  }
}

bool
L0Buffer::recvRequest(PacketPtr pkt)
{
  if (!req_packet) {
    assert(req_status == RequestStatus::Invalid);
    req_packet = pkt;
    req_status = Pending;

    DPRINTF(L0Buffer, "Received request [addr = 0x%x, line_addr = 0x%x]\n",
                       pkt->getAddr(), makeLineAddress(pkt->getAddr()));

    // schedule the next tickEvent
    if (!tickEvent.scheduled())
      schedule(tickEvent, clockEdge(Cycles(1)));

    return true;
  }

  DPRINTF(L0Buffer, "L0 Buffer busy [addr = 0x%x, line_addr = 0x%x]\n",
          pkt->getAddr(), makeLineAddress(pkt->getAddr()));

  return false;
}

void
L0Buffer::tick()
{
  // If we don't have any request and the CPU needs to retry, wake up
  // the CPU now.
  if (cpu_port.retry && req_status == RequestStatus::Invalid) {
    DPRINTF(L0Buffer, "L0 Buffer sending retry request\n");
    assert(!req_packet);
    cpu_port.retry = false;
    cpu_port.sendRetryReq();
  }

  if (req_status == RequestStatus::Pending) {
    assert(req_packet);

    // get the target line address
    Addr line_addr = makeLineAddress(req_packet->getAddr());

    // do L0 buffer lookup
    L0Entry* matched_entry = nullptr;
    for (auto entry : l0_entries) {
      if (entry->base_addr == line_addr) {
        matched_entry = entry;
      }
    }

    if (!matched_entry) {
      // This is an L0 miss
      DPRINTF(L0Buffer, "L0 Buffer Miss [addr = 0x%x, line_addr = 0x%x]\n",
              req_packet->getAddr(), line_addr);

      // The original req_packet is for a single word. We make a new cache
      // request packet for an entire cache line.
      PacketPtr cache_pkt = new Packet(req_packet->req,
                                       req_packet->cmd,
                                       cache_line_size);
      cache_pkt->dataDynamic(new uint8_t[cache_line_size]);

      // Issue cache_pkt
      if (!cache_port.sendTimingReq(cache_pkt)) {
        // failed to send out the cache_pkt, so we need to remember it to
        // retry sending it when the cache is available.
        cache_port.pending_packet = cache_pkt;
      }

      // Update the request status
      req_status = RequestStatus::Issued;

      // Update miss count
      miss_count++;
    } else {
      // This is an L0 hit
      DPRINTF(L0Buffer, "L0 Buffer Hit [addr = 0x%x, line_addr = 0x%x]\n",
              req_packet->getAddr(), line_addr);

      // Turn request pkt into a response packet
      req_packet->makeResponse();

      // Copy data from matched_entry to the response packet
      int offset = req_packet->getAddr() - matched_entry->base_addr;
      std::memcpy(req_packet->getPtr<uint8_t>(),
                  matched_entry->data + offset,
                  req_packet->getSize());

      // Send the response to CPU
      // Assume that there's no back pressure from CPU
      bool success = cpu_port.sendTimingResp(req_packet);
      assert(success);

      DPRINTF(L0Buffer, "Sent CPU response [addr = 0x%x, data = 0x%x]\n",
              req_packet->getAddr(),
              *(req_packet->getPtr<uint8_t>()));

      // Clean up
      req_packet = nullptr;
      req_status = RequestStatus::Invalid;

      // Update hit count
      hit_count++;
    }
  }
}

void
L0Buffer::handleCacheResponse(PacketPtr pkt)
{
  Addr line_addr = pkt->getAddr();

  // sanity check
  assert(line_addr % cache_line_size == 0);
  assert(req_packet && req_status == RequestStatus::Issued);

  // if l0_entries is full, do cache replacement (assuming FIFO policy)
  if (l0_entries.size() == size) {
    DPRINTF(L0Buffer, "Replaced L0 entry [line_addr = 0x%x]\n",
            l0_entries.front()->base_addr);
    // remove the front entry
    assert(l0_entries.front()->data);
    delete l0_entries.front();
    l0_entries.pop_front();
  }

  // refill l0_entries
  assert(l0_entries.size() < size);
  L0Entry* new_entry = new L0Entry(line_addr,
                                   pkt->getPtr<uint8_t>(),
                                   cache_line_size);
  l0_entries.emplace_back(new_entry);

  // Turn req_packet to a response packet
  req_packet->makeResponse();

  // Copy data from cache pkt to the response packet
  int offset = req_packet->getAddr() - line_addr;
  std::memcpy(req_packet->getPtr<uint8_t>(),
              pkt->getPtr<uint8_t>() + offset,
              req_packet->getSize());

  // Send the response to CPU
  // Assume that there's no back pressure from CPU
  bool success = cpu_port.sendTimingResp(req_packet);
  assert(success);

  DPRINTF(L0Buffer, "Sent CPU response [addr = 0x%x, data = 0x%x]\n",
          req_packet->getAddr(), *(req_packet->getPtr<uint8_t>()));

  // schedule the next tickEvent
  if (!tickEvent.scheduled())
    schedule(tickEvent, clockEdge(Cycles(1)));

  // Clean up
  req_packet = nullptr;
  req_status = RequestStatus::Invalid;
  delete pkt;
}

void
L0Buffer::checkForwardProgress()
{
  DPRINTF(L0Buffer, "Checking forward progress\n");

  if (req_packet) {
    Tick issue_tick = req_packet->req->time();
    if (curTick() - issue_tick > forward_check_period) {
      panic("Pkt [addr = 0x%x] is not making any progress in L0 Buffer\n",
            req_packet->getAddr());
    }
  }

  PacketPtr pkt = cache_port.pending_packet;
  if (pkt) {
    Tick issue_tick = req_packet->req->time();
    if (curTick() - issue_tick > forward_check_period) {
      panic("Pkt [addr = 0x%x] is not waiting for cache for too long\n",
            pkt->getAddr());
    }
  }

  schedule(forwardCheckEvent, curTick() + forward_check_period);
}

void
L0Buffer::regStats()
{
  MemObject::regStats();

  hit_count.name(name() + ".hit_count")
           .desc("Number of hits in L0 buffer")
  ;

  miss_count.name(name() + ".miss_count")
            .desc("Number of hits in L0 buffer")
  ;

  hit_rate.name(name() + ".hit_rate")
          .desc("L0 buffer hit rate")
  ;

  hit_rate = hit_count/(hit_count + miss_count);
}

L0Buffer*
L0BufferParams::create()
{
  return new L0Buffer(this);
}
