/* nd.c */

#include <xinu.h>

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
	ndptr->reachable = 30000;
	ndptr->retrans = 1000;

	for(i = 0; i < ND_NC_SLOTS; i++) {
		memset((char *)&ifptr->if_ncache[i], 0, sizeof(struct nd_nce));
		ifptr->if_ncache[i].state = ND_NCE_FREE;
		ifptr->if_ncache[i].waitsem = semcreate(1);
		if(ifptr->if_ncache[i].waitsem == SYSERR) {
			panic("nd_init: cannot create semaphore");
		}
	}

	restore(mask);

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

	msgbuf = getbuf(netbufpool);
	while(TRUE) {
		//msgbuf = nd_msgbuf1;
		//msgbuf = getbuf(netbufpool);
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
			nd_in_nbrsol(iface, (struct nd_nbrsol *)msgbuf, buflen, &ipdata);
		}
		else if(retval == slots[3]) {
			nd_in_nbradv(iface, (struct nd_nbradv *)msgbuf, buflen, &ipdata);
		}
		else {
			panic("nd_in: Unknown message type");
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
	memcpy(sllao->lladdr, ifptr->if_hwucast, 8);
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
				memcpy(&ifptr->if_ipucast[i].ipaddr[8], ifptr->if_hwucast, 8);
				ifptr->if_nipucast++;
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
	struct	nd_nce *ncptr;	/* Pointer to nbr cache entry	*/
	struct	nd_nbradv *nbradv;/* Pointer to nbr adv		*/
	struct	nd_llao *sllao;	/* Pointer to SLLAO		*/
	struct	nd_llao *tllao;	/* Pointer to TLLAO		*/
	struct	nd_aro *aro;	/* Pointer to ARO		*/
	struct	nd_aro *newaro;	/* Pointer to ARO		*/
	byte	*optptr;	/* Pointer to current option	*/
	byte	*optend;	/* Pointer to end of message	*/
	int32	found;		/* Empty NC index		*/
	byte	aro_status;	/* ARO return status		*/
	int32	ncslot;		/* Empty nbr cache slot		*/
	intmask	mask;		/* Saved interrupt mask		*/
	int32	i;		/* For loop index		*/

	if((len < 20) ||
	   (ipdata->iphl != 255) ||
	   (isipmc(nbrsol->tgtaddr))) {
	   	return;
	}

	if(isipunspec(ipdata->ipsrc)) {
		if(memcmp(ipdata->ipdst, ip_solmc, 13)) {
			return;
		}
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
		kprintf("nd_in_nbrsol: ARO without SLLAO\n");
		aro = NULL;
	}

	if(aro) {
		kprintf("nd_in_nbrsol: ARO included\n");
		found = -1;
		for(i = 0; i < ND_NC_SLOTS; i++) {
			if(ifptr->if_ncache[i].state == ND_NCE_FREE) {
				if(found == -1) {
					found = i;
				}
				continue;
			}
			if(!memcmp(ifptr->if_ncache[i].hwucast, sllao->lladdr, 8)) {
				break;
			}
		}
		if(i < ND_NC_SLOTS) {
			if(memcmp(ifptr->if_ncache[i].ipaddr, ipdata->ipsrc, 16)) {
				kprintf("nd_in_nbrsol: Duplicate address\n");
				aro_status = 1;
			}
			else if(aro->reglife == 0) {
				kprintf("nd_in_nbrsol: removing entry\n");
				memset((char *)&ifptr->if_ncache[i], 0, sizeof(struct nd_nce));
				ifptr->if_ncache[i].state = ND_NCE_FREE;
				aro_status = 0;
			}
			else {
				kprintf("nd_in_nbrsol: updating entry\n");
				ifptr->if_ncache[i].type = ND_NCE_REG;
				ifptr->if_ncache[i].reglife = aro->reglife;
				//ifptr->if_ncache[i].timestamp = clktime;
				aro_status = 0;
			}
		}
		else {
			if(found == -1) {
				for(i = 0; i < ND_NC_SLOTS; i++) {
					if(ifptr->if_ncache[i].type == ND_NCE_GRB) {
						found = i;
						break;
					}
				}
			}
			if(found == -1) {
				kprintf("nd_in_nbrsol: nbr cache full\n");
				aro_status = 2;
			}
			else if(aro->reglife != 0) {
				kprintf("nd_in_nbrsol: adding a new entry\n");
				i = found;
				ifptr->if_ncache[i].state = ND_NCE_STALE;
				ifptr->if_ncache[i].type = ND_NCE_REG;
				memcpy(ifptr->if_ncache[i].ipaddr, ipdata->ipsrc, 16);
				memcpy(ifptr->if_ncache[i].hwucast, sllao->lladdr, 8);
				ifptr->if_ncache[i].reglife = aro->reglife;
				//ifptr->if_ncache[i].timestamp = clktime;
				aro_status = 0;
			}
			else {
				aro_status = 0;
			}
		}
	}
	else if((!isipunspec(ipdata->ipsrc)) && (sllao)) { /* No ARO, normal ND */
		ncslot = -1;
		for(i = 0; i < ND_NC_SLOTS; i++) {
			if(ifptr->if_ncache[i].state == ND_NCE_FREE) {
				ncslot = (ncslot == -1) ? i : ncslot;
				continue;
			}
			if(!memcmp(ifptr->if_ncache[i].ipaddr, ipdata->ipsrc, 16)) {
				break;
			}
		}
		if(i >= ND_NC_SLOTS) {
			if(ncslot != -1) {
				ncptr = &ifptr->if_ncache[ncslot];
				memcpy(ncptr->ipaddr, ipdata->ipsrc, 16);
				memcpy(ncptr->hwucast, sllao->lladdr, ifptr->if_halen);
				ncptr->state = ND_NCE_STALE;
				ncptr->type = ND_NCE_GRB;
				kprintf("nd_in_nbrsol: added "); ip_print(ncptr->ipaddr);
				kprintf(" to the nbr cache\n");
			}
		}
		else {
			ncptr = &ifptr->if_ncache[i];
			if(memcmp(ncptr->hwucast, sllao->lladdr, ifptr->if_halen)) {
				memcpy(ncptr->hwucast, sllao->lladdr, ifptr->if_halen);
				ncptr->state = ND_NCE_STALE;
			}
		}
	}

	nbradv = (struct nd_nbradv *)getbuf(netbufpool);
	nbradv->res1 = 0;
	nbradv->r = 1;
	nbradv->s = 1;
	memcpy(nbradv->tgtaddr, nbrsol->tgtaddr, 16);
	len = sizeof(struct nd_nbradv);

	tllao = (struct nd_llao *)nbradv->opts;
	tllao->type = ND_TYPE_TLLAO;
	tllao->len = (ifptr->if_halen == 6) ? 1 : 2;
	memcpy(tllao->lladdr, ifptr->if_hwucast, ifptr->if_halen);
	len += (tllao->len)*8;

	if(aro) {
		newaro = (struct nd_aro *)(tllao + 1);
		newaro->type = ND_TYPE_ARO;
		newaro->len = 2;
		newaro->status = aro_status;
		memcpy(newaro->eui64, aro->eui64, 8);
		len += sizeof(struct nd_aro);
	}

	if((aro) && (aro_status == 1)) {
		if(!sllao) {
			freebuf((char *)nbradv);
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
	freebuf((char *)nbradv);
}

/*------------------------------------------------------------------------
 * nd_in_nbradv  -  Handle incoming Neighbor Advertisements
 *------------------------------------------------------------------------
 */
void	nd_in_nbradv (
	int32	iface,
	struct	nd_nbradv *nbradv,
	int32	len,
	struct	ipinfo *ipdata
	)
{
	struct	ifentry *ifptr;
	struct	nd_nce *ncptr;
	struct	nd_llao *tllao;
	byte	*optptr, *optend;
	int32	optlen;
	intmask	mask;
	int32	i;

	if(nbradv == NULL) {
		return;
	}

	if((iface < 0) || (iface >= NIFACES)) {
		return;
	}
	ifptr = &if_tab[iface];

	if((ipdata->iphl != 255) ||
	   (len < 20) ||
	   (isipmc(nbradv->tgtaddr)) ||
	   ((isipmc(ipdata->ipdst)) && (nbradv->s))) {
		return;
	}

	mask = disable();
	for(i = 0; i < ND_NC_SLOTS; i++) {
		ncptr = &ifptr->if_ncache[i];
		if(ncptr->state == ND_NCE_FREE) {
			continue;
		}
		if(!memcmp(ncptr->ipaddr, nbradv->tgtaddr, 16)) {
			break;
		}
	}
	if(i >= ND_NC_SLOTS) {
		restore(mask);
		return;
	}

	optptr = nbradv->opts;
	optend = (byte *)(optptr + len - sizeof(struct nd_nbradv));
	tllao = NULL;
	while(optptr < optend) {

		optlen = (*(optptr+1))*8;
		if(optlen == 0) {
			restore(mask);
			return;
		}

		switch(*optptr) {

		 case ND_TYPE_TLLAO:
			 tllao = (struct nd_llao *)optptr;
			 optptr += optlen;
			 break;

		 default:
			 optptr += optlen;
			 break;
		}
	}

	if(ncptr->state == ND_NCE_INCOM) {
		if(!tllao) {
			restore(mask);
			return;
		}
		memcpy(ncptr->hwucast, tllao->lladdr, ifptr->if_halen);
		if(nbradv->s) {
			ncptr->state = ND_NCE_REACH;
			ncptr->ttl = ifptr->if_ndData.reachable;
		}
		else {
			ncptr->state = ND_NCE_STALE;
		}
		send(ncptr->pid, OK);
		restore(mask);
		return;
	}

	if((nbradv->o == 0) && (tllao) && (memcmp(ncptr->hwucast, tllao->lladdr, ifptr->if_halen))) {
		if(ncptr->state == ND_NCE_REACH) {
			ncptr->state = ND_NCE_STALE;
		}
		else {
			restore(mask);
			return;
		}
	}

	if((nbradv->o == 1) ||
	   ((tllao) && (!memcmp(ncptr->hwucast, tllao->lladdr, ifptr->if_halen))) ||
	   (tllao == NULL)) {
		if(tllao) {
			if(memcmp(ncptr->hwucast, tllao->lladdr, ifptr->if_halen)) {
				memcpy(ncptr->hwucast, tllao->lladdr, ifptr->if_halen);
				if(!nbradv->s) {
					ncptr->state = ND_NCE_STALE;
				}
			}
		}
		if(nbradv->s) {
			kprintf("neighbor "); ip_print(ncptr->ipaddr);
			kprintf(" moved to REACHABLE\n");
			ncptr->state = ND_NCE_REACH;
			ncptr->ttl = ifptr->if_ndData.reachable;
		}
	}

	restore(mask);
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
	memcpy(sllao->lladdr, ifptr->if_hwucast, 8);

	aro = (struct nd_aro *)(sllao + 1);
	aro->type = ND_TYPE_ARO;
	aro->len = 2;
	aro->reglife = 1;
	memcpy(aro->eui64, ifptr->if_hwucast, 8);

	tries = 0;
	while(tries < 3) {

		memcpy(ipdata.ipsrc, ifptr->if_ipucast[index].ipaddr, 16);
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
			if(ifptr->if_ncache[i].state == ND_NCE_FREE) {
				if(found == -1) {
					found = i;
				}
				continue;
			}
			if(!memcmp(ifptr->if_ncache[i].ipaddr, nbrip, 16)) {
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
			if(ifptr->if_ncache[i].state == ND_NCE_FREE) {
				ifptr->if_ncache[i].state = ND_NCE_REACH;
				ifptr->if_ncache[i].type = ND_NCE_GRB;
				memcpy(ifptr->if_ncache[i].ipaddr, nbrip, 16);
				memcpy(ifptr->if_ncache[i].hwucast, tllao->lladdr, 8);
				ifptr->if_ncache[i].reglife = 1;
				//ifptr->if_ncache[i].timestamp = clktime;
			}
			else {
				ifptr->if_ncache[i].state = ND_NCE_REACH;
				ifptr->if_ncache[i].reglife = 1;
				//ifptr->if_ncache[i].timestamp = clktime;
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

/*------------------------------------------------------------------------
 * nd_resolve_eth  -  Resolve IPv6 address into Ethernet address
 *------------------------------------------------------------------------
 */
int32	nd_resolve_eth (
	int32	iface,
	byte	*ip,
	byte	*hwucast
	)
{
	struct	ifentry *ifptr;	/* Pointer to interface	*/
	struct	nd_nce *ncptr;
	struct	ipinfo ipdata1, ipdata2;
	struct	nd_nbrsol *nbrsol;
	struct	nd_nbradv *nbradv;
	struct	nd_llao *sllao, *tllao;
	intmask	mask;		/* Saved interrupt mask	*/
	//int32	slot;		/* Slot in ICMP table	*/
	int32	ncslot;		/* Slot in Nbr cache	*/
	byte	*optptr, *optend, optlen;
	bool8	opterr = FALSE;
	int32	retval, tries, msglen;
	int32	i;		/* For loop index	*/

	if(ip == NULL || hwucast == NULL) {
		return SYSERR;
	}

	if((iface < 0) || (iface >= NIFACES)) {
		return SYSERR;
	}

	mask = disable();
	ifptr = &if_tab[iface];

	if(ifptr->if_state == IF_DOWN || ifptr->if_type != IF_TYPE_ETH) {
		restore(mask);
		return SYSERR;
	}

	ncslot = -1;
	for(i = 0; i < ND_NC_SLOTS;) {
		if(ifptr->if_ncache[i].state == ND_NCE_FREE) {
			ncslot = ncslot == -1 ? i : ncslot;
			i++;
			continue;
		}
		if(!memcmp(ip, ifptr->if_ncache[i].ipaddr, 16)) {
			if(ifptr->if_ncache[i].state == ND_NCE_INCOM) {
				wait(ifptr->if_ncache[i].waitsem);
				i = 0;
				continue;
			}
			else {
				break;
			}
		}
		i++;
	}
	if(i < ND_NC_SLOTS) {
		ncptr = &ifptr->if_ncache[i];
		memcpy(hwucast, ifptr->if_ncache[i].hwucast, 6);
		if(ifptr->if_ncache[i].state == ND_NCE_STALE) {
			ncptr->state = ND_NCE_DELAY;
			ncptr->ttl = 5000;
		}
		restore(mask);
		return OK;
	}

	if(ncslot == -1) {
		restore(mask);
		return SYSERR;
	}

	ifptr->if_ncache[ncslot].state = ND_NCE_INCOM;
	memcpy(ifptr->if_ncache[ncslot].ipaddr, ip, 16);
	ifptr->if_ncache[ncslot].pid = getpid();
	semreset(ifptr->if_ncache[ncslot].waitsem, 1);
	wait(ifptr->if_ncache[ncslot].waitsem);

	memcpy(ipdata1.ipdst, ip_solmc, 13);
	memcpy(&ipdata1.ipdst[13], &ip[13], 3);
	memcpy(ipdata1.ipsrc, ifptr->if_ipucast[0].ipaddr, 16);
	ipdata1.iphl = 255;

	nbrsol = (struct nd_nbrsol *)getbuf(netbufpool);
	nbrsol->res = 0;
	memcpy(nbrsol->tgtaddr, ip, 16);
	msglen = sizeof(struct nd_nbrsol);

	sllao = (struct nd_llao *)nbrsol->opts;
	sllao->type = ND_TYPE_SLLAO;
	sllao->len = 1;
	memcpy(sllao->lladdr, ifptr->if_hwucast, 6);
	msglen += 8;

	tries = 0;
	while(tries < 3) {

		tries++;

		retval = icmp_send(iface, ICMP_TYPE_NBRSOL, 0, &ipdata1, (char *)nbrsol, msglen);
		if(retval == SYSERR) {
			signal(ifptr->if_ncache[ncslot].waitsem);
			freebuf((char *)nbrsol);
			restore(mask);
			return SYSERR;
		}

		retval = recvtime(ifptr->if_ndData.retrans);
		if(retval == TIMEOUT) {
			continue;
		}
		else {
			break;
		}
	}

	if(ifptr->if_ncache[ncslot].state == ND_NCE_REACH) {
		kprintf("nd_resolve_eth: "); ip_print(ifptr->if_ncache[ncslot].ipaddr);
		kprintf(" moved to REACHABLE\n");
		memcpy(hwucast, ifptr->if_ncache[ncslot].hwucast, 6);
		signal(ifptr->if_ncache[ncslot].waitsem);
		freebuf((char *)nbrsol);
		restore(mask);
		return OK;
	}

	ifptr->if_ncache[ncslot].state = ND_NCE_FREE;
	signal(ifptr->if_ncache[ncslot].waitsem);
	freebuf((char *)nbrsol);
	restore(mask);
	return SYSERR;
}

/*------------------------------------------------------------------------
 * nd_nud  -  Perform Neighbor Unreachability Detection
 *------------------------------------------------------------------------
 */
process	nd_nud (
	int32	iface,	/* Interface index	*/
	int32	slot	/* Slot in nbr cache	*/
	)
{
	struct	ifentry *ifptr;
	struct	nd_nce *ncptr;
	struct	nd_nbrsol *nbrsol;
	struct	ipinfo ipdata;
	int32	tries;
	intmask	mask;

	if((iface < 0) || (iface >= NIFACES)) {
		return SYSERR;
	}

	if((slot < 0) || (slot >= ND_NC_SLOTS)) {
		return SYSERR;
	}

	mask = disable();
	ifptr = &if_tab[iface];
	ncptr = &ifptr->if_ncache[slot];

	if(ncptr->state != ND_NCE_PROBE) {
		restore(mask);
		return SYSERR;
	}

	nbrsol = (struct nd_nbrsol *)getbuf(netbufpool);
	tries = 0;
	while(tries < 3) {

		tries++;

		nbrsol->res = 0;
		memcpy(nbrsol->tgtaddr, ncptr->ipaddr, 16);

		memcpy(ipdata.ipdst, ncptr->ipaddr, 16);
		memcpy(ipdata.ipsrc, ifptr->if_ipucast[0].ipaddr, 16);
		ipdata.iphl = 255;

		icmp_send(iface, ICMP_TYPE_NBRSOL, 0, &ipdata, (char *)nbrsol, sizeof(struct nd_nbrsol));

		sleepms(ifptr->if_ndData.retrans);

		if(ncptr->state == ND_NCE_REACH) {
			break;
		}
	}

	if(ncptr->state != ND_NCE_REACH) {
		memset((char *)ncptr, 0, sizeof(struct nd_nce));
		ncptr->state = ND_NCE_FREE;
	}

	freebuf((char *)nbrsol);
	restore(mask);
	return OK;
}

/*------------------------------------------------------------------------
 * nd_timer  -  Periodic ND process to cleanup neighbor cache
 *------------------------------------------------------------------------
 */
process	nd_timer (void) {

	struct	ifentry *ifptr;
	struct	nd_nce *ncptr;
	int32	iface;
	intmask	mask;
	int32	i;

	while(TRUE) {

		mask = disable();
		for(iface = 0; iface < NIFACES; iface++) {

			ifptr = &if_tab[iface];
			if(ifptr->if_state == IF_DOWN) {
				continue;
			}

			for(i = 0; i < ND_NC_SLOTS; i++) {
				ncptr = &ifptr->if_ncache[i];
				if(ncptr->state == ND_NCE_FREE) {
					continue;
				}

				if(ncptr->ttl > 0) {
					if(--ncptr->ttl > 0) {
						continue;
					}
				}

				if(ncptr->state == ND_NCE_REACH) {
					kprintf("neighbor "); ip_print(ncptr->ipaddr);
					kprintf(" moved to STALE\n");
					//ncptr->ttl = 100;
					ncptr->state = ND_NCE_STALE;
					continue;
				}
				if(ncptr->state == ND_NCE_DELAY) {
					kprintf("neighbor "); ip_print(ncptr->ipaddr);
					kprintf(" moved to PROBE\n");
					ncptr->state = ND_NCE_PROBE;
					resume(create(nd_nud, NETSTK, NETPRIO, "nd_nud", 2, iface, i));
					/*
					resume(create(nd_resolve_eth,
							NETSTK,
							NETPRIO,
							"nd_resolve_eth",
							3,
							iface,
							ncptr->ipaddr,
							ncptr->hwucast));
					*/
					continue;
				}
			}
		}
		restore(mask);
		sleepms(1);
	}

	return OK;
}
