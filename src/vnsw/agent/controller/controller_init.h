/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

#ifndef __VNSW_CONTROLLER_INIT_HPP__
#define __VNSW_CONTROLLER_INIT_HPP__

#include <sandesh/sandesh_trace.h>
#include <discovery/client/discovery_client.h>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <controller/controller_cleanup_timer.h>
#include "xmpp/xmpp_channel.h"

class AgentXmppChannel;
class AgentDnsXmppChannel;
class AgentIfMapVmExport;
class BgpPeer;
class XmlBase;

class ControllerWorkQueueData {
public:
    ControllerWorkQueueData() {}
    virtual ~ControllerWorkQueueData() {}

private:
    DISALLOW_COPY_AND_ASSIGN(ControllerWorkQueueData);
};

class ControllerDeletePeerData : public ControllerWorkQueueData {
public:
    ControllerDeletePeerData(BgpPeer *bgp_peer) :
        ControllerWorkQueueData(),
        bgp_peer_(bgp_peer) {}
    virtual ~ControllerDeletePeerData() {}

    BgpPeer *bgp_peer() const {return bgp_peer_;}

private:
    BgpPeer *bgp_peer_;
    DISALLOW_COPY_AND_ASSIGN(ControllerDeletePeerData);
};

class ControllerXmppData : public ControllerWorkQueueData {
public:
    ControllerXmppData(xmps::PeerId peer_id, xmps::PeerState peer_state,
                       uint8_t channel_id, std::auto_ptr<XmlBase> dom,
                       bool config) :
        ControllerWorkQueueData(),
        peer_id_(peer_id), peer_state_(peer_state), channel_id_(channel_id),
        dom_(dom), config_(config) { }
    virtual ~ControllerXmppData() { }

    xmps::PeerId peer_id() const {return peer_id_;}
    xmps::PeerState peer_state() const {return peer_state_;}
    uint8_t channel_id() const {return channel_id_;}
    std::auto_ptr<XmlBase> dom() {return dom_;}
    bool config() const {return config_;}

private:
    xmps::PeerId peer_id_;
    xmps::PeerState peer_state_;
    uint8_t channel_id_;
    std::auto_ptr<XmlBase> dom_;
    bool config_;
    DISALLOW_COPY_AND_ASSIGN(ControllerXmppData);
};

class VNController {
public:
    typedef boost::shared_ptr<ControllerXmppData> ControllerXmppDataType;
    typedef boost::shared_ptr<ControllerDeletePeerData> ControllerDeletePeerDataType;
    typedef boost::shared_ptr<ControllerWorkQueueData> ControllerWorkQueueDataType;
    typedef boost::shared_ptr<BgpPeer> BgpPeerPtr; 
    typedef std::list<boost::shared_ptr<BgpPeer> >::iterator BgpPeerIterator;
    VNController(Agent *agent);
    virtual ~VNController();
    void Connect();
    void DisConnect();

    void Cleanup();

    void XmppServerConnect();
    void DnsXmppServerConnect();

    void XmppServerDisConnect();
    void DnsXmppServerDisConnect();

    void ApplyDiscoveryXmppServices(std::vector<DSResponse> resp); 
    void ApplyDiscoveryDnsXmppServices(std::vector<DSResponse> resp); 

    void DisConnectControllerIfmapServer(uint8_t idx);
    void DisConnectDnsServer(uint8_t idx);

    //Multicast peer identifier
    void increment_multicast_sequence_number() {multicast_sequence_number_++;}
    uint64_t multicast_sequence_number() {return multicast_sequence_number_;}

    //Peer maintenace routines 
    uint8_t ActiveXmppConnectionCount();
    AgentXmppChannel *GetActiveXmppChannel();
    uint32_t DecommissionedPeerListSize() const {
        return decommissioned_peer_list_.size();
    }
    void AddToDecommissionedPeerList(boost::shared_ptr<BgpPeer> peer);

    //Unicast timer related routines
    void StartUnicastCleanupTimer(AgentXmppChannel *agent_xmpp_channel);
    void UnicastCleanupTimerExpired();
    CleanupTimer &unicast_cleanup_timer() {return unicast_cleanup_timer_;}
    void ControllerPeerHeadlessAgentDelDoneEnqueue(BgpPeer *peer);
    bool ControllerPeerHeadlessAgentDelDone(BgpPeer *peer);

    //Multicast timer
    void StartMulticastCleanupTimer(AgentXmppChannel *agent_xmpp_channel);
    void MulticastCleanupTimerExpired(uint64_t peer_sequence);
    CleanupTimer &multicast_cleanup_timer() {return multicast_cleanup_timer_;}

    AgentIfMapVmExport *agent_ifmap_vm_export() const {
        return agent_ifmap_vm_export_.get();
    }
    void StartConfigCleanupTimer(AgentXmppChannel *agent_xmpp_channel);
    CleanupTimer &config_cleanup_timer() {return config_cleanup_timer_;}

    // Clear of decommissioned peer listener id for vrf specified
    void DeleteVrfStateOfDecommisionedPeers(DBTablePartBase *partition, 
                                            DBEntryBase *e);
    bool ControllerWorkQueueProcess(ControllerWorkQueueDataType data);
    bool XmppMessageProcess(ControllerXmppDataType data);
    Agent *agent() {return agent_;}
    void Enqueue(ControllerWorkQueueDataType data);

private:
    AgentXmppChannel *FindAgentXmppChannel(const std::string &server_ip);
    AgentDnsXmppChannel *FindAgentDnsXmppChannel(const std::string &server_ip);
    void DeleteConnectionInfo(const std::string &addr, bool is_dns) const;
    const std::string MakeConnectionPrefix(bool is_dns) const;

    Agent *agent_;
    uint64_t multicast_sequence_number_;
    std::list<boost::shared_ptr<BgpPeer> > decommissioned_peer_list_;
    boost::scoped_ptr<AgentIfMapVmExport> agent_ifmap_vm_export_;
    UnicastCleanupTimer unicast_cleanup_timer_;
    MulticastCleanupTimer multicast_cleanup_timer_;
    ConfigCleanupTimer config_cleanup_timer_;
    WorkQueue<ControllerWorkQueueDataType> work_queue_;
};

extern SandeshTraceBufferPtr ControllerTraceBuf;

#define CONTROLLER_TRACE(obj, ...)\
do {\
    AgentXmpp##obj::TraceMsg(ControllerTraceBuf, __FILE__, __LINE__, __VA_ARGS__);\
} while(0);\

#endif
