/*---------------------------------------------------------------------------
 * Arbiter
 *---------------------------------------------------------------------------
 *
 * Author: Tuan Ta
 */

#ifndef ARBITER_HH
#define ARBITER_HH

#include <vector>

/*
 * An arbiter arbitrates N different requests for one shared resource
 */

class Arbiter {
  public:
    Arbiter(int _num_requesters);

    /* Decide which requester can access the shared resource using round-robin
     * policy
     * request_bitset - a bitset representing active requesters
     * Return the granted requester ID
     */
    int arbitrate(const std::vector<bool>& request_bitset) const;

    /*
     * Update the last granted requester
     */
    void update(int _last_granted_requester);

  private:
    // Number of requesters
    int num_requesters;

    // The last granted requester
    int last_granted_requester;
};

#endif /* ARBITER_HH */
