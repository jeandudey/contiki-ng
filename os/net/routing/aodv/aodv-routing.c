/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * @file aodv.c
 * @author Locha Mesh project developers (locha.io)
 * @brief Implementation of:
 *
 *          Ad Hoc On-demand Distance Vector Version
 *            https://tools.ietf.org/html/rfc3561
 *
 * @version 0.1
 * @date 2019-12-10
 *
 * @copyright Copyright (c) 2019 Locha Mesh project developers
 * @license Apache 2.0, see LICENSE file for details
 */

#include "net/ipv6/uip.h"
#include "sys/process.h"
#include "sys/log.h"

#include "aodv-routing.h"
#include "aodv-conf.h"
#include "aodv-defs.h"
#include "aodv-rt.h"

#define LOG_MODULE "aodv"
#define LOG_LEVEL LOG_LEVEL_INFO

PROCESS(aodv_process, "AODV");
/*---------------------------------------------------------------------------*/
static struct uip_udp_conn* multicast_tx_conn; /* Used to send multicast packets */
static struct uip_udp_conn* multicast_rx_conn; /* Used to receive multicast packets */
static struct uip_udp_conn* unicast_tx_conn;   /* Used to send unicast packets */
/*---------------------------------------------------------------------------*/
void aodv_routing_init(void)
{
    process_start(&aodv_process, NULL);
}
/*---------------------------------------------------------------------------*/
/* Compare sequence numbers as per RFC 3561 on Section 6.1 "Maintaining
 * Sequence Numbers". */
#define SCMP32(a, b) ((int32_t)((int32_t)(a) - (int32_t)(b)))
/*---------------------------------------------------------------------------*/
/**
 * @brief Look the last known sequence number for the host.
 *
 * @param host: Host address.
 *
 * @return Sequence number in network byte order.
 */
static CC_INLINE uint32_t
last_known_seqno(uip_ipaddr_t *host)
{
    aodv_rt_entry_t *route = NULL;

    route = aodv_rt_lookup_any(host);

    if (route != NULL) {
        /* Convert the sequence number to network byte order. */
        return uip_htonl(route->hseqno);
    }

    /* Not found. */
    return 0;
}
/*---------------------------------------------------------------------------*/
static uint32_t rreq_id = 0;   /*!< Current RREQ ID */
static uint32_t my_hseqno = 0; /*!< In host byte order! */
/*---------------------------------------------------------------------------*/
#if 0
static struct {
    uip_ipaddr_t orig;
    uint32_t id;
} fwcache[AODV_NUM_FW_CACHE];

static CC_INLINE int
fwc_lookup(const uip_ipaddr_t *orig, const uint32_t *id)
{
    /* TODO: analyze this modulo reduction. See above comment. */
    unsigned n = (orig->u8[2] + orig->u8[3]) % AODV_NUM_FW_CACHE;
    return fwcache[n].id == *id && uip_ipaddr_cmp(&fwcache[n].orig, orig);
}

static CC_INLINE void
fwc_add(const uip_ipaddr_t *orig, const uint32_t *id)
{
    /* TODO: analyze this modulo reduction. Possible security issue. */
    unsigned n = (orig->u8[2] + orig->u8[3]) % AODV_NUM_FW_CACHE;
    fwcache[n].id = *id;
    uip_ipaddr_copy(&fwcache[n].orig, orig);
}
#endif
/*---------------------------------------------------------------------------*/
#define uip_udp_sender() (&(UIP_IP_BUF->srcipaddr))
/*---------------------------------------------------------------------------*/
static void
sendto(const uip_ipaddr_t *dest, const void *buf, int len)
{
  /* XXX: this is a HACK! We're updating the uIP UDP connection
     "unicastconn" so that the destination address is the next-hop,
     and we're patching the "uip_udp_conn" variable so that it points
     the this connection instead. THIS IS NOT A NICE WAY TO DO THIS,
     but it is currently nicer than the alternative (requesting a new
     poll, and remembering the state, etc.). */

  uip_ipaddr_copy(&unicast_tx_conn->ripaddr, dest);
  uip_udp_conn = unicast_tx_conn;
  uip_udp_packet_send(unicast_tx_conn, buf, len);
}
/*---------------------------------------------------------------------------*/
void
aodv_send_rreq(uip_ipaddr_t *addr)
{
    aodv_msg_rreq_t rm = {0};

    LOG_INFO("sending RREQ.\n");

    rm.type = AODV_TYPE_RREQ;
    rm.dest_seqno = last_known_seqno(addr);
    if (rm.dest_seqno == 0) {
        LOG_INFO("Unknown sequence number\n");
        rm.flags = AODV_RREQ_FLAG_UNKSEQNO;
    } else {
        rm.flags = 0;
    }

    rm.reserved = 0;
    rm.hop_count = 0;
    rm.rreq_id = uip_htonl(rreq_id);
    /* Increment RREQ ID because the current is already used. */
    rreq_id += 1;
    rm.orig_seqno = uip_htonl(my_hseqno);

    uip_ipaddr_copy(&rm.dest_addr, addr);
    uip_ipaddr_copy(&rm.orig_addr, &multicast_tx_conn->ripaddr);

    /* Always. */
    my_hseqno += 1;

    multicast_tx_conn->ttl = AODV_NET_DIAMETER;
    uip_udp_packet_send(multicast_tx_conn, &rm, sizeof(aodv_msg_rreq_t));
}
/*---------------------------------------------------------------------------*/
void
aodv_send_rrep(uip_ipaddr_t *dest,
               uip_ipaddr_t *nexthop,
               uip_ipaddr_t *orig,
               uint32_t *seqno,
               unsigned hop_count)
{
    aodv_msg_rrep_t rm = {0};

    rm.type = AODV_TYPE_RREP;
    rm.flags = 0;
    rm.prefix_sz = 0;
    rm.hop_count = hop_count;
    rm.dest_seqno = *seqno;
    uip_ipaddr_copy(&rm.dest_addr, dest);
    uip_ipaddr_copy(&rm.orig_addr, orig);
    rm.lifetime = UIP_HTONL(AODV_ROUTE_TIMEOUT);

    sendto(nexthop, &rm, sizeof(aodv_msg_rrep_t));
}
/*---------------------------------------------------------------------------*/
void
aodv_send_rerr(uip_ipaddr_t *addr, uint32_t *seqno)
{
    aodv_msg_rerr_t rm = {0};

    rm.type = AODV_TYPE_RERR;
    rm.reserved = 0;
    rm.dest_count = 1;
    uip_ipaddr_copy(&rm.unreach[0].addr, addr);
    rm.unreach[0].seqno = *seqno;
    rm.flags = 0;

    uip_udp_packet_send(multicast_tx_conn, &rm, sizeof(aodv_msg_rerr_t));
}
/*---------------------------------------------------------------------------*/
static void
handle_incoming_rreq(void)
{
    LOG_INFO("Handling incoming RREP.\n");

    /*
     * Defensive coding. uip_appdata SHOULD be valid here, but
     * we test to avoid crashes at runtime.
     */
    if(uip_appdata == NULL) {
        LOG_ERR("RREQ has no data.\n");
        return;
    }

    /*
     * Check the length of the packet, if doesn't met the needed
     * size we don't process it. It can be larger, but we don't
     * worry about any extra bytes.
     */
    if(uip_datalen() < sizeof(aodv_msg_rreq_t)) {
        LOG_ERR("RREQ is too short, is %d expected at least %d.\n",
                uip_len,
                sizeof(aodv_msg_rreq_t));
        return;
    }

    aodv_msg_rreq_t *rm = (aodv_msg_rreq_t *)uip_appdata;

    /*
     * Defensive coding. This is already checked by handle_incoming_packet.
     * Anyway it's checked here to avoid a misuse of the function.
     */
    if(rm->type != AODV_TYPE_RREQ) {
        LOG_ERR("Invalid AODV message type.\n");
        return;
    }

    /* TODO: handle RREQ logic */
}
/*---------------------------------------------------------------------------*/
static void
handle_incoming_rrep(void)
{
    if(uip_appdata == NULL) {
        LOG_ERR("RREP has no data.\n");
        return;
    }

    if(uip_datalen() < sizeof(aodv_msg_rrep_t)) {
        LOG_ERR("RREP is too short, is %d expected at least %d.\n", uip_len, sizeof(aodv_msg_rrep_t));
        return;
    }

    aodv_msg_rrep_t *rm = (aodv_msg_rrep_t *)uip_appdata;

    if(rm->type != AODV_TYPE_RREP) {
        LOG_ERR("Invalid RREP message type.\n");
        return;
    }

    /* TODO: handle RREP logic */
}
/*---------------------------------------------------------------------------*/
static void
handle_incoming_rerr(void)
{
    if(uip_appdata == NULL) {
        LOG_ERR("RERR has no data.\n");
        return;
    }

    if(uip_len < sizeof(aodv_msg_rerr_t)) {
        LOG_ERR("RERR is too short, is %d expected at least %d.\n", uip_len, sizeof(aodv_msg_rerr_t));
        return;
    }

    aodv_msg_rerr_t *rm = (aodv_msg_rerr_t *)uip_appdata;

    if(rm->type != AODV_TYPE_RREP) {
        LOG_ERR("Invalid RERR message type.\n");
        return;
    }

    /* TODO: handle RREP logic */
}
/*---------------------------------------------------------------------------*/
static void
handle_incoming_packet(void)
{
    if(uip_appdata == NULL) {
        LOG_ERR("AODV message has no data.\n");
        return;
    }

    if(uip_len < sizeof(aodv_msg_t)) {
        LOG_ERR("AODV message is too short.\n");
        return;
    }

    aodv_msg_t *m = (aodv_msg_t *)uip_appdata;

    switch(m->type) {
    case AODV_TYPE_RREQ:
        handle_incoming_rreq();
        break;
    case AODV_TYPE_RREP:
        handle_incoming_rrep();
        break;
    case AODV_TYPE_RERR:
        handle_incoming_rerr();
        break;
    }
}
/*---------------------------------------------------------------------------*/
static enum {
    COMMAND_NONE,
    COMMAND_SEND_RREQ,
    COMMAND_SEND_RERR,
} command;
/*---------------------------------------------------------------------------*/
static uip_ipaddr_t bad_dest;
static uint32_t bad_seqno;      /* In network byte order! */
/*---------------------------------------------------------------------------*/
void
aodv_bad_dest(uip_ipaddr_t *dest)
{
    aodv_rt_entry_t *rt = aodv_rt_lookup_any(dest);

    if (rt == NULL) {
        bad_seqno = 0;      /* Or flag this in RERR? */
    } else {
        rt->is_bad = 1;
        bad_seqno = uip_htonl(rt->hseqno);
    }

    uip_ipaddr_copy(&bad_dest, dest);
    command = COMMAND_SEND_RERR;
    process_post(&aodv_process, PROCESS_EVENT_MSG, NULL);
}
/*---------------------------------------------------------------------------*/
static uip_ipaddr_t rreq_addr;
static struct timer next_time;
/*---------------------------------------------------------------------------*/
aodv_rt_entry_t *
aodv_request_route_to(uip_ipaddr_t *host)
{
    LOG_INFO("Requesting route to ");
    LOG_INFO_6ADDR(host);
    LOG_INFO_(".\n");
    /* Look on the Routing Table if this route is know. */
    aodv_rt_entry_t *route = aodv_rt_lookup(host);

    if (route != NULL) {
        LOG_INFO("Route exists in table.\n");
        /* The route exists; set as the Least Recently Used. */
        aodv_rt_lru(route);
        return route;
    }

    /* Broadcast protocols must be rate-limited! */
    if(!timer_expired(&next_time)) {
        LOG_WARN("Route request has been made before timeout!\n");
        return NULL;
    }

    /* We are processing another command. */
    if (command != COMMAND_NONE) {
        LOG_WARN("A command is being processed!\n");
        return NULL;
    }

    LOG_DBG("Sending command to aodv_process.\n");

    uip_ipaddr_copy(&rreq_addr, host);
    command = COMMAND_SEND_RREQ;
    process_post(&aodv_process, PROCESS_EVENT_MSG, NULL);
    timer_set(&next_time, CLOCK_SECOND/8); /* Max 10/s per RFC3561. */
    return NULL;
}

PROCESS_THREAD(aodv_process, ev, data)
{
    PROCESS_EXITHANDLER(goto exit);

    PROCESS_BEGIN();

    /*
     * Create broadcast UDP connection to send packets to the ff02::1,
     * This connection is ONLY to send packets, it doesn't receive anything.
     */
    multicast_tx_conn = udp_broadcast_new(UIP_HTONS(AODV_UDPPORT), NULL);

    /*
     * Create a UDP connection to receive (RX) packets sent to a ff02:1 address.
     * This connection is ONLY to receive packets, it SHOULD NOT be used to
     * send anything as it does not has a Remote Address.
     */
    multicast_rx_conn = udp_new(NULL, 0, NULL);
    if(multicast_rx_conn == NULL) {
        LOG_ERR("Couldn't create multicast connection.\n");
        PROCESS_EXIT();
    }
    /*
     * Bind the connection to the AODV UDP port (per the RFC it's 654).
     */
    udp_bind(multicast_rx_conn, UIP_HTONS(AODV_UDPPORT));

    /*
     * Create a unicast connection, acually the address is changed to the address of
     * the node that the response should be sent.
     */
    unicast_tx_conn = udp_broadcast_new(UIP_HTONS(AODV_UDPPORT), NULL);

    while (1) {
        PROCESS_WAIT_EVENT();

        if (ev == tcpip_event) {
            LOG_INFO("New TCPIP event\n");
            /*
             * Check if we have received any UDP data.
             */
            if (uip_newdata()) {
                LOG_INFO("Received UDP datagram\n");
                handle_incoming_packet();
                continue;
            }

            if (uip_poll()) {
                if(command == COMMAND_SEND_RREQ) {
                    LOG_INFO("Received COMMAND_SEND_RREQ\n");
                    /*
                     * Search in the Routing Table if we have that address stored,
                     * if not, send RREQs to all near nodes.
                     */
                    if (aodv_rt_lookup(&rreq_addr) == NULL)
                        aodv_send_rreq(&rreq_addr);
                } else if (command == COMMAND_SEND_RERR) {
                    aodv_send_rerr(&bad_dest, &bad_seqno);
                }

                /*
                 * Reset the command state.
                 */
                command = COMMAND_NONE;
                continue;
            }
        }

        if (ev == PROCESS_EVENT_MSG) {
            tcpip_poll_udp(multicast_rx_conn);
        }
    }

exit:
    command = COMMAND_NONE;
    aodv_rt_flush_all();

    uip_udp_remove(multicast_tx_conn);
    multicast_tx_conn = NULL;

    uip_udp_remove(multicast_rx_conn);
    multicast_rx_conn = NULL;

    uip_udp_remove(unicast_tx_conn);
    unicast_tx_conn = NULL;

    PROCESS_END();
}
