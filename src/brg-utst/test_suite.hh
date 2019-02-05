//========================================================================
// Test Suite
//========================================================================
//
// A collection of all tests cases
//
// Authors  : Tuan Ta
// Date     : Sep 26, 2018
//

#ifndef TEST_SUITE_HH
#define TEST_SUITE_HH

#include <queue>
#include <vector>

#include "mem/packet.hh"

class TestSuite {

  public:

    TestSuite(int _num_test_ports);
    virtual ~TestSuite() = default;

    // Populate directed test cases
    virtual void initDirectedTests() = 0;

    // Populate random test cases
    virtual void initRandomTests() = 0;

    // Get the next request packet for a given test port
    virtual PacketPtr getNextReqPkt(int port_id) = 0;

    // Verify a response packet for a given test port
    virtual bool verifyRespPkt(int port_id, PacketPtr resp_pkt) = 0;

    // Print head test case info
    virtual std::string printHeadTestCase(int port_id) const = 0;

    // Set the head test case for a given test port issued
    void setHeadTestCaseIssued(int port_id);

    // Pop the head test case for a given test port
    void popHeadTestCase(int port_id);

    // Check if the head test case for a given test port is already issued
    bool isHeadTestCaseIssued(int port_id) const;

    // Is the test suite for a given port empty?
    bool isEmpty(int port_id) const;

  protected:

    // A structure holding information about a specific test case
    struct TestCase {

      TestCase(int _id)
          : id(_id), issued(false)
      { }

      virtual ~TestCase() = default;

      int id;
      bool issued;
    };

  protected:

    int num_test_ports;

    typedef std::queue<std::shared_ptr<TestCase>> TestQueue;
    std::vector<TestQueue> test_queues;
};

#endif // TEST_SUITE_HH
