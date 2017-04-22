/* trickle.c */

#include <xinu.h>

struct	_trickle trickle[NTRICKLE];

/*------------------------------------------------------------------------
 * trickle_init  -  Initialize the Trickle system
 *------------------------------------------------------------------------
 */
void	trickle_init (void) {

	int32	i;

	for(i = 0; i < NTRICKLE; i++) {
		trickle[i].state = TRK_STATE_FREE;
	}
}

/*------------------------------------------------------------------------
 * trickle_new  -  Allocate a new trickle timer
 *------------------------------------------------------------------------
 */
int32	trickle_new (
		int32	Imin,
		int32	Imax,
		int32	k
		)
{

	struct	_trickle *tptr;
	int32	i;
	intmask	mask;

	mask = disable();

	for(i = 0; i < NTRICKLE; i++) {
		if(trickle[i].state == TRK_STATE_FREE) {
			break;
		}
	}
	if(i >= NTRICKLE) {
		restore(mask);
		return SYSERR;
	}

	tptr = &trickle[i];

	tptr->Imin = Imin;
	tptr->Imax = Imax;
	tptr->k = k;
	tptr->state = TRK_STATE_USED;

	tptr->tproc = create(trickle_process, 8192, NETPRIO, "Trickle Timer", 1, i);

	restore(mask);

	resume(tptr->tproc);
	return i;
}

/*------------------------------------------------------------------------
 * trickle_consistent  -  Handle consistency
 *------------------------------------------------------------------------
 */
void	trickle_consistent (
		int32	index
		)
{
	intmask	mask;

	mask = disable();
	trickle[index].c++;
	restore(mask);
}

/*------------------------------------------------------------------------
 * trickle_reset  -  Reset the trickle timer
 *------------------------------------------------------------------------
 */
void	trickle_reset (
		int32	index
		)
{
	intmask	mask;

	mask = disable();

	trickle[index].reset = 1;
	unsleep(trickle[index].tproc);
	ready(trickle[index].tproc);

	restore(mask);
}

/*------------------------------------------------------------------------
 * trickle_process  -  Trickle Timer process
 *------------------------------------------------------------------------
 */
process	trickle_process (
		int32	index
		)
{
	struct	_trickle *tptr;
	int32	I;
	int32	t;
	intmask	mask;

	mask = disable();

	tptr = &trickle[index];

	I = tptr->Imin + (rand() % (tptr->Imax - tptr->Imin));

	while(1) {

		tptr->state = TRK_STATE_1;

		tptr->c = 0;

		t = (I/2) + (rand() % (I/2));
		//kprintf("trickle_process: I %d, t %d\n", I, t);

		sleepms(t);

		if(tptr->reset) {
			//kprintf("trickle_process: state1 resetting\n");
			tptr->reset = 0;
			I = tptr->Imin;
			continue;
		}

		if(tptr->c < tptr->k) {
			//kprintf("trickle_process: sending DIO\n");
			//TODO Change this to a generic function
			rpl_send_dio(0, ip_allrplnodesmc);
			//kprintf("trickle_process: returned from DIO\n");
		}

		tptr->state = TRK_STATE_2;

		//kprintf("trickle_process: remaining %d\n", I-t);
		sleepms(I-t);

		if(tptr->reset) {
			//kprintf("trickle_process: state2 resetting\n");
			tptr->reset = 0;
			I = tptr->Imin;
			continue;
		}

		I *= 2;
		if(I > tptr->Imax) {
			I = tptr->Imax;
		}
		//kprintf("trickle_process: interval over, new I %d\n", I);
	}

	restore(mask);
	return OK;
}
