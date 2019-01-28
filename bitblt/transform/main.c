#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <getopt.h>

#include "gfx_demo_transform.h"

struct ArgVec {
    int                     mmu;
} argvec;

void argvec_init(int argc, char *argv[]);

struct AppCtx {
    volatile int            signo;
    volatile int            go;
} appctx;

void myapp_init(void);
void myapp_fini(void);
void myapp_sighdlr(int sig);
void myapp_regsigint(void);
int myapp_isterm(void);

int main(int argc, char *argv[])
{
    myapp_init();
    argvec_init(argc, argv);
    gfx_setup(argvec.mmu);
        
    do {
        S_DRVBLT_RECT bbox = {80, 240, 40, 200};
        
        gfx_demo_scale(80.0f, 40.0f, &bbox);
        gfx_demo_rotate(160.0f, 120.0f, &bbox);
        gfx_demo_fadein(160.0f, 120.0f, &bbox);
        gfx_demo_fadeout(160.0f, 120.0f, &bbox);
        gfx_demo_flip(160.0f, 120.0f, &bbox);
        gfx_demo_multilayer(&bbox);
    }
    while (0);
    
    gfx_shutdown();
    myapp_fini();
    
    return 0;
}

void argvec_init(int argc, char *argv[])
{
    argvec.mmu = 1;
    
    while (1) {
        int c;
        int option_index = 0;
        
        static struct option long_options[] = {
            {"mmu", required_argument, 0, 'm'},
            {0, 0, 0, 0}
        };
        
        c = getopt_long (argc, argv, "m:", long_options, &option_index);
        
        // Detect the end of the options.
        if (c == -1) {
            break;
        }
        
        switch (c) {
        case 'm':
            argvec.mmu = strtol (optarg, NULL, 0);
            break;
            
        default:
            printf ("Usage: %s [-m mmu]\n", argv[0]);
            exit(1);
        }
    }
}

void myapp_init()
{
    appctx.signo = -1;
    appctx.go = 1;
    myapp_regsigint();
}

void myapp_fini(void)
{
    if (appctx.signo >= 0) {
        extern char *strsignal (int);
        const char *sigstr = strsignal(appctx.signo);
        printf("\nReceived signal: %s(%d)\n",  sigstr ? sigstr : "Unknown", appctx.signo);
        fflush(stdout);
    }
}
       
void myapp_sighdlr(int sig)
{
    appctx.signo = sig;
    appctx.go = 0;
}

void myapp_regsigint(void)
{
    signal(SIGINT, myapp_sighdlr);
    signal(SIGHUP, myapp_sighdlr);
    signal(SIGQUIT, myapp_sighdlr);
    signal(SIGILL, myapp_sighdlr);
    signal(SIGABRT, myapp_sighdlr);
    signal(SIGFPE, myapp_sighdlr);
    signal(SIGKILL, myapp_sighdlr);
    //signal(SIGSEGV, myapp_sighdlr);
    signal(SIGPIPE, myapp_sighdlr);
    signal(SIGTERM, myapp_sighdlr);
}

int myapp_isterm(void)
{
    return ! appctx.go;
}
