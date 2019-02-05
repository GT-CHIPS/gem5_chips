#=========================================================================
# Cache Allocator
#=========================================================================
#
# Authors: Tuan Ta
#

from m5.params import *
from m5.proxy import *
from MemObject import MemObject

class CacheAllocator(MemObject):
  type = "CacheAllocator"
  cxx_header = "cpu/structures/cache_allocator.hh"
  num_cpu_ports = Param.Int(1, "Number of CPU ports")
  cpu_ports = VectorSlavePort("CPU side ports")
  cache_port = MasterPort("Cache side port")
  cache_line_size = Param.Int(64, "Cache line size (in bytes)")
  enable_coalescing = Param.Bool(False, "Enable coalescing")
  request_latency = Param.Int(1, "Request latency")
  response_latency = Param.Int(0, "Response latency")
  check_global_progress = Param.Bool(False, "Enable global forward checking")
