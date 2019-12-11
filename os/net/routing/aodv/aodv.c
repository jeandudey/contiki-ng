#include "net/routing/routing.h"

/*---------------------------------------------------------------------------*/
static void
init(void)
{
}
/*---------------------------------------------------------------------------*/
static void
root_set_prefix(uip_ipaddr_t *prefix, uip_ipaddr_t *iid)
{
}
/*---------------------------------------------------------------------------*/
static int
root_start(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
node_is_root(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
get_root_ipaddr(uip_ipaddr_t *ipaddr)
{
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
}
/*---------------------------------------------------------------------------*/
static int
node_has_joined(void)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
node_is_reachable(void)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
static void
global_repair(const char *str)
{
}
/*---------------------------------------------------------------------------*/
static void
local_repair(const char *str)
{
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
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
ext_header_hbh_update(uint8_t *ext_buf, int opt_offset)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
ext_header_srh_update(void)
{
  return 0; /* Means SRH not found */
}
/*---------------------------------------------------------------------------*/
static int
ext_header_srh_get_next_hop(uip_ipaddr_t *ipaddr)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
link_callback(const linkaddr_t *addr, int status, int numtx)
{
}
/*---------------------------------------------------------------------------*/
static void
neighbor_state_changed(uip_ds6_nbr_t *nbr)
{
}
/*---------------------------------------------------------------------------*/
static void
drop_route(uip_ds6_route_t *route)
{
}
/*---------------------------------------------------------------------------*/
static uint8_t
is_in_leaf_mode(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
const struct routing_driver aodv_driver = {
  "aodv",
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
