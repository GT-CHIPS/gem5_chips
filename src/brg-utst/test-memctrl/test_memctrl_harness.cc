//========================================================================
// Test Memory Controller Harness
//========================================================================
//
// Authors  : Tuan Ta
// Date     : Sep 26, 2018
//

#include "test_memctrl_harness.hh"
#include "test_memctrl_suite.hh"

TestMemCtrlHarness::TestMemCtrlHarness(const Params* params)
    : TestHarness(params)
{
  test_suite_ptr = new TestMemCtrlSuite(num_test_ports);

  // init directed tests
  test_suite_ptr->initDirectedTests();
}

TestMemCtrlHarness*
TestMemCtrlHarnessParams::create()
{
  return new TestMemCtrlHarness(this);
}
