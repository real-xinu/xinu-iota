/*  main.c  - main */

#include <xinu.h>
#define RANDMAX 2147483647

double simple_sqrt(double input) {
	if (input == 0) return 0;
    int32 answer = 1;
	
	while (answer * answer < input) {
		answer++;
	}

    if (answer * answer == input)
        return answer;
    else
	    return answer - 1;
}


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

process	main(void)
{
/*    uint32 time = 0;
    kprintf("gettime\n");
    gettime(&time);
    srand(time);//sets seed so it's random
    kprintf("seed set at: %d\n", time);*/
    
    uint32 i;
    for (i = 0; i < 20; i++) {
        kprintf("Gauss: %d\n", calc_gauss(0, 4));
    }
    
    /* Obtain an IP address */

	//netstart();

	/* Run the Xinu shell */


	//resume(create(shell, 8192, 50, "shell", 1, CONSOLE));

	/* Wait for shell to exit and recreate it */

        kprintf("\n...creating testbed server process\n");
	recvclr();
	init_topo();
	resume(create(wsserver, 8192, NETPRIO-2, "wsserver", 1, CONSOLE));
	//init_topo();
	/*while (TRUE) {
		receive();
		sleepms(200);
		kprintf("\n\nMain process recreating shell\n\n");
		resume(create(shell, 4096, 20, "shell", 1, CONSOLE));
	}*/
	return OK;
    
}
