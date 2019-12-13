/**
 * @file aodv.h
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

#include "net/routing/routing.h"
#include "net/ipv6/uip-ds6.h"
#include "sys/log.h"

#include "net/routing/aodv/aodv-defs.h"
#include "net/routing/aodv/aodv-routing.h"
#include "net/routing/aodv/aodv-rt.h"

#define LOG_MODULE "aodv"
#define LOG_LEVEL LOG_LEVEL_DBG

/*---------------------------------------------------------------------------*/
static void
init(void)
{
    LOG_INFO("Initializing AODV routing driver.\n");

    LOG_INFO("Initializing AODV routing table.\n");
    aodv_rt_init();
    LOG_INFO("Initializing AODV routing engine.\n");
    aodv_routing_init();
}

/*---------------------------------------------------------------------------*/
static void
root_set_prefix(uip_ipaddr_t *prefix, uip_ipaddr_t *iid)
{
    /* Prefixes aren't supported by the moment. */
}
/*---------------------------------------------------------------------------*/
static int
root_start(void)
{
  /* AODV doesn't has root nodes, every node is an equal. */
  return -1;
}
/*---------------------------------------------------------------------------*/
static int
node_is_root(void)
{
  /* AODV doesn't has root nodes, every node is an equal. Can't tell if it's
   * root */
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
get_root_ipaddr(uip_ipaddr_t *ipaddr)
{
  /* AODV doesn't has root nodes, every node is an equal. Can't tell if it's
   * root */
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
get_sr_node_ipaddr(uip_ipaddr_t *addr, const uip_sr_node_t *node)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
leave_network(void)
{
  /* Can't leave */
}
/*---------------------------------------------------------------------------*/
static int
node_has_joined(void)
{
  /* We aren't sure if we're part or not of the network */
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
node_is_reachable(void)
{
  /* We are always reachable, we just need to process the RREQ request */
  return 1;
}
/*---------------------------------------------------------------------------*/
static void
global_repair(const char *str)
{
  /* TODO: implement */
}
/*---------------------------------------------------------------------------*/
static void
local_repair(const char *str)
{
  /* TODO: implement */
}
/*---------------------------------------------------------------------------*/
static bool
ext_header_remove(void)
{
#if NETSTACK_CONF_WITH_IPV6
  return uip_remove_ext_hdr();
#else
  return true;
#endif /* NETSTACK_CONF_WITH_IPV6 */
}
/*---------------------------------------------------------------------------*/
static int
ext_header_update(void)
{
  /* XXX: what's this?*/
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
ext_header_hbh_update(uint8_t *ext_buf, int opt_offset)
{
  /* XXX: what's this? */
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
ext_header_srh_update(void)
{
  /* XXX: what's this?*/
  return 0; /* Means SRH not found */
}
/*---------------------------------------------------------------------------*/
static int
ext_header_srh_get_next_hop(uip_ipaddr_t *ipaddr)
{
  /* XXX: what's this? */
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
link_callback(const linkaddr_t *addr, int status, int numtx)
{
  /* XXX: what's this? */
}
/*---------------------------------------------------------------------------*/
static void
neighbor_state_changed(uip_ds6_nbr_t *nbr)
{
  /* XXX: what's this? */
}
/*---------------------------------------------------------------------------*/
static void
drop_route(uip_ds6_route_t *route)
{
  /* XXX: what's this? */
}
/*---------------------------------------------------------------------------*/
static uint8_t
is_in_leaf_mode(void)
{
  /* AODV doesn't has leafs. */
  return 0;
}
/*---------------------------------------------------------------------------*/
const struct routing_driver aodv_driver = {
  "AODV",
  init,
  root_set_prefix,
  root_start,
  node_is_root,
  get_root_ipaddr,
  get_sr_node_ipaddr,
  leave_network,
  node_has_joined,
  node_is_reachable,
  global_repair,
  local_repair,
  ext_header_remove,
  ext_header_update,
  ext_header_hbh_update,
  ext_header_srh_update,
  ext_header_srh_get_next_hop,
  link_callback,
  neighbor_state_changed,
  drop_route,
  is_in_leaf_mode,
};
/*---------------------------------------------------------------------------*/
