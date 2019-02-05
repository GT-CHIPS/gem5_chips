//========================================================================
// Test Harness
//========================================================================
//
// Authors  : Tuan Ta
// Date     : Sep 26, 2018
//

#ifndef TEST_HARNESS_HH
#define TEST_HARNESS_HH

#include <vector>

#include "mem/mem_object.hh"
#include "mem/port.hh"
#include "params/TestHarness.hh"
#include "test_port.hh"
#include "test_suite.hh"

class TestPort;
class SystemDebugPort;
struct TestHarnessParams;

class TestHarness : public MemObject {

  public:

    typedef TestHarnessParams Params;

    TestHarness(const Params* params);
    ~TestHarness();

    // Return a reference to the test port
    BaseMasterPort& getMasterPort(const std::string &if_name, PortID idx);

    // Retern a reference to the system debug port
    BaseSlavePort& getSlavePort(const std::string &if_name, PortID idx);

    // Verify a test response from test object
    void verifyResponse(int port_id, PacketPtr pkt);

  protected:

    // Tick this test harness to issue the next request
    void tick();

  protected:

    // Number of test ports
    const int num_test_ports;

    // List of test ports
    std::vector<TestPort*> test_ports;

    // This port is connected to system.system_port for gem5 debugging purpose
    SystemDebugPort* system_port;

    // Pointer to test suite
    TestSuite* test_suite_ptr;

    // Tick event
    EventFunctionWrapper tickEvent;
};

#endif // TEST_HARNESS_HH
