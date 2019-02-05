//========================================================================
// Test MemCtrl Suite
//========================================================================
//
// A collection of all tests cases for memory controller
//
// Authors  : Tuan Ta
// Date     : Sep 26, 2018
//

#ifndef TEST_MEMCTRL_SUITE_HH
#define TEST_MEMCTRL_SUITE_HH

#include "brg-utst/test_suite.hh"

class TestMemCtrlSuite : public TestSuite {

  public:

    TestMemCtrlSuite(int _num_test_ports);
    ~TestMemCtrlSuite() = default;

    // Populate directed test cases
    void initDirectedTests() override;

    // Populate random test cases
    void initRandomTests() override;

    // Get the next request packet for a given test port
    PacketPtr getNextReqPkt(int port_id) override;

    // Verify a response packet for a given test port
    bool verifyRespPkt(int port_id, PacketPtr resp_pkt) override;

    // Print head test case info
    std::string printHeadTestCase(int port_id) const override;

  protected:

    struct TestMemCtrlCase : public TestSuite::TestCase {

      TestMemCtrlCase(int _id, MemCmd _cmd, Addr _addr,
                      int _ref_val, int _store_val)
          : TestCase(_id),
            cmd(_cmd),
            addr(_addr),
            ref_val(_ref_val),
            store_val(_store_val)
      { }

      ~TestMemCtrlCase() = default;

      MemCmd cmd;
      Addr addr;
      int ref_val;    // for read
      int store_val;  // for store
    };

  private:

    // TODO for random tests
    // std::unordered_map<Addr, int> ref_backing_storage;
};

#endif //TEST_MEMCTRL_SUITE_HH
