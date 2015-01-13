/* nd.c */

#include <xinu.h>

struct	nd_nce nd_ncache[ND_NC_SLOTS];
byte	nd_msgbuf1[500];
byte	nd_msgbuf2[500];

/*------------------------------------------------------------------------
 * nd_init  -  Initialize Neighbor Discovery related information
 *------------------------------------------------------------------------
 */
void	nd_init (
	int32	iface	/* Interface index	*/
	)
{
	struct	ifentry *ifptr;	/* Pointer to interface entry	*/
	struct	nd_info *ndptr;	/* Pointer tp ND information	*/
	intmask	mask;		/* Saved interrupt mask		*/
	int32	i;

	if((iface < 0) || (iface >= NIFACES)) {
		return;
	}

	mask = disable();

	ifptr = &if_tab[iface];

	memset((char *)&ifptr->if_ndData, 0, sizeof(ifptr->if_ndData));

	ndptr = &ifptr->if_ndData;
	ndptr->isrouter = TRUE;
	ndptr->sendadv = FALSE;
	ndptr->currhl = 255;

	for(i = 0; i < ND_NC_SLOTS; i++) {
		memset((char *)&nd_ncache[i], 0, sizeof(struct nd_nce));
		nd_ncache[i].state = ND_NCE_FREE;
	}

	resume(create(nd_in, ND_PROCSSIZE, ND_PROCPRIO, "nd_in", 1, iface));
}

/*------------------------------------------------------------------------
 * nd_in  -  ND process to handle incoming ND messages
 *------------------------------------------------------------------------
 */
process	nd_in (
	int32	iface	/* Interface index	*/
	)
{
	struct	ipinfo ipdata;	/* IP information	*/
	int32	slots[4];	/* ICMP slots		*/
	byte	*msgbuf;	/* Message buffer	*/
	uint32	buflen;		/* Buffer length	*/
	int32	retval;		/* Return value 	*/

	if((iface < 0) || (iface >= NIFACES)) {
		return SYSERR;
	}

	slots[0] = icmp_register(iface, ip_unspec, ip_unspec, ICMP_TYPE_RTRSOL, 0);
	slots[1] = icmp_register(iface, ip_unspec, ip_unspec, ICMP_TYPE_RTRADV, 0);
	slots[2] = icmp_register(iface, ip_unspec, ip_unspec, ICMP_TYPE_NBRSOL, 0);
	slots[3] = icmp_register(iface, ip_unspec, ip_unspec, ICMP_TYPE_NBRADV, 0);

	if((slots[0] == SYSERR) ||
	   (slots[1] == SYSERR) ||
	   (slots[2] == SYSERR) ||
	   (slots[3] == SYSERR)) {
	   	panic("nd_in: Error while registering icmp slots");
	}

	while(TRUE) {
		msgbuf = nd_msgbuf1;
		buflen = 500;
		retval = icmp_recvnaddr(slots,	/* ICMP slots to listen on	*/
					4,	/* No. of ICMP slots		*/
					msgbuf, /* Message buffer		*/
					&buflen,/* Buffer length		*/
					3000,	/* Timeout in ms		*/
					&ipdata	/* IP information		*/
					);
		if(retval == SYSERR) {
			panic("nd_in: error in icmp_recvnaddr");
		}
		if(retval == TIMEOUT) {
			continue;
		}
		if(retval == slots[0]) {
			kprintf("Incoming rtrsol\n");
			nd_in_rtrsol(iface, (struct nd_rtrsol *)msgbuf, buflen, &ipdata);
		}
		else if(retval == slots[1]) {
			kprintf("Incoming rtradv\n");
			nd_in_rtradv(iface, (struct nd_rtradv *)msgbuf, buflen, &ipdata);
		}
		else if(retval == slots[2]) {
			kprintf("Incoming nbrsol\n");
			nd_in_nbrsol(iface, (struct nd_nbrsol *)msgbuf, buflen, &ipdata);
		}
		else if(retval == slots[3]) {
			kprintf("Incoming nbradv\n");
		}
		else {
			panic("nd_in: Unknown message type\n");
		}
	}

	return OK;
}

/*------------------------------------------------------------------------
 * nd_in_rtrsol  -  Handle incoming Router Solicitation message
 *------------------------------------------------------------------------
 */
void	nd_in_rtrsol (
	int32	iface,			/* Interface index	*/
	struct	nd_rtrsol *rtrsol,	/* Router solicitation	*/
	uint32	len,			/* Length of message	*/
	struct	ipinfo *ipdata		/* IP information	*/
	)
{
	struct	ifentry *ifptr;	/* Pointer to interface		*/
	struct	nd_info *ndptr;	/* Pointer to ND information	*/
	struct	nd_rtradv *rtradv;/* Pointer to rtr adv		*/
	struct	nd_llao *sllao;	/* Pointer to SLLAO		*/
	struct	nd_pio *pio;	/* Pointer to PIO		*/
	byte	*optptr;	/* Pointer to current option	*/
	byte	*optend;	/* Pointer to end of message	*/
	intmask	mask;		/* Saved interrupt mask		*/
	int32	i;		/* For loop index		*/

	ifptr = &if_tab[iface];
	ndptr = &ifptr->if_ndData;

	if(ndptr->sendadv == FALSE) {
		return;
	}

	if((len < 4) ||
	   (ipdata->iphl != 255)) {
	   	return;
	}

	sllao = (struct nd_llao *)NULL;

	optptr = rtrsol->opts;
	optend = (byte *)rtrsol + len;

	while(optptr < optend) {

		switch(*optptr) {

		 case ND_TYPE_SLLAO:
		 	sllao = (struct nd_llao *)optptr;
			if(sllao->len == 0) {
				return;
			}
			optptr = (byte *)(sllao + 1);
			break;

		 default:
		 	if((*(optptr+1)) == 0) {
				return;
			}
		 	optptr += 8 * (*(optptr+1));
		}
	}

	if(sllao && isipunspec(ipdata->ipsrc)) {
		return;
	}

	rtradv = (struct nd_rtradv *)nd_msgbuf2;

	mask = disable();
	rtradv->currhl = ndptr->currhl;
	rtradv->res = 0;
	rtradv->o = ndptr->other;
	rtradv->m = ndptr->managed;
	rtradv->rtrlife = ndptr->lifetime;
	rtradv->reachable = ndptr->reachable;
	rtradv->retrans = ndptr->retrans;
	len = sizeof(struct nd_rtradv);

	sllao = (struct nd_llao *)rtradv->opts;
	sllao->type = ND_TYPE_SLLAO;
	sllao->len = 2;
	memcpy(sllao->lladdr, ifptr->if_eui64, 8);
	len += sizeof(struct nd_llao);

	pio = (struct nd_pio *)(sllao + 1);
	for(i = 1; i < ifptr->if_nipucast; i++) {
		pio->type = ND_TYPE_PIO;
		pio->len = 4;
		pio->preflen = 64;
		pio->res1 = 0;
		pio->a = 1;
		pio->l = 0;
		pio->validlife = 0;
		pio->preflife = 0;
		pio->res2 = 0;
		memcpy(pio->prefix, ifptr->if_ipucast[i].ipaddr, 8);
		memset((char *)&pio->prefix[8], 0, 8);
		len += sizeof(struct nd_pio);
		pio += 1;
	}

	memcpy(ipdata->ipdst, ipdata->ipsrc, 16);
	memcpy(ipdata->ipsrc, ifptr->if_ipucast[0].ipaddr, 16);

	restore(mask);

	icmp_send(iface, ICMP_TYPE_RTRADV, 0, ipdata, (char *)rtradv, len);
}

/*------------------------------------------------------------------------
 * nd_in_rtradv  -  Handle incoming Router Advertisement message
 *------------------------------------------------------------------------
 */
void	nd_in_rtradv (
	int32	iface,			/* Interface index	*/
	struct	nd_rtradv *rtradv,	/* Router advertisement	*/
	uint32	len,			/* Message length	*/
	struct	ipinfo *ipdata		/* IP information	*/
	)
{
	struct	ifentry *ifptr;	/* Pointer to interface		*/
	struct	nd_info *ndptr;	/* Pointer to ND information	*/
	struct	nd_llao *sllao;	/* Pointer to SLLAO		*/
	struct	nd_pio *pio;	/* Pointer to PIO		*/
	byte	*optptr;	/* Pointer to current option	*/
	byte	*optend;	/* Pointer to end of message	*/
	intmask	mask;		/* Saved interrupt mask		*/
	int32	i;		/* For loop index		*/

	ifptr = &if_tab[iface];
	ndptr = &ifptr->if_ndData;

	if((!isipllu(ipdata->ipsrc)) ||
	   (ipdata->iphl != 255) ||
	   (len < 12)) {
	   	return;
	}

	mask = disable();

	ndptr->currhl = (rtradv->currhl != 0) ? rtradv->currhl : ndptr->currhl;
	ndptr->other = rtradv->o;
	ndptr->managed = rtradv->m;
	ndptr->reachable = rtradv->reachable;
	ndptr->retrans = rtradv->retrans;
	ndptr->sendadv = TRUE;

	optptr = rtradv->opts;
	optend = (byte *)rtradv + len;

	while(optptr < optend) {

		switch(*optptr) {

		 case ND_TYPE_SLLAO:
		 	sllao = (struct nd_llao *)optptr;
			if(sllao->len == 0) {
				restore(mask);
				return;
			}
			optptr += 8 * sllao->len;
			break;

		 case ND_TYPE_PIO:
		 	pio = (struct nd_pio *)optptr;
			if(pio->len == 0) {
				restore(mask);
				return;
			}
			optptr += 8 * pio->len;
			if((pio->preflen != 64) ||
			   (pio->a != 1) ||
			   (isipmc(pio->prefix)) ||
			   (isipllu(pio->prefix))) {
			   	break;
			}

			for(i = 1; i < ifptr->if_nipucast; i++) {
				if(!memcmp(ifptr->if_ipucast[i].ipaddr, pio->prefix, 8)) {
					break;
				}
			}
			if(i < ifptr->if_nipucast) {
			}
			else if(ifptr->if_nipucast < IF_NIPUCAST) {
				i = ifptr->if_nipucast;
				memcpy(ifptr->if_ipucast[i].ipaddr, pio->prefix, 8);
				memcpy(&ifptr->if_ipucast[i].ipaddr[8], ifptr->if_eui64, 8);
				ifptr->if_nipucast++;
				kprintf("Added a new IP ucast address\n");
			}
			break;

		 default:
		 	optptr += 8 * (*(optptr+1));
		}
	}

	restore(mask);
}

/*------------------------------------------------------------------------
 * nd_in_nbrsol  -  Handle incoming Neighbor Solicitation message
 *------------------------------------------------------------------------
 */
void	nd_in_nbrsol (
	int32	iface,			/* Interface index		*/
	struct	nd_nbrsol *nbrsol,	/* Neighbor solicitation	*/
	uint32	len,			/* Length of message		*/
	struct	ipinfo *ipdata		/* IP information		*/
	)
{
	struct	ifentry *ifptr;	/* Pointer to interface		*/
	struct	nd_nbradv *nbradv;/* Pointer to nbr adv		*/
	struct	nd_llao *sllao;	/* Pointer to SLLAO		*/
	struct	nd_llao *tllao;	/* Pointer to TLLAO		*/
	struct	nd_aro *aro;	/* Pointer to ARO		*/
	struct	nd_aro *newaro;	/* Pointer to ARO		*/
	byte	*optptr;	/* Pointer to current option	*/
	byte	*optend;	/* Pointer to end of message	*/
	int32	found;		/* Empty NC index		*/
	byte	aro_status;	/* ARO return status		*/
	intmask	mask;		/* Saved interrupt mask		*/
	int32	i;		/* For loop index		*/

	if((len < 20) ||
	   (ipdata->iphl != 255) ||
	   (isipmc(nbrsol->tgtaddr))) {
	   	return;
	}

	ifptr = &if_tab[iface];

	mask = disable();
	for(i = 0; i < ifptr->if_nipucast; i++) {
		if(!memcmp(ifptr->if_ipucast[i].ipaddr, nbrsol->tgtaddr, 16)) {
			break;
		}
	}
	if(i >= ifptr->if_nipucast) {
		restore(mask);
		return;
	}
	restore(mask);

	sllao = (struct nd_llao *)NULL;
	aro = (struct nd_aro *)NULL;

	optptr = nbrsol->opts;
	optend = (byte *)nbrsol + len;

	while(optptr < optend) {

		switch(*optptr) {

		 case ND_TYPE_SLLAO:
		 	sllao = (struct nd_llao *)optptr;
			if(sllao->len == 0) {
				return;
			}
			optptr += 8 * sllao->len;
			break;

		 case ND_TYPE_ARO:
		 	aro = (struct nd_aro *)optptr;
			if(aro->len == 0) {
				return;
			}
			optptr += 8 * aro->len;
			if(isipllu(ipdata->ipsrc)) {
				aro = NULL;
			}
			break;

		 default:
		 	optptr += 8 * (*(optptr+1));
		}
	}

	mask = disable();

	if(aro && (!sllao)) {
		aro = NULL;
	}

	if(aro) {
		found = -1;
		for(i = 0; i < ND_NC_SLOTS; i++) {
			if(nd_ncache[i].state == ND_NCE_FREE) {
				if(found == -1) {
					found = i;
				}
				continue;
			}
			if(!memcmp(nd_ncache[i].eui64, sllao->lladdr, 8)) {
				break;
			}
		}
		if(i < ND_NC_SLOTS) {
			if(memcmp(nd_ncache[i].ipaddr, ipdata->ipsrc, 16)) {
				aro_status = 1;
			}
			else if(aro->reglife == 0) {
				memset((char *)&nd_ncache[i], 0, sizeof(struct nd_nce));
				nd_ncache[i].state = ND_NCE_FREE;
				aro_status = 0;
			}
			else {
				nd_ncache[i].type = ND_NCE_REG;
				nd_ncache[i].reglife = aro->reglife;
				nd_ncache[i].timestamp = clktime;
				aro_status = 0;
			}
		}
		else {
			if(found == -1) {
				for(i = 0; i < ND_NC_SLOTS; i++) {
					if(nd_ncache[i].type == ND_NCE_GRB) {
						found = i;
						break;
					}
				}
			}
			if(found == -1) {
				aro_status = 2;
			}
			else if(aro->reglife != 0) {
				i = found;
				nd_ncache[i].state = ND_NCE_STALE;
				nd_ncache[i].type = ND_NCE_REG;
				memcpy(nd_ncache[i].ipaddr, ipdata->ipsrc, 16);
				memcpy(nd_ncache[i].eui64, sllao->lladdr, 8);
				nd_ncache[i].reglife = aro->reglife;
				nd_ncache[i].timestamp = clktime;
			}
			else {
				aro_status = 0;
			}
		}
	}

	nbradv = (struct nd_nbradv *)nd_msgbuf2;
	nbradv->res1 = 0;
	nbradv->r = 1;
	nbradv->s = 1;
	memcpy(nbradv->tgtaddr, nbrsol->tgtaddr, 16);
	len = sizeof(struct nd_nbradv);

	tllao = (struct nd_llao *)nbradv->opts;
	tllao->type = ND_TYPE_TLLAO;
	tllao->len = 2;
	memcpy(tllao->lladdr, ifptr->if_eui64, 8);
	len += sizeof(struct nd_llao);

	if(aro) {
		newaro = (struct nd_aro *)(tllao + 1);
		newaro->type = ND_TYPE_ARO;
		newaro->len = 2;
		newaro->status = aro_status;
		memcpy(newaro->eui64, aro->eui64, 8);
		len += sizeof(struct nd_aro);
	}

	if(aro_status == 1) {
		if(!sllao) {
			restore(mask);
			return;
		}
		memcpy(ipdata->ipdst, ip_llprefix, 8);
		memcpy(&ipdata->ipdst[8], sllao->lladdr, 8);
	}
	else {
		memcpy(ipdata->ipdst, ipdata->ipsrc, 16);
	}
	memcpy(ipdata->ipsrc, ifptr->if_ipucast[0].ipaddr, 16);
	ipdata->iphl = 255;

	restore(mask);

	icmp_send(iface, ICMP_TYPE_NBRADV, 0, ipdata, (char *)nbradv, len);
}

/*------------------------------------------------------------------------
 * nd_reg_address  -  Register an IP address with a neighbor
 *------------------------------------------------------------------------
 */
int32	nd_reg_address (
	int32	iface,	/* Interface index	*/
	int32	index,	/* IP address index	*/
	byte	nbrip[]	/* Neighbor IP address	*/
	)
{
	struct	ifentry *ifptr;		/* Pointer to interface		*/
	struct	nd_nbrsol *nbrsol;	/* Pointer to Nbr Solicitation	*/
	struct	nd_nbradv *nbradv;	/* Pointer to Nbr advertisement	*/
	struct	nd_llao *sllao;		/* Pointer to SLLAO		*/
	struct	nd_llao *tllao;		/* Pointer to TLLAO		*/
	struct	nd_aro *aro;		/* Pointer to ARO		*/
	struct	ipinfo ipdata;		/* IP information		*/
	int32	slot;			/* ICMP slot			*/
	uint32	nslen;			/* Length of NS message		*/
	uint32	nalen;			/* Length of NA message		*/
	intmask mask;			/* Saved interrupt mask		*/
	byte	*optptr;		/* Pointer to current option	*/
	byte	*optend;		/* Pointer to end of message	*/
	int32	found;			/* Empty index in NC		*/
	int32	tries;			/* No. of tries			*/
	int32	retval;			/* Return value of functions	*/
	int32	i;			/* For loop index		*/

	if((iface < 0) || (iface >= NIFACES)) {
		return SYSERR;
	}

	slot = icmp_register(iface, nbrip, ip_unspec, ICMP_TYPE_NBRADV, 0);
	if(slot == SYSERR) {
		kprintf("nd_reg_address: ICMP reg failed\n");
		return SYSERR;
	}

	mask = disable();
	ifptr = &if_tab[iface];

	if(index >= ifptr->if_nipucast) {
		kprintf("nd_reg_address: index out of bounds\n");
		restore(mask);
		return SYSERR;
	}
	restore(mask);

	nslen = sizeof(struct nd_nbrsol) + sizeof(struct nd_llao) + sizeof(struct nd_aro);
	nbrsol = (struct nd_nbrsol *)getmem(nslen);
	if((int32)nbrsol == SYSERR) {
		return SYSERR;
	}

	nalen = sizeof(struct nd_nbradv) + sizeof(struct nd_llao) + sizeof(struct nd_aro);
	nbradv = (struct nd_nbradv *)getmem(nalen);
	if((int32)nbradv == SYSERR) {
		freemem((char *)nbrsol, nslen);
		return SYSERR;
	}

	memset((char *)nbrsol, 0, nslen);
	nbrsol->res = 0;
	memcpy(nbrsol->tgtaddr, nbrip, 16);

	sllao = (struct nd_llao *)nbrsol->opts;
	sllao->type = ND_TYPE_SLLAO;
	sllao->len = 2;
	memcpy(sllao->lladdr, ifptr->if_eui64, 8);

	aro = (struct nd_aro *)(sllao + 1);
	aro->type = ND_TYPE_ARO;
	aro->len = 2;
	aro->reglife = 1;
	memcpy(aro->eui64, ifptr->if_eui64, 8);

	tries = 0;
	while(tries < 3) {

		memcpy(ipdata.ipsrc, ifptr->if_ipucast[0].ipaddr, 16);
		memcpy(ipdata.ipdst, nbrip, 16);
		ipdata.iphl = 255;

		retval = icmp_send(iface, ICMP_TYPE_NBRSOL, 0, &ipdata, (char *)nbrsol, nslen);
		if(retval == SYSERR) {
			freemem((char *)nbrsol, nslen);
			freemem((char *)nbradv, nalen);
			return SYSERR;
		}

		retval = icmp_recv(slot, (char *)nbradv, nalen, 1000);
		if(retval == SYSERR) {
			freemem((char *)nbrsol, nslen);
			freemem((char *)nbradv, nalen);
			return SYSERR;
		}
		if(retval == TIMEOUT) {
			tries++;
			continue;
		}

		if((retval < 20) ||
		   (ipdata.iphl != 255) ||
		   (memcmp(nbradv->tgtaddr, nbrip, 16)) ||
		   (nbradv->s == 0)) {
		   	tries++;
			continue;
		}

		break;
	}

	mask = disable();

	if(!isipllu(nbrip)) {
		found = -1;
		for(i = 0; i < ND_NC_SLOTS; i++) {
			if(nd_ncache[i].state == ND_NCE_FREE) {
				if(found == -1) {
					found = i;
				}
				continue;
			}
			if(!memcmp(nd_ncache[i].ipaddr, nbrip, 16)) {
				break;
			}
		}
		if(i < ND_NC_SLOTS) {
			found = i;
		}
	}

	tllao = (struct nd_llao *)NULL;
	aro = (struct nd_aro *)NULL;

	optptr = nbradv->opts;
	optend = (byte *)nbradv + retval;

	while(optptr < optend) {

		switch(*optptr) {

		 case ND_TYPE_TLLAO:
		 	tllao = (struct nd_llao *)optptr;
			if(tllao->len == 0) {
				restore(mask);
				freemem((char *)nbrsol, nslen);
				freemem((char *)nbradv, nalen);
				return SYSERR;
			}
			optptr += 8 * tllao->len;
			break;

		 case ND_TYPE_ARO:
		 	aro = (struct nd_aro *)optptr;
			if(aro->len == 0) {
				restore(mask);
				freemem((char *)nbrsol, nslen);
				freemem((char *)nbradv, nalen);
				return SYSERR;
			}
			optptr += 8 * aro->len;
			break;

		 default:
		 	optptr += 8 * (*(optptr+1));
		}
	}

	if(tllao) {
		if(found != -1) {
			i = found;
			if(nd_ncache[i].state == ND_NCE_FREE) {
				nd_ncache[i].state = ND_NCE_REACH;
				nd_ncache[i].type = ND_NCE_GRB;
				memcpy(nd_ncache[i].ipaddr, nbrip, 16);
				memcpy(nd_ncache[i].eui64, tllao->lladdr, 8);
				nd_ncache[i].reglife = 1;
				nd_ncache[i].timestamp = clktime;
			}
			else {
				nd_ncache[i].state = ND_NCE_REACH;
				nd_ncache[i].reglife = 1;
				nd_ncache[i].timestamp = clktime;
			}
		}
	}

	restore(mask);
	freemem((char *)nbrsol, nslen);
	freemem((char *)nbradv, nalen);

	if(aro) {
		return aro->status;
	}
	else {
		return SYSERR;
	}
}
