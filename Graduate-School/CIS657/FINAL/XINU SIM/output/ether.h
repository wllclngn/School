/* ether.h - Generated stub header for XINU OS simulation
 * Generated on: 2025-06-18 22:15:49
 */
#ifndef _ETHER_H_
#define _ETHER_H_

#include "base_types.h"

/* Ethernet device driver stub */
#include "device.h"      /* Include device definitions */

/* Basic Ethernet definitions */
#define ETH_ADDR_LEN   6       /* Ethernet address length  */
#define ETH_HDR_LEN    14      /* Ethernet header length   */
#define ETH_CRC_LEN    4       /* Ethernet CRC length      */

/* Structure of an Ethernet address */
typedef struct {
    byte addr[ETH_ADDR_LEN];   /* Ethernet MAC address */
} eth_addr_t;

#endif /* _ETHER_H_ */
