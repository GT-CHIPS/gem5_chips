#=========================================================================
# L0 Buffer
#=========================================================================
#
# Authors: Tuan Ta
#

from m5.params import *
from m5.proxy import *
from MemObject import MemObject

class L0Buffer(MemObject):
  type = "L0Buffer"
  cxx_header = "cpu/structures/L0Buffer.hh"
  cpu_port = SlavePort("CPU side port")
  cache_port = MasterPort("Cache side port")
  size = Param.Int(1, "L0 buffer size")
  cache_line_size = Param.Int(64, "Cache line size (in bytes)")
