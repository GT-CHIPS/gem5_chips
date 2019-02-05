/**
 * Authors: Shunning Jiang
 *          Khalid Al-Hawaj
 */

/**
 * @file
 *
 *  This file is adapted from Shreesha Srinath's accelerator implemntation.
 *
 */

/* Class definition */
#include "accelerators/base_accelerator.hh"

/* gem5 includes */
#include "sim/eventq.hh"

/* AcceleratorEvent
 *   NOTE: XCEL_Tick_Pri < CPU_Tick_Pri
 *         The priorty for the xcel tick should be lower
 *         than the cpu tick */
static const EventBase::Priority XCEL_Tick_Pri = 75;
BaseAccelerator::AcceleratorEvent::AcceleratorEvent(BaseAccelerator *xcel)
  : Event(XCEL_Tick_Pri), owner(xcel)
{ }

void
BaseAccelerator::AcceleratorEvent::process() {
    /* Schedule a tick */
    if (!this->scheduled()) {
        owner->schedule(this, owner->clockEdge(Cycles(1)));
    }

    owner->xtick();
}

const char*
BaseAccelerator::AcceleratorEvent::description() const
{
  return "Accelerator Tick event";
}

BaseAccelerator::BaseAccelerator(BaseAcceleratorParams *p) :
  MemObject(p), event(this),
  ifcsNeedsRetry(false),
  roccPort("rocc_port", this)
{
    schedule(&event, clockEdge(Cycles(1)));
}

BaseMasterPort&
BaseAccelerator::getMasterPort(const std::string& if_name, PortID idx)
{
    /* No ports, so far ...*/
    return MemObject::getMasterPort(if_name, idx);
}

BaseSlavePort&
BaseAccelerator::getSlavePort(const std::string& if_name, PortID idx)
{
    /* Only RoCC port is here */
    if (if_name == "rocc_port") {
        return roccPort;
    }
    else {
        return MemObject::getSlavePort(if_name, idx);
    }
}

BaseAccelerator*
BaseAcceleratorParams::create()
{
    return new BaseAccelerator(this);
}

BaseAccelerator::~BaseAccelerator()
{
}

void
BaseAccelerator::recvRetry()
{
    /* This would be used in-case we have a retry for the response */
    std::cout << "Warning: recvRetry in accelerator called!" << std::endl;
}

bool BaseAccelerator::sendAcceleratorResp(void* id_, bool reg_xd_,
                                          uint64_t reg_id_, uint64_t data_)
{
    /* Initialize response packet */
    RoccRespPtr resp = new RoccResp(id_, reg_xd_, reg_id_, data_);

    /* Print */
    DPRINTF(ACCEL, "Sending accelerator response: %s\n", resp->print());

    /* Downcast */
    PacketPtr pkt = safe_cast<PacketPtr>(resp);

    /* return outcome */
    return roccPort.sendTimingResp(pkt);
}

/** Interface API */
void
BaseAccelerator::resetInterface()
{
    /* Send a request retry if needed */
    if (ifcsNeedsRetry) {
        roccPort.sendRetryReq();
    }

    /* Reset internal flag */
    ifcsNeedsRetry = false;
}

/** RoCC Interface */
bool
BaseAccelerator::recvRoccRequest(RoccCmdPtr req)
{
    /* Record returned value */
    bool ret = false;

    /* Call internal recieve request with parameters */
    ret = recvAcceleratorReq(req->inst,  req->opcode, req->funct,
                             req->rd.x,  req->rd.id,  req->rd.data,
                             req->rs1.x, req->rs1.id, req->rs1.data,
                             req->rs2.x, req->rs2.id, req->rs2.data);

    /* Remember to send a retry request if failed */
    if (!ret) {
      ifcsNeedsRetry = true;
    }

    /* Return status */
    return ret;
}

/* Override the recieve function */
bool
BaseAccelerator::recvAcceleratorReq(void* id_,
                                    uint64_t opcode_, uint64_t funct_,
                                    bool rd_x,
                                      uint64_t rd_id,  uint64_t rd_data,
                                    bool rs1_x,
                                      uint64_t rs1_id, uint64_t rs1_data,
                                    bool rs2_x,
                                      uint64_t rs2_id, uint64_t rs2_data)
{
    /* Return whether request recieved correctly */
    bool recvd = false;

    /* Return status */
    return recvd;
}


/* Override the xtick */
void
BaseAccelerator::xtick()
{
    return;
}
