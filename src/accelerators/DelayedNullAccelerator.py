from m5.params import *
from m5.proxy import *

from BaseAccelerator import BaseAccelerator

class DelayedNullAccelerator(BaseAccelerator):
    type = 'DelayedNullAccelerator'
    cxx_header = "accelerators/delayed_null_accelerator.hh"
