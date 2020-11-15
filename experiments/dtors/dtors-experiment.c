#include <stdio.h>
#include <stdlib.h>

static void start(void) __attribute__ ((constructor));
static void stop(void) __attribute__ ((destructor));

int
main(int argc, char *argv[])
{
        printf("start == %p\n", start);
        printf("stop == %p\n", stop);

        exit(EXIT_SUCCESS);
}

void
start(void)
{
        printf("hello world!\n");
}

void
stop(void)
{
        printf("goodbye world!\n");
}
