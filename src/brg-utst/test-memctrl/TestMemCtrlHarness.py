from TestHarness import TestHarness
from m5.params import *
from m5.proxy import *

class TestMemCtrlHarness(TestHarness):
  type = 'TestMemCtrlHarness'
  cxx_header = "brg-utst/test-memctrl/test_memctrl_harness.hh"
