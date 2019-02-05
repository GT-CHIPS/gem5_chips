from MemObject import MemObject
from m5.params import *
from m5.proxy import *

class TestHarness(MemObject):
  type = 'TestHarness'
  abstract = True
  cxx_header = "brg-utst/test_harness.hh"
  num_test_ports = Param.Unsigned(1, "Number of test ports")
  test_ports = VectorMasterPort("Test ports")

  # This port is used to connect with system.system_port for gem5's debugging
  # purpose
  system_port = SlavePort("Test harness system port")
