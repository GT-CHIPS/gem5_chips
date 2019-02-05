/*
 * Authors: Khalid Al-Hawaj
 */
/**
 * @file
 *
 *  This file contain a basic implementation of a null accelerator.
 */

#ifndef __ACCELERATORS_NULL_ACCELERATOR_HH
#define __ACCELERATORS_NULL_ACCELERATOR_HH

/* Parent class */
#include "accelerators/base_accelerator.hh"

/* Accelerator parameters */
#include "params/DelayedNullAccelerator.hh"

struct DelayedNullAcceleratorParams;

class DelayedNullAccelerator : public BaseAccelerator
{
  public:
    /* Define state */
    std::array<uint64_t, 32> rf;

    void*     req_id;

    uint64_t  dest_id;
    uint64_t  dest_data;
    uint64_t  src_id;
    uint64_t  src_data;

    bool      read;
    bool      busy;

    uint64_t  max_delay;
    uint64_t  delay;

    /* Constructor */
    DelayedNullAccelerator(DelayedNullAcceleratorParams *p) :
        BaseAccelerator(p),
        busy(false),
        max_delay(1000),
        delay(0) {}

    /* Deconstructor */
    ~DelayedNullAccelerator() {}

    /* Override the xtick */
    void xtick() override;

    /* Override the recieve function */
    bool recvAcceleratorReq(void* id,
                                 uint64_t opcode, uint64_t funct,
                            bool rd_x,
                                 uint64_t rd_id,  uint64_t rd_data,
                            bool rs1_x,
                                 uint64_t rs1_id, uint64_t rs1_data,
                            bool rs2_x,
                                 uint64_t rs2_id, uint64_t rs2_data) override;
};

#endif /* __ACCELERATORS_NULL_ACCELERATOR_HH */
