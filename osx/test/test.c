#include <stdint.h>
#include <stdio.h>
#include <objc/objc.h>
#include <objc/runtime.h>
#include <objc/message.h>

#include "../nsapplication.c"

int main(int argc, char** argv)
{
    nsapplication_t sharedApplication = nsapplication_sharedApplication();
    nsapplication_run( sharedApplication );

    return 0;
}