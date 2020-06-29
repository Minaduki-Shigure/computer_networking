# Copyright 2012 James McCauley
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at:
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Modefied by Minaduki Shigure @ NJU, June 2020.
# Running the switch as a static router.
#

"""
This component is for use with the OpenFlow tutorial.

It acts as a simple hub, but can be modified to act like an L2
learning switch.

It's roughly similar to the one Brandon Heller did for NOX.
"""

from pox.core import core
import pox.openflow.libopenflow_01 as of
from netaddr import *
from pox.lib.revent import *
from pox.lib.packet.ethernet import ethernet
from pox.lib.packet.ipv4 import ipv4
from pox.lib.packet.arp import arp
from pox.lib.packet.icmp import icmp, echo
from pox.lib.packet.icmp import TYPE_ECHO_REQUEST, TYPE_ECHO_REPLY, TYPE_DEST_UNREACH, CODE_UNREACH_NET, CODE_UNREACH_HOST
from pox.lib.addresses import IPAddr, EthAddr
from pox.lib.util import str_to_bool, dpid_to_str


log = core.getLogger()



class Router (object):
  """
  A Router object is created for each switch that connects.
  A Connection object for that switch is passed to the __init__ function.
  """
  def __init__ (self, connection):
    # Keep track of the connection to the switch so that we can
    # send it messages!
    self.connection = connection

    # This binds our PacketIn event listener
    connection.addListeners(self)

    # Use this table to keep track of which ethernet address is on
    # which switch port (keys are MACs, values are ports).
    # self.mac_to_port = {}

    # ARP cache
    self.arp_cache = {}
    # Init ARP record for router itself.
    self.arp_cache['10.0.1.1'] = 'FF:FF:FF:FF:FF:01'
    self.arp_cache['10.0.2.1'] = 'FF:FF:FF:FF:FF:02'
    self.arp_cache['10.0.3.1'] = 'FF:FF:FF:FF:FF:03'
    # Default MAC that mininet use starts from all zeros, 
    # so we start ours from all ones.

    # Routing table (create a structure with all of the information statically assigned)
    self.routing_table = {}
    self.routing_table['10.0.1.0/24'] = {'gateway_ip': '10.0.1.1', 'port': 1}
    self.routing_table['10.0.2.0/24'] = {'gateway_ip': '10.0.2.1', 'port': 2}
    self.routing_table['10.0.3.0/24'] = {'gateway_ip': '10.0.3.1', 'port': 3}

    # Gateway IP to switch port
    self.ip2port_dict = {}
    self.ip2port_dict['10.0.1.1'] = 1
    self.ip2port_dict['10.0.2.1'] = 2
    self.ip2port_dict['10.0.3.1'] = 3

    # Message queue (while the router waits for an ARP reply)
    self.msg_queue = {}


  def resend_packet (self, packet_in, out_port):
    """
    Instructs the switch to resend a packet that it had sent to us.
    "packet_in" is the ofp_packet_in object the switch had sent to the
    controller due to a table-miss.
    """
    msg = of.ofp_packet_out()
    msg.data = packet_in

    # Add an action to send to the specified port
    action = of.ofp_action_output(port = out_port)
    msg.actions.append(action)

    # Send message to switch
    self.connection.send(msg)


  def _handle_ARP (self, packet, packet_in):
    """
    Handles ARP frame.
    """
    # Read ARP info.
    arp_body = packet.payload
    hwdst = arp_body.hwdst
    hwsrc = arp_body.hwsrc
    protodst = arp_body.protodst
    protosrc = arp_body.protosrc
    opcode = arp_body.opcode

    # The type of variables need to be converted.
    hwdst = str(hwdst)
    hwsrc = str(hwsrc)
    protodst = str(protodst)
    protosrc = str(protosrc)

    # New host here
    if protosrc not in self.arp_cache.keys():
      self.arp_cache[protosrc] = hwsrc
      # print(self.arp_cache)
      self.ip2port_dict[protosrc] = packet_in.in_port
      log.debug("Updated MAC and port for ip %s : Port #%d, MAC: %s" % (protosrc, packet_in.in_port, hwsrc))
      for subnet in self.routing_table.keys():
        if IPAddress(protodst) in IPNetwork(subnet):
          myhwaddr = self.arp_cache[self.routing_table[subnet]['gateway_ip']]
          break;
      
    if opcode == arp.REQUEST:
      log.debug("Handling ARP REQUEST frame:")
      log.debug(arp_body._to_str())

      if protodst in self.arp_cache.keys():
        log.debug("Replying that %s has %s" % (self.arp_cache[protodst], protodst))
        arp_reply = arp()
        arp_reply.opcode = arp.REPLY
        arp_reply.hwsrc = EthAddr(self.arp_cache[protodst])
        arp_reply.hwdst = arp_body.hwsrc
        arp_reply.protosrc = arp_body.protodst
        arp_reply.protodst = arp_body.protosrc

        ether = ethernet()
        ether.type = ether.ARP_TYPE
        ether.src = EthAddr(self.arp_cache[protodst])
        ether.dst = arp_body.hwsrc
        ether.payload = arp_reply

        self.resend_packet(ether, packet_in.in_port)
        log.debug("ARP reply sent to port #%d" % packet_in.in_port)

    elif opcode == arp.REPLY:
      log.debug("Handling ARP REPLY frame:")
      log.debug(arp_body._to_str())

      if protosrc in self.msg_queue.keys():
        log.debug("ARP received: %s has %s" % (hwsrc, protosrc))
        
        ether = ethernet()
        ether.type = ether.IP_TYPE
        ether.src = EthAddr(self.arp_cache[self.routing_table[self.msg_queue[protosrc]['dstsubnet']]['gateway_ip']])
        ether.dst = EthAddr(hwsrc)
        ether.payload = self.msg_queue[protosrc]['ip_packet']
        self.resend_packet(ether, packet_in.in_port)
        self.msg_queue.pop(protosrc)

    else:
      log.warning("Unsupported ARP opcode %d. Ignored." % opcode)


  def _reply_ICMP (self, packet, packet_in):
    """
    Replys ICMP packet.
    """
    ip_packet = packet.payload
    icmp_body = ip_packet.payload
    # log.debug()

    if icmp_body.type == TYPE_ECHO_REQUEST:
      log.debug("ICMP echo request from %s" % str(ip_packet.srcip))
      icmp_reply = icmp_body
      icmp_reply.type = TYPE_ECHO_REPLY

      ip_reply = ipv4()
      ip_reply.protocol = ipv4.ICMP_PROTOCOL
      ip_reply.srcip = ip_packet.dstip
      ip_reply.dstip = ip_packet.srcip
      ip_reply.payload = icmp_reply

      ether = ethernet()
      ether.type = ethernet.IP_TYPE
      ether.src = packet.dst
      ether.dst = packet.src
      ether.payload = ip_reply

      self.resend_packet(ether, packet_in.in_port)
      log.debug("ICMP echo reply sent to %s" % ip_reply.dstip)

    else:
      log.warning("I am not supposed to reply to ICMP type %d. Dropping." % icmp_body.type)
      

  def _handle_IPv4 (self, packet, packet_in):
    """
    Handles IPv4 diagram.
    """

    ip_packet = packet.payload # This is the packet payload.

    srcip = ip_packet.srcip
    dstip = ip_packet.dstip
    srcip = str(srcip)
    dstip = str(dstip)
    is_routable = False

    for subnet in self.routing_table:
      if IPAddress(dstip) in IPNetwork(subnet):
        is_routable = True
        dstsubnet = subnet
        log.debug("IP diagram routable to subnet %s" % dstsubnet)
        break

    if is_routable:
      if self.routing_table[dstsubnet]['gateway_ip'] == dstip:
        if ip_packet.protocol == ipv4.ICMP_PROTOCOL:
          self._reply_ICMP(packet, packet_in)
        else:
          log.warning("I am not supposed to reply to any IP diagrams. Dropping.")
      else:
        out_port = self.ip2port_dict[self.routing_table[dstsubnet]['gateway_ip']]
        log.debug("Trying to forward IP diagram to subnet %s at port %d." % (dstsubnet, out_port))
        if dstip not in self.arp_cache.keys():
          self.msg_queue[dstip] = {'dstsubnet': dstsubnet, 'ip_packet': ip_packet}
          log.debug("The owner of %s unknown. Flooding ARP request to port #%d." % (dstip, out_port))

          arp_body = arp()
          arp_body.opcode = arp.REQUEST
          arp_body.protosrc = IPAddr(self.routing_table[dstsubnet]['gateway_ip'])
          arp_body.protodst = ip_packet.dstip
          arp_body.hwsrc = EthAddr(self.arp_cache[self.routing_table[dstsubnet]['gateway_ip']])
          arp_body.hwdst = EthAddr('FF:FF:FF:FF:FF:FF')

          ether = ethernet()
          ether.type = ethernet.ARP_TYPE
          ether.src = EthAddr(self.arp_cache[self.routing_table[dstsubnet]['gateway_ip']])
          ether.dst = EthAddr('FF:FF:FF:FF:FF:FF')
          ether.payload = arp_body

          self.resend_packet(ether, out_port)

        else:
          fwd = packet
          fwd.src = EthAddr(self.arp_cache[self.routing_table[dstsubnet]['gateway_ip']])
          fwd.dst = EthAddr(self.arp_cache[dstip])
          self.resend_packet(fwd, out_port)
          log.debug("IP diagram forwarded to %s." % fwd.dst)

          # Install new flow
          log.debug("Installing flow...")
          log.debug("Flow added: MATCH: nw_dst : %s" % dstip)
          log.debug("Flow added: ACTION: set_src : %s" % self.arp_cache[self.routing_table[dstsubnet]['gateway_ip']])
          log.debug("Flow added: ACTION: set_dst : %s" % self.arp_cache[dstip])
          log.debug("Flow added: ACTION: output : #%d" % self.ip2port_dict[self.routing_table[dstsubnet]['gateway_ip']])

          msg = of.ofp_flow_mod()
          ## Set fields to match received packet
          msg.match.dl_type = ethernet.IP_TYPE
          msg.match.nw_dst = ip_packet.dstip
          
          #< Set other fields of flow_mod (timeouts? buffer_id?) >
          msg.idle_timeout = 60
          msg.hard_timeout = 600
          msg.flags = 3

          #< Add an output action, and send -- similar to resend_packet() >
          msg.actions.append(of.ofp_action_dl_addr.set_src(self.arp_cache[self.routing_table[dstsubnet]['gateway_ip']]))
          msg.actions.append(of.ofp_action_dl_addr.set_dst(self.arp_cache[dstip]))
          msg.actions.append(of.ofp_action_output(port=self.ip2port_dict[self.routing_table[dstsubnet]['gateway_ip']]))

          self.connection.send(msg)
          log.debug("New flow configured.")

    else:
      log.debug("Destination %s is unreachable. Replying with ICMP Unreachable." % dstip)
      
      for subnet in self.routing_table:
        if IPAddress(srcip) in IPNetwork(subnet):
          srcsubnet = subnet
          break

      icmp_reply = icmp()
      icmp_reply.type = TYPE_DEST_UNREACH
      icmp_reply.code = CODE_UNREACH_NET
      icmp_reply.payload = ip_packet.payload.payload

      ip_reply = ipv4()
      ip_reply.protocol = ipv4.ICMP_PROTOCOL
      ip_reply.srcip = IPAddr(self.routing_table[srcsubnet]['gateway_ip'])
      ip_reply.dstip = ip_packet.srcip
      ip_reply.payload = icmp_reply

      ether = ethernet()
      ether.type = ethernet.IP_TYPE
      ether.src = packet.dst
      ether.dst = packet.src
      ether.payload = ip_reply

      self.resend_packet(ether, packet_in.in_port)


  def act_like_router (self, packet, packet_in):
    """
    Implement router-like behavior.
    """

    # Learn the port for the source MAC
    # self.mac_to_port[packet.src] = packet_in.in_port
    # log.debug("Updated MAC for port %d : %s" % (packet_in.in_port, packet.src))

    if packet.type == ethernet.ARP_TYPE:
      log.debug("ARP frame received from port #%d" % packet_in.in_port)
      self._handle_ARP(packet, packet_in)
    elif packet.type == ethernet.IP_TYPE:
      log.debug("IPv4 diagram received from port #%d" % packet_in.in_port)
      self._handle_IPv4(packet, packet_in)
    else:
      log.warning("Unsupported frame received from port #%d. Dropping." % packet_in.in_port)


  def _handle_PacketIn (self, event):
    """
    Handles packet in messages from the switch.
    """
    packet = event.parsed # This is the parsed packet data.
    if not packet.parsed:
      log.warning("Ignoring incomplete packet")
      return

    packet_in = event.ofp # The actual ofp_packet_in message.

    # Comment out the following line and uncomment the one after
    # when starting the exercise.
    self.act_like_router(packet, packet_in)



def launch ():
  """
  Starts the component
  """
  def start_switch (event):
    log.debug("Controlling %s" % (event.connection,))
    Router(event.connection)
  core.openflow.addListenerByName("ConnectionUp", start_switch)
