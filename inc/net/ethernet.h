#ifndef ETHERNET_H_
#define ETHERNET_H_

#define ETH_ALEN	6		/* Octets in one ethernet addr	 */

struct ether_addr
{
  uint8_t ether_addr_octet[ETH_ALEN];
} __attribute__ ((__packed__));

#include <zephyr/net/ethernet.h>
#endif