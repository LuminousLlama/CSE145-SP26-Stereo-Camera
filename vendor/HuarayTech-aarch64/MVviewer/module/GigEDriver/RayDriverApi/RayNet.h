// RayNet.h

#ifndef INCLUDED_RAYNET_H
#define INCLUDED_RAYNET_H

#include "RayApi.h"

// <1> sk_buff

#define RAY_IPPROTO_IP	0
#define RAY_IPPROTO_ICMP	1
#define RAY_IPPROTO_TCP	6
#define RAY_IPPROTO_UDP	17
#define RAY_IPPROTO_IPV6	41

#define RAY_ETH_ALEN	6
#define RAY_ETH_HLEN	14

#define RAY_ETH_P_IP	0x0800
#define RAY_ETH_P_ARP	0x0806
#define RAY_ETH_P_RARP	0x8035
#define RAY_ETH_P_IPV6	0x86DD

#define RAY_ETHHDR_LEN	14
#define RAY_IPHDR_LEN	20
#define RAY_TCPHDR_LEN	20
#define RAY_UDPHDR_LEN	8

typedef struct RAY_sk_buff_st
{
	char data[256];
} RAY_sk_buff_t;

extern int RAY_skb_copy_bits( RAY_sk_buff_t * skbuf, int offset, void * buf, int len );
extern int RAY_sk_buff_get_len( RAY_sk_buff_t * skbuf );

extern int RAY_sk_buff_get_udp_len( RAY_sk_buff_t * skbuf );
extern int RAY_sk_buff_get_udp_source( RAY_sk_buff_t * skbuf );
extern int RAY_sk_buff_get_udp_dest( RAY_sk_buff_t * skbuf );
extern int RAY_sk_buff_get_udp_payload_offset( RAY_sk_buff_t * skbuf );
extern int RAY_sk_buff_get_ipv4_protocol( RAY_sk_buff_t * skbuf );
extern unsigned int RAY_sk_buff_get_ipv4_saddr( RAY_sk_buff_t * skbuf );
extern unsigned int RAY_sk_buff_get_ipv4_daddr( RAY_sk_buff_t * skbuf );
extern void RAY_sk_buff_get_eth_source( RAY_sk_buff_t * skbuf, void * buf );
extern void RAY_sk_buff_get_eth_dest( RAY_sk_buff_t * skbuf, void * buf );
extern int RAY_sk_buff_get_eth_protocol( RAY_sk_buff_t * skbuf );

extern RAY_sk_buff_t * RAY_alloc_skb( int skb_len );
extern void RAY_kfree_skb( RAY_sk_buff_t * skbuf );

extern int RAY_skb_init_custom_1( RAY_sk_buff_t * skb, const char * devname );

extern void RAY_skb_reserve( RAY_sk_buff_t * skb, int len );
extern unsigned char * RAY_skb_push( RAY_sk_buff_t * skb, unsigned int len);

extern void RAY_skb_set_udp_header( RAY_sk_buff_t * skb, unsigned int srcport_ho, unsigned int dstport_ho, int payload_len );

extern void RAY_skb_set_ip_header( RAY_sk_buff_t * skb, int ver, int proto, unsigned int sip, unsigned int dip, int ttl, int tot_len );

extern void RAY_skb_set_checksum_udp( RAY_sk_buff_t * skb );

extern void RAY_skb_set_mac_header( RAY_sk_buff_t * skb, void * dstmac, void * srcmac, int h_proto );

// <2> dev

extern int RAY_dev_queue_xmit( RAY_sk_buff_t * skbuf );

// <3> socket

#define RAY_POLLIN      0x0001
#define RAY_POLLPRI     0x0002
#define RAY_POLLOUT     0x0004
#define RAY_POLLERR     0x0008
#define RAY_POLLHUP     0x0010
#define RAY_POLLNVAL    0x0020
#define RAY_POLLRDNORM  0x0040
#define RAY_POLLRDBAND  0x0080
#define RAY_POLLWRNORM  0x0100
#define RAY_POLLWRBAND  0x0200
#define RAY_POLLMSG     0x0400
#define RAY_POLLREMOVE  0x1000
#define RAY_POLLRDHUP   0x2000

#endif // INCLUDED_RAYNET_H

