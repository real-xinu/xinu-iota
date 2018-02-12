#include <xinu.h>

/* ***************************************************
 * Taylor series that works for 0 < n <= 2 
 * Approximates ln of n and then divides by ln of 10
 * ****************************************************/

double myPow(double x, double exp) {
    int i;
    double res = x;
    for (i = 1; i < exp; i++) {
        res *= x;
    }
    return res;
}

double taylorFrag(double n, double exp) {
    double ans = myPow(n - 1, exp) / exp;
    return ans;
}

double log10(double n) {
    double LN10 = 2.302585092994046; 
    
    if (n == 0) {
        kprintf("log10(0) is undefined\n");
        return 0;
    }
    if (n < 0 || n > 2) {
        kprintf("This log10 calculator can only handle 0 < n <= 2!\n");
        return 0;
    }

    double lnN = 0;
    int i = 0;

    for (i = 1; i < 300; i += 2) {
        lnN += taylorFrag(n, i) - taylorFrag(n, i + 1);
    }

    double res = lnN / LN10;

    return res;
}


