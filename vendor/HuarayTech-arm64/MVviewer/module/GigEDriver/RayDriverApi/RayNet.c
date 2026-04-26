// RayNet.c

#include "RayNet.h"

#include <linux/ip.h>
#include <linux/if_ether.h>
#include <linux/mm.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/inet.h>
#include <linux/ip.h>
#include <linux/udp.h>

///////////////////////////////////////////////////////////
// <1> linux network

RAY_BUILD_BUG_ON( sizeof(RAY_sk_buff_t) < sizeof(struct sk_buff) );

int RAY_dev_queue_xmit( RAY_sk_buff_t * skbuf )
{
	return dev_queue_xmit( (struct sk_buff *)skbuf );
}

int RAY_skb_copy_bits( RAY_sk_buff_t * skbuf, int offset, void * buf, int len )
{
	return skb_copy_bits( (struct sk_buff *)skbuf, offset, buf, len );
}

int RAY_sk_buff_get_len( RAY_sk_buff_t * skbuf )
{
	return ((struct sk_buff *)skbuf)->len;
}

int RAY_sk_buff_get_udp_len( RAY_sk_buff_t * skbuf )
{
	struct iphdr  * ipv4 = ip_hdr( (struct sk_buff *)skbuf );
	struct udphdr * udpHeader = (struct udphdr *) ((u8 *) ipv4 + (ipv4->ihl << 2));
	return htons( udpHeader->len );
}

int RAY_sk_buff_get_udp_source( RAY_sk_buff_t * skbuf )
{
	struct iphdr  * ipv4 = ip_hdr( (struct sk_buff *)skbuf );
	struct udphdr * udpHeader = (struct udphdr *) ((u8 *) ipv4 + (ipv4->ihl << 2));
	return htons( udpHeader->source );
}

int RAY_sk_buff_get_udp_dest( RAY_sk_buff_t * skbuf )
{
	struct iphdr  * ipv4 = ip_hdr( (struct sk_buff *)skbuf );
	struct udphdr * udpHeader = (struct udphdr *) ((u8 *) ipv4 + (ipv4->ihl << 2));
	return htons( udpHeader->dest );
}

int RAY_sk_buff_get_udp_payload_offset( RAY_sk_buff_t * skbuf )
{
	struct iphdr * ipv4 = ip_hdr( (struct sk_buff *)skbuf );
	return (ipv4->ihl << 2) + 8;
}

int RAY_sk_buff_get_ipv4_protocol( RAY_sk_buff_t * skbuf )
{
	struct iphdr * ipv4 = ip_hdr( (struct sk_buff *)skbuf );
	return ipv4->protocol;
}

unsigned int RAY_sk_buff_get_ipv4_saddr( RAY_sk_buff_t * skbuf )
{
	struct iphdr * ipv4 = ip_hdr( (struct sk_buff *)skbuf );
	return ipv4->saddr;	
}

unsigned int RAY_sk_buff_get_ipv4_daddr( RAY_sk_buff_t * skbuf )
{
	struct iphdr * ipv4 = ip_hdr( (struct sk_buff *)skbuf );
	return ipv4->daddr;	
}

void RAY_sk_buff_get_eth_source( RAY_sk_buff_t * skbuf, void * buf )
{
	struct ethhdr * eth = eth_hdr( (struct sk_buff *)skbuf );
	memcpy( buf, eth->h_source, ETH_ALEN );
	return;
}

void RAY_sk_buff_get_eth_dest( RAY_sk_buff_t * skbuf, void * buf )
{
	struct ethhdr * eth = eth_hdr( (struct sk_buff *)skbuf );
	memcpy( buf, eth->h_dest, ETH_ALEN );
	return;
}

int RAY_sk_buff_get_eth_protocol( RAY_sk_buff_t * skbuf )
{
	struct ethhdr * eth = eth_hdr( (struct sk_buff *)skbuf );
	return htons( eth->h_proto );
}

RAY_sk_buff_t * RAY_alloc_skb( int skb_len )
{
	return (RAY_sk_buff_t *)alloc_skb( skb_len, GFP_ATOMIC );
}

void RAY_kfree_skb( RAY_sk_buff_t * skbuf )
{
	kfree_skb( (struct sk_buff *)skbuf );
	return;
}

int RAY_skb_init_custom_1( RAY_sk_buff_t * skb, const char * devname )
{
	struct sk_buff * s = (struct sk_buff *)skb;
	s->dev = dev_get_by_name( &init_net, devname );
	s->pkt_type = PACKET_OTHERHOST;
	s->protocol = htons( ETH_P_IP );
	s->ip_summed = CHECKSUM_NONE;
	s->priority = 0;
	return 0;
}

void RAY_skb_reserve( RAY_sk_buff_t * skb, int len )
{
	skb_reserve( (struct sk_buff *)skb, len );
}

unsigned char * RAY_skb_push( RAY_sk_buff_t * skb, unsigned int len)
{
	return skb_push( (struct sk_buff *)skb, len );
}

void RAY_skb_set_udp_header( RAY_sk_buff_t * skb, unsigned int srcport_ho, unsigned int dstport_ho, int payload_len )
{
	struct udphdr * udp_header = (struct udphdr *)skb_push( (struct sk_buff *)skb, sizeof(struct udphdr) );
	udp_header->source = htons( srcport_ho );
	udp_header->dest = htons( dstport_ho );
	udp_header->len = htons( RAY_UDPHDR_LEN + payload_len );
	udp_header->check = 0;
	skb_reset_transport_header( (struct sk_buff *)skb );	
	return;
}

void RAY_skb_set_ip_header( RAY_sk_buff_t * skb, int ver, int proto, unsigned int sip, unsigned int dip, int ttl, int tot_len )
{
	struct iphdr * ip_header = (struct iphdr *)skb_push( (struct sk_buff *)skb, sizeof(struct iphdr) );
	ip_header->version = ver;
	ip_header->ihl = sizeof(struct iphdr) >> 2;
	ip_header->frag_off = 0;
	ip_header->protocol = proto;
	ip_header->tos = 0;
	ip_header->daddr = dip;
	ip_header->saddr = sip;
	ip_header->ttl = ttl;
	ip_header->tot_len = htons( tot_len );
	ip_header->check = 0;
	skb_reset_network_header( (struct sk_buff *)skb );
	return;
}

void RAY_skb_set_checksum_udp( RAY_sk_buff_t * skb )
{
	struct iphdr * ip_header = ip_hdr( (struct sk_buff *)skb );
	struct udphdr * udp_header = udp_hdr( (struct sk_buff *)skb );
	struct sk_buff * s = (struct sk_buff *)skb;
	s->csum = skb_checksum( s, ip_header->ihl * 4, s->len - ip_header->ihl * 4, 0 );
	ip_header->check = ip_fast_csum( ip_header, ip_header->ihl );
	udp_header->check = csum_tcpudp_magic( ip_header->saddr, ip_header->daddr, s->len - ip_header->ihl * 4, IPPROTO_UDP, s->csum );
	return;
}

void RAY_skb_set_mac_header( RAY_sk_buff_t * skb, void * dstmac, void * srcmac, int h_proto )
{
	struct ethhdr * eth_header = (struct ethhdr *)skb_push( (struct sk_buff *)skb, sizeof(struct ethhdr) );

	memcpy(eth_header->h_dest, dstmac, ETH_ALEN);
	memcpy(eth_header->h_source, srcmac, ETH_ALEN);
	eth_header->h_proto = htons( h_proto );
	skb_reset_mac_header( (struct sk_buff *)skb );
	return; 
}

