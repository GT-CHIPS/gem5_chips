//========================================================================
// Test Memory Controller Harness
//========================================================================
//
// Authors  : Tuan Ta
// Date     : Sep 26, 2018
//

#ifndef TEST_MEMCTRL_HARNESS_HH
#define TEST_MEMCTRL_HARNESS_HH

#include "brg-utst/test_harness.hh"
#include "params/TestMemCtrlHarness.hh"

class TestMemCtrlHarness : public TestHarness {

  public:

    typedef TestMemCtrlHarnessParams Params;

    TestMemCtrlHarness(const Params* params);
    ~TestMemCtrlHarness() = default;

};

#endif // TEST_MEMCTRL_HARNESS_HH
