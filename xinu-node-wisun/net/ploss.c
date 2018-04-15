#include <xinu.h>
//#include <fastmath.h> //consider using <tgmath.h>
//#include <sys/time.h>

#define RANDMAX 0x7fffffff
#define DROP 1
#define NODROP 0

//TODO: have to set this up somehow with the topology file
void pathloss_init () {
    //go through each node and set the parameters
    int32 i;
    kprintf("Initializing path loss parameters\n");
    for (i = 0; i < 46; i++) {
        
        //TODO: for future implementations we can change these values
        //depending on the topology file, hard coded for now
        //
        //change this to 140 or so for packet not to drop, 107 for about half to
        //drop
        info.link_info[i].threshold = 0;//change this to negative when we have to subtract from power  
        info.link_info[i].pathloss_ref = 0;//pick a default at d0, castalia did 55
        info.link_info[i].pathloss_exp = 0;//castalia default
        info.link_info[i].dist_ref = 0;//usually defaults 1
        info.link_info[i].sigma = 0;//std dev for gaussian, 6-12

        //look at how the topgen parses the file and decide what to do
        info.link_info[i].distance = 0;//set up from topology file
    }
    kprintf("pathloss_init done\n");

    for(i = 0; i< 20; i++) {
        kprintf("Gauss: %d\n", calc_gauss(0, 4));
    }

}

double simple_sqrt(double input) {
	int32 answer = 1;
	
	while (answer * answer < input) {
		answer++;
	}
	return answer - 1;
}

/*------------------------------------------------------------------------
 * calc_pathloss  -  Function to calculate path loss
 *             -  Returns 1 if packet is dropped 0 otherwise
 *------------------------------------------------------------------------
 */

int32	calc_pathloss (
        int32 receiver
        )
{

    int32 pathloss;//= rand();
    int32 gauss = 0; //calc_gauss(0, info.link_info[receiver].sigma);

    //PL(d)=PL(d0)+10*exponent*log(d/d0)+X
    //TODO: implement a better log function
    pathloss = info.link_info[receiver].pathloss_ref + 10 * info.link_info[receiver].pathloss_exp * log10(info.link_info[receiver].distance / info.link_info[receiver].dist_ref) + gauss;

    //TODO: future improvement, have a transmission power, then minus path loss,
    //if result is less than threshold, then packet isn't received - look below
    kprintf("calculating path loss: %d, threshold: %d\n", pathloss, info.link_info[receiver].threshold);
    if (pathloss > info.link_info[receiver].threshold) 
        //lost too much power, so drop the packet
        return DROP;
    else
        return NODROP;
}

//polar form of Box-Muller transformation
/*====================================================================================*/
int32   calc_gauss (
        double mean,
        double stdDev
        )
{
    double x1, x2, w, y1;
    static double y2;
    static int32 use_last = 0;
    //srand(time(0)); //sets the seed, not sure if this seed should be here or in main

    if (use_last)               // use value from previous call 
    {
        y1 = y2;
        use_last = 0;
    }
    else
    {
        do {
            x1 = (double) rand() / RANDMAX;
            x2 = (double) rand() / RANDMAX;

            x1 = 2.0 * x1 - 1.0;
            x2 = 2.0 * x2 - 1.0;
            w = x1 * x1 + x2 * x2;
        } while ( w >= 1.0 );

        kprintf("calc_gauss: w * 1000:%d\n", (int32)(w * 1000));
        
        w = simple_sqrt( (-2.0 * log10( w ) ) / w );
        y1 = x1 * w;
        y2 = x2 * w;
        use_last = 1;
    }

    return( mean + y1 * stdDev );
}

/*====================================================================================*/

/*==================================================================================*/
/*
 * TODO expand to calculate signal power
 * can use this eventually to calculate power drain

 * float currentSignalReceived = signalMsg->getPower_dBm() - (*it1)->avgPathLoss;
 * if (currentSignalReceived < signalDeliveryThreshold) continue;

 *******************************************************
 * Calculate the distance, beyond which we cannot
 * have connectivity between two nodes. This calculation is
 * based on the maximum TXPower the signalDeliveryThreshold
 * the pathLossExponent, the PLd0. For the random
 * shadowing part we use 3*sigma to account for 99.7%
 * of the cases. We use this value to considerably
 * speed up the filling of the pathLoss array,
 * especially for the mobile case.
 *******************************************************

 * float distanceThreshold = d0 *
 pow(10.0,(maxTxPower - signalDeliveryThreshold - PLd0 + 3 * sigma) /
 (10.0 * pathLossExponent));

 * If the resulting current signal received is not strong enough,
 * to be delivered to the radio module, continue to the next cell.
 *
 *TODO expand to include temporal variation

 ==================================================================================*/
