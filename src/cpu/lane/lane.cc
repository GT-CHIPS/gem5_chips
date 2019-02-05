/*
 * Authors: Tuan Ta
 */

#include "cpu/lane/lane.hh"

LaneCPU::LaneCPU(LaneCPUParams *params) :
    MinorCPU(params),
    _laneId(params->lane_id)
{ }

LaneCPU::~LaneCPU()
{ }

LaneCPU *
LaneCPUParams::create()
{
    return new LaneCPU(this);
}
