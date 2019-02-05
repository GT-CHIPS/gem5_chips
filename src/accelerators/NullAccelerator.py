from m5.params import *
from m5.proxy import *

from BaseAccelerator import BaseAccelerator

class NullAccelerator(BaseAccelerator):
    type = 'NullAccelerator'
    cxx_header = "accelerators/null_accelerator.hh"
