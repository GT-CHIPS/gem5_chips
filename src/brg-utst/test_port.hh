//========================================================================
// Test Port
//========================================================================
//
// Authors  : Tuan Ta
// Date     : Sep 26, 2018
//

#ifndef TEST_PORT_HH
#define TEST_PORT_HH

#include "mem/port.hh"
#include "test_harness.hh"

class TestHarness;

class TestPort : public MasterPort {

  public:

    TestPort(TestHarness* _test_harness,
             const std::string& _name,
             int _port_id);
    ~TestPort();

    // Port ID
    const int port_id;

    void setBlocked();
    bool isBlocked() const;

  protected:

    // Receive a response packet from the test object
    virtual bool recvTimingResp(PacketPtr pkt);

    // Receive a retry request from the test object
    virtual void recvReqRetry();

    // Receive a timing snoop request from the test object
    virtual void recvTimingSnoopReq(PacketPtr pkt)
    { panic("recvTimingSnoopReq not implemented\n"); }

    // Receive an atomic snoop request from the test object
    virtual Tick recvAtomicSnoop(PacketPtr pkt)
    { panic("recvAtomicSnoop not implemented\n"); }

    // Pointer to the test harness owning this port
    TestHarness* test_harness;

    // Is blocked?
    bool is_blocked;
};

class SystemDebugPort : public SlavePort {
  public:
    SystemDebugPort(TestHarness* _test_harness, const std::string& _name);
    ~SystemDebugPort();

  protected:
    virtual bool recvTimingReq(Packet *pkt)
    { panic("recvTimingReq Not implemented!\n"); }
    virtual void recvFunctional(Packet *pkt)
    { panic("recvFunctional Not implemented!\n"); }
    virtual Tick recvAtomic(Packet *pkt)
    { panic("recvAtomic Not implemented!\n"); }
    virtual void recvRespRetry()
    { panic("recvRespRetry Not implemented!\n"); }
    virtual AddrRangeList getAddrRanges() const
    { panic("getAddrRanges Not implemented!\n"); }

};

#endif // TEST_PORT_HH
