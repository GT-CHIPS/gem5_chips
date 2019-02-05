//========================================================================
// Test Suite
//========================================================================
//
// A collection of all tests cases
//
// Authors  : Tuan Ta
// Date     : Sep 26, 2018
//

#include "test_suite.hh"

TestSuite::TestSuite(int _num_test_ports)
    : num_test_ports(_num_test_ports),
      test_queues(num_test_ports)
{ }

void
TestSuite::setHeadTestCaseIssued(int port_id)
{
  assert(!test_queues[port_id].empty() &&
         !test_queues[port_id].front()->issued);
  test_queues[port_id].front()->issued = true;
}

void
TestSuite::popHeadTestCase(int port_id)
{
  assert(!test_queues[port_id].empty());
  test_queues[port_id].pop();
}

bool
TestSuite::isHeadTestCaseIssued(int port_id) const
{
  return test_queues[port_id].front()->issued;
}

bool
TestSuite::isEmpty(int port_id) const
{
  return test_queues[port_id].empty();
}
