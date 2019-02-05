from m5.params import *
from m5.proxy import *

from m5.objects.MemObject import MemObject

class BaseAccelerator(MemObject):
    type = 'BaseAccelerator'
    cxx_header = "accelerators/base_accelerator.hh"

    num_dmem_ports = Param.Unsigned(0, "Number of data memory ports "
                                       "the accelerator uses")
    num_imem_ports = Param.Unsigned(0, "Number of inst memory ports "
                                       "the accelerator uses")

    rocc_port = SlavePort ("RoCC Command Slave Port")
