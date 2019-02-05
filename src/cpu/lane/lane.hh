/*
 * LaneCPU - a light-weight in-order execution pipeline. Multiple lanes
 * can be grouped in a lane group to form a lane-based accelerator.
 *
 * Authors: Tuan Ta
 */

#ifndef __CPU_LANE_LANE_HH__
#define __CPU_LANE_LANE_HH__

#include "cpu/minor/cpu.hh"
#include "params/LaneCPU.hh"

class LaneCPU : public MinorCPU
{
  public:
    LaneCPU(LaneCPUParams *params);
    ~LaneCPU();

    int laneId() const { return _laneId; };

  private:
    /** Local lane ID in a lane group */
    int _laneId;
};

#endif /* __CPU_LANE_LANE_HH__ */
