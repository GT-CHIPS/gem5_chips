/*
 * Authors: Khalid Al-Hawaj
 */

#include "rocc/ifcs.hh"

namespace ROCC
{

RoccInterface::RoccInterface(std::string name_, std::string rocc_port_name_,
    MemObject& owner_,
    unsigned int requests_queue_size,
    unsigned int responses_queue_size) :
    Named(name_),
    owner(owner_),
    state(Running),
    channel_state(Not_Used),
    inflights(0),
    roccPort(rocc_port_name_, *this, owner_),
    requests("requests_queue", requests_queue_size),
    responses("responses_queue", responses_queue_size)
{
    if (requests_queue_size < 1) {
        fatal("%s: roccRequestsQueueSize must be >= 1 (%d)\n", name_,
            requests_queue_size);
    }

    if (responses_queue_size < 1) {
        fatal("%s: roccResponsesQueueSize must be >= 1 (%d)\n", name_,
            responses_queue_size);
    }
}

RoccInterface::~RoccInterface()
{ }

/** Ticking the queues
 *  So far, I don't know whether this will ever be _actually_ needed */
void
RoccInterface::step()
{
    /* Reset the state if unblocked */
    channel_state = Not_Used;

    /* If we have a request, send it. */
    sendQueuedRequests();
}

/** Search for a response with a specific ID */
RoccRespPtr
RoccInterface::findResponse(void* inst)
{
    /* Initialize returned response */
    RoccRespPtr ret = NULL;

    /* If we have some responses */
    if (!responses.isEmpty()) {
        /* Get the response in the front */
        RoccRespPtr resp = responses.front();

        /* Check for matched ID */
        if (resp->inst == inst) {
            ret = resp;
        }
    }

    /* Return whatever we have got */
    return ret;
}

/** Sanity check and pop the head response */
void
RoccInterface::popResponse(RoccRespPtr response)
{
    /* TODO(hawajkm): Implement comparator in response class */
    bool sanity_check = !responses.isEmpty() &&
                        (responses.front()->inst == response->inst);

    assert(sanity_check);

    response = responses.pop();

    delete response;
}

/** Is there nothing left in-flight? */
bool
RoccInterface::isDrained()
{
    return requests.isEmpty() && (inflights == 0);
}

/** Is there anything worth ticking for? */
/** TODO(hawajkm):
 *        I need to make sure the CPU is scheduled every
 *        cycle there is an outstanding request. This is
 *        hacky, but usually we would have an interface
 *        to wakeup the CPU/owner on demand. Because we
 *        chose to have a generic interface that does not
 *        assume any CPU, we lack that interface. Thanks
 *        gem5 :D*/
bool
RoccInterface::needsToTick()
{
    bool ret = false;

    if (!requests.isEmpty()  ||
        !responses.isEmpty() ||
        inflights > 0)
    {
        ret = true;
    }

    return ret;
}

/** Single interface for pushing requests */
bool
RoccInterface::pushRequest(RoccCmdPtr request)
{
    /* We just enqueue the requests if we have space
     * and we ask the interface to send the requests.
     * If the interface wasn't used to send a request,
     * and the queues are empty, we just send this new
     * request in the same cycle. */

    /* Statue of pushing the request */
    bool status = false;

    /* Check if the we have space */
    if (!requests.isFull()) {
        /* Enqueue the new request */
        requests.enqueue(request);

        /* Try sending the new request */
        sendQueuedRequests();

        /* Done :) */
        status = true;
    }

    /* Return whether we accepted the request or not */
    return status;
}

/** RoCC port interface */
bool
RoccInterface::recvTimingResp(PacketPtr pkt)
{
    /* Down-cast the response packet */
    RoccRespPtr resp = safe_cast<RoccRespPtr>(pkt);

    /* Define a return value */
    bool ret = false;

    /* Add the response to the queue if we have a space */
    if (!responses.isFull()) {
        /* We can accomidate this response */
        responses.enqueue(resp);

        /* Book-keeping! */
        inflights--;

        /* We have queued the response */
        ret = true;
    }

    /* Return outcome of reciving the response */
    return ret;
}

void
RoccInterface::recvReqRetry()
{
    //DPRINTF(MinorMem, "Received retry request\n");

    /* Check that we are waiting for retry */
    assert(state == NeedsRetry);

    /* Check request queue is not empty */
    assert(!requests.isEmpty());

    /* Unblock the channel */
    state = Running;

    /* We just send the head of the queue */
    sendQueuedRequests();
}

void
RoccInterface::sendRespRetry()
{
    roccPort.sendRetryResp();
}

void
RoccInterface::recvTimingSnoopReq(PacketPtr pkt)
{
    // Nothing to implement
}

/** Internal function to send requests */
void
RoccInterface::sendQueuedRequests()
{
    /* Check if we have a request at the top of the queue */
    if (!requests.isEmpty()) {
        /* Get the request at the front of the queue */
        RoccCmdPtr req = requests.front();

        /* Try sending the request */
        bool sent = trySendingRequest(req);

        /* Check sending state and do some book-keeping */
        if (sent) {
          /* Remove request */
          req = requests.pop();

          /* Free */
          delete req;

          /* Book-keeping */
          inflights++;
        }
    }
}

bool
RoccInterface::trySendingRequest(RoccCmdPtr req)
{
    bool ret = false;

    /* Double check that the channel is running */
    if ((state == Running) && (channel_state == Not_Used)) {
        /* Get a packet pointer */
        PacketPtr pkt = safe_cast<PacketPtr>(req);

        /* Send the request */
        ret = roccPort.sendTimingReq(pkt);

        /* Indicate channel has been used */
        channel_state = Used;

        /* Block the channel if failed */
        if (!ret) {
            state = NeedsRetry;
        }
    }

    /* Return sending status */
    return ret;
}

}
