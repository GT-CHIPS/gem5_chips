//========================================================================
// Test Harness
//========================================================================
//
// Authors  : Tuan Ta
// Date     : Sep 26, 2018
//

#include "debug/UnitTest.hh"
#include "sim/sim_exit.hh"
#include "test_harness.hh"

TestHarness::TestHarness(const Params* params)
    : MemObject(params),
      num_test_ports(params->num_test_ports),
      test_suite_ptr(nullptr),  // must be allocated by a child class
      tickEvent([this]{ tick(); },
                 "test harness tick event",
                 false,
                 Event::CPU_Tick_Pri)
{
  // create test ports
  for (int i = 0; i < num_test_ports; ++i) {
    std::string port_name = csprintf(".test_ports[%d]", i);
    TestPort* port = new TestPort(this, this->name() + port_name, i);
    test_ports.push_back(port);
  }

  system_port = new SystemDebugPort(this, this->name() + ".system_port");

  // schedule the first tick event
  schedule(tickEvent, curTick());
}

TestHarness::~TestHarness()
{
  for (auto port : test_ports) {
    assert(port);
    delete port;
  }

  delete system_port;

  assert(test_suite_ptr);
  delete test_suite_ptr;
}

BaseMasterPort&
TestHarness::getMasterPort(const std::string &if_name, PortID idx)
{
  return *(test_ports[idx]);
}

BaseSlavePort&
TestHarness::getSlavePort(const std::string &if_name, PortID idx)
{
  return *system_port;
}

void
TestHarness::tick()
{
  DPRINTF(UnitTest, "TestHarness ticks ...\n");

  // Are all tests done?
  bool done = true;

  // Go through all ports
  for (int port_id = 0; port_id < num_test_ports; port_id++) {

    if (!test_ports[port_id]->isBlocked() &&
        !test_suite_ptr->isEmpty(port_id) &&
        !test_suite_ptr->isHeadTestCaseIssued(port_id)) {

      PacketPtr req_pkt = test_suite_ptr->getNextReqPkt(port_id);

      if (test_ports[port_id]->sendTimingReq(req_pkt)) {

        // successfully issued the request, so mark this test case as
        // "issued"
        test_suite_ptr->setHeadTestCaseIssued(port_id);

        DPRINTF(UnitTest, "Issued %s\n",
                          test_suite_ptr->printHeadTestCase(port_id));

      } else {

        // the test object is busy, so we set this port being blocked
        test_ports[port_id]->setBlocked();

        // delete pkt and pkt->req
        if (req_pkt->req && req_pkt->isRequest())
          delete req_pkt->req;
        delete req_pkt;

      }
    }

    if (!test_suite_ptr->isEmpty(port_id)) {
      // we're not done yet
      done = false;
    }
  }

  if (!done) {
    // Schedule tickEvent in the next cycle
    schedule(tickEvent, clockEdge(Cycles(1)));
  } else {
    exitSimLoop("All unit tests completed!");
  }
}

void
TestHarness::verifyResponse(int port_id, PacketPtr pkt)
{
  assert(!test_suite_ptr->isEmpty(port_id) &&
          test_suite_ptr->isHeadTestCaseIssued(port_id));

  // verify the response packet
  if (!test_suite_ptr->verifyRespPkt(port_id, pkt)) {
    panic("Unit test failed!\n");
  }

  // retire this test case
  test_suite_ptr->popHeadTestCase(port_id);

  // clean up
  delete pkt;
}
