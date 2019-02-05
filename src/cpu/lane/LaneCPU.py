#
# Authors: Tuan Ta
#

from m5.defines import buildEnv
from m5.params import *
from m5.proxy import *
from m5.SimObject import SimObject
from MinorCPU import MinorCPU

class LaneCPU(MinorCPU):
  type = 'LaneCPU'
  cxx_header = 'cpu/lane/lane.hh'

  lane_id = Param.Unsigned(0, 'Lane ID in a lane group')

  # overwrite some configurations in MinorCPU
  #
  # @tuan:
  # NOTE: This CPU configuration is specific to a lane-based system.
  # Each lane is a very simple non-superscalar in-order pipeline
  # Different from the default MinorCPU configuration, fetch1 and fetch2
  # work at the granularity of instructions instead of cache lines. Fetch1
  # stage allows at most 2 instructions to be fetched at a time, and the
  # input buffer in fetch2 has two entries. This is used to hide 1-cycle hit
  # latency in L1 by allowing 2 instruction fetches to be concurrent at a
  # time. Other input buffers in other stages (i.e., Decode and Execute)
  # have the size of 1 instruction.
  #

  enableIdling                  = False
  #
  # Fetch1
  #
  fetch1FetchLimit              = 2   # in cache requests
  fetch1LineSnapWidth           = 4   # in bytes
  fetch1LineWidth               = 4   # in bytes
  fetch1ToFetch2ForwardDelay    = 1   # in cycles
  fetch1ToFetch2BackwardDelay   = 1   # in cycles

  #
  # Fetch2
  #
  fetch2InputBufferSize         = 2   # in instructions
  fetch2ToDecodeForwardDelay    = 1   # in cycles
  # Allow Fetch2 to cross input lines to generate full output each cycle
  fetch2CycleInput              = True

  #
  # Decode
  #
  decodeInputBufferSize         = 1   # in instructions
  decodeToExecuteForwardDelay   = 1   # in cycles
  decodeInputWidth              = 1   # in instructions
  # Allow Decode to pack instructions from more than one input cycle
  # to fill its output each cycle
  decodeCycleInput              = True

  #
  # Execute
  #
  executeInputWidth             = 1   # in instructions
  executeCycleInput             = True
  executeIssueLimit             = 1
  executeMemoryIssueLimit       = 1
  executeCommitLimit            = 1
  executeMemoryCommitLimit      = 1
  executeInputBufferSize        = 1
  executeMemoryWidth            = 8   # in bytes (64-bit)
  executeMaxAccessesInMemory    = 1
#  executeBranchDelay            = 0

