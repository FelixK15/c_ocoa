#include <stdint.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#define assert_runtime(x)    if(!(x)) raise(SIGTRAP)
#define code_path_invalid()  assert_runtime(0)
#define hook_breakpoint()    asm("nop")
//FK: TODO
#define restrict_modifier __restrict__

#include "../c_ocoa_generator.h"
#include <objc/runtime.h>

char s_AppPath[512] = {};
size_t s_AppPathLength = 0;

FILE* fopen_ios( const char* pPath, const char* pMode )
{
    char fixedPath[512] = {};
    const size_t pathLength = strlen( pPath );
    fixedPath[s_AppPathLength] = '/';
    
    memcpy( fixedPath + 0, s_AppPath, s_AppPathLength );
    memcpy( fixedPath + s_AppPathLength + 1, pPath, pathLength + 1 );

    return fopen( fixedPath, pMode );
}

void extract_ios_app_path( const char* pAppPath )
{
    const size_t appPathLength = strlen( pAppPath );
    const char* pAppPathEnd = pAppPath + appPathLength - 1;
    const char* pAppPathStart = pAppPath;

    while( pAppPathStart != pAppPathEnd )
    {
        if( *pAppPathEnd == '/' )
        {
            break;
        }

        --pAppPathEnd;
    }

    s_AppPathLength = pAppPathEnd - pAppPathStart;
    memcpy( s_AppPath, pAppPath, s_AppPathLength );
}

void ios_print_directory(void)
{
    printf("###########################\n\n%s\n\n###########################\n", s_AppPath );
}

int main(int argc, const char** argv)
{
    extract_ios_app_path( argv[0] );

    c_ocoa_code_generator_parameter parameters = c_ocoa_default_code_generator_parameter();
    evaluate_code_generator_argv_arguments( argc, argv, &parameters );

    parameters.fopen = fopen_ios;

    c_ocoa_code_gen_context context;
    if( !c_ocoa_create_code_gen_context( &context ) )
    {
        return 2;
    }
    
    const int returnValue = c_ocoa_create_classes_api( &parameters, &context );
    if( returnValue != 0 )
    {
        return returnValue;
    }

    ios_print_directory();

    return 0;
}
