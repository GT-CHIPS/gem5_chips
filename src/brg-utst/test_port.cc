//========================================================================
// Test Port
//========================================================================
//
// Authors  : Tuan Ta
// Date     : Sep 26, 2018
//

#include "test_port.hh"

TestPort::TestPort(TestHarness* _test_harness,
                   const std::string& _name,
                   int _port_id)
    : MasterPort(_name, _test_harness),
      port_id(_port_id),
      test_harness(_test_harness),
      is_blocked(false)
{ }

TestPort::~TestPort()
{ }

bool
TestPort::recvTimingResp(PacketPtr pkt)
{
  // Have the test harness verified if the response packet is correct
  test_harness->verifyResponse(port_id, pkt);

  return true;
}

void
TestPort::recvReqRetry()
{
  assert(is_blocked);

  // unblock this port
  is_blocked = false;
}

void
TestPort::setBlocked()
{
  assert(!is_blocked);

  is_blocked = true;
}

bool
TestPort::isBlocked() const
{
  return is_blocked;
}

SystemDebugPort::SystemDebugPort(TestHarness* _test_harness,
                                 const std::string& _name)
    : SlavePort(_name, _test_harness)
{ }

SystemDebugPort::~SystemDebugPort()
{ }
