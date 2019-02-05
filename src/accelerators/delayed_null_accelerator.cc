/*
 * Authors: Khalid Al-Hawaj
 */
/**
 * @file
 *
 *  This file contain a basic implementation of a null accelerator.
 */

/* This accelerator definition */
#include "accelerators/delayed_null_accelerator.hh"

/* Parameter Create */
DelayedNullAccelerator*
DelayedNullAcceleratorParams::create()
{
    return new DelayedNullAccelerator(this);
}

/* Override the recieve function */
bool
DelayedNullAccelerator::recvAcceleratorReq(void* id,
                                    uint64_t opcode, uint64_t funct,
                                    bool rd_x,
                                       uint64_t rd_id,  uint64_t rd_data,
                                    bool rs1_x,
                                       uint64_t rs1_id, uint64_t rs1_data,
                                    bool rs2_x,
                                       uint64_t rs2_id, uint64_t rs2_data)
{
    /* Return whether request recieved correctly */
    bool recvd = false;

    if (!busy) {
        /* Request revieved :) */
        recvd = true;

        /* Initialize timer */
        delay = max_delay;

        /* RoCC Internal Sequence Number */
        req_id = id;

        /* Determine if this request is a read or a write.
         * A read would have the rd_x set while a write would
         * have the rd_x unset */
        read = rd_x;

        /* Get request details */
        dest_id   = rd_id;
        dest_data = rd_data;

        src_id    = rs1_id;
        src_data  = rs1_data;

        /* Make sure the request is coherent */
        assert((read && rd_x && !rs1_x) || (!read && !rd_x && rs1_x));

        /* We are busy :) */
        busy = true;
    }

    /* Return status */
    return recvd;
}


/* Override the xtick */
void
DelayedNullAccelerator::xtick()
{
    /* Deal with the delay counter */
    if (busy) {
        if (delay) delay--;
    }

    /* Check if we have a request to send */
    if (busy && (delay == 0)) {
      /* Construct a response */
      bool     rd_x    = read;
      uint64_t rd_id   = dest_id;
      uint64_t rd_data = rf[src_id];

      /* Try to send a response */
      bool sent = sendAcceleratorResp(req_id, rd_x, rd_id, rd_data);

      /* Are we done? */
      if (sent) {
          /* Only perform a write if a response can be sent */
          if (!read) {
              /* Perform a write */
              rf[dest_id] = src_data;
          }

          /* Reset accelerator internal state */
          busy = false;

          /* Reset interface */
          resetInterface();
      }
    }
}
