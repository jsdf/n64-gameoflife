

#ifndef _MAIN_H_
#define _MAIN_H_

// #define RAND(x) (guRandom() % x) /* random number between 0 to x */
//
#ifndef CLAMP
#define CLAMP(x, low, high) \
  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#endif
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

extern NUContData contdata[1];  // input data for one controller

#endif /* _MAIN_H_ */
