
// Include this *only* in .cc files. Then define DEBUG in such .cc file
// to enable debugging. Use DEBUGLN to wrap around statements which should
// only execute when debugging is active.

#ifdef DEBUG
#pragma message("DEBUG enabled for " DEBUG)
static bool debug_stop = false;
#define DEBUGLN(ln) if(!debug_stop) { ln; }
#define DEBUGSTOP(fl) { debug_stop = fl; }
#else
#define DEBUGLN(ln)
#define DEBUGSTOP(fl)
#endif
