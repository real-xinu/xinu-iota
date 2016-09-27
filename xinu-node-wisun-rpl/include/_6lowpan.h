/* _6lowpan.h */

#define LOWPAN_DISPATCH_MASK	0xE0

#define	LOWPAN_IPHC	0x60

/* Bit shifts in IPHC */
#define LOWPAN_IPHC_TF	3
#define LOWPAN_IPHC_NH	2
#define LOWPAN_IPHC_HL	0
#define LOWPAN_IPHC_SAM	4
#define LOWPAN_IPHC_DAM	0

#define LOWPAN_IPNHC	0xE0

/* Bit shifts in IPNHC */
#define LOWPAN_IPNHC_EID	1
#define LOWPAN_IPNHC_NH		0

#define LOWPAN_IPNHC_CHECK(x)	( ((x) == IP_IPV6) ||	\
				  ((x) == IP_HBH) )
