/*---------------------------------------------------------------------------
 * Arbiter
 *---------------------------------------------------------------------------
 *
 * Author: Tuan Ta
 */

#include <cassert>

#include "arbiter.hh"

Arbiter::Arbiter(int _num_requesters)
    : num_requesters(_num_requesters),
      last_granted_requester(0)
{
  assert(num_requesters > 0);
}

int
Arbiter::arbitrate(const std::vector<bool>& request_bitset) const
{
  int requester = (last_granted_requester + 1) % num_requesters;

  for (int i = 0; i < num_requesters; ++i) {
    int chosen_requester = (requester + i) % num_requesters;
    if (request_bitset[chosen_requester]) {
      return chosen_requester;
    }
  }

  // There is no request
  return -1;
}

void
Arbiter::update(int _last_granted_requester)
{
  assert(_last_granted_requester >= 0 &&
         _last_granted_requester < num_requesters);
  last_granted_requester = _last_granted_requester;
}
