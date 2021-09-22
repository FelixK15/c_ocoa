#include <stdint.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#define RuntimeAssert(x)    if(!(x)) raise(SIGTRAP)
#define InvalidCodePath()   RuntimeAssert(0)
#define BreakpointHook()    asm("nop")
//FK: TODO
#define restrict_modifier __restrict__
#include "../shimmer_api_generator.h"

#include <objc/runtime.h>

uint8_t createCSourceCodeForClass( ObjCTypeDict* pTypeDict, StringAllocator* pStringAllocator, Class pClass, const char* pClassName )
{
    const size_t classNameLength = getCStringLengthInclNullTerminator( pClassName );

    //FK: +2 for file extension (.c/.h)
    char* pHeaderFileName = (char*)alloca( classNameLength + 2);
    char* pSourceFileName = (char*)alloca( classNameLength + 2);

    char* pLowerClassName = allocateLowerCStringCopy( pClassName, classNameLength );
    sprintf( pHeaderFileName, "%s.h", pLowerClassName );
    sprintf( pSourceFileName, "%s.c", pLowerClassName );

    FILE* pSourceFileHandle = fopen( pSourceFileName, "w" );
    FILE* pHeaderFileHandle = fopen( pHeaderFileName, "w" );

    if( pSourceFileHandle == NULL )
    {
        printf_stderr( "[error] Couldn't open '%s' for writing.\n", pSourceFileName );
        return 0;
    }

    if( pHeaderFileName == NULL )
    {
        printf_stderr( "[error] Couldn't open '%s' for writing.\n", pHeaderFileName );
        return 0;
    }

    writeCHeaderPrefix( pHeaderFileHandle, pClassName, classNameLength );
    writeCSourcePrefix( pSourceFileHandle, pHeaderFileName );
    
    uint32_t methodCount = 0u;
    Method* ppMethods = class_copyMethodList( pClass, &methodCount );
    if( methodCount == 0u )
    {
        printf_stderr("[error] Class '%s' doesn't have any methods.\n", pClassName );
        return 0;
    }

    ParseResult parseResult = {};
    for( uint32_t methodIndex = 0u; methodIndex < methodCount; ++methodIndex )
    {
        Method pMethod = ppMethods[ methodIndex ];

        const size_t stringAllocatorCapacity = (size_t)getRemainingStringAllocatorCapacity( pStringAllocator );
        char* pReturnType = getCurrentStringAllocatorBase( pStringAllocator );
        
        //FK: There seems to be no way to know the return type length beforehand...?
        //    NOTE: Potential memory trampler here since we don't know beforehand
        //          if the string allocator can satisfy this allocation or not
        //          Alternativaly, we could use 'method_copyReturnType()' but that
        //          calls 'malloc()' each time internally...
        method_getReturnType( pMethod, pReturnType, stringAllocatorCapacity );

        const int32_t returnTypeLength = getCStringLengthExclNullTerminator( pReturnType );
        decrementStringAllocatorCapacity( pStringAllocator, returnTypeLength + 1 );

        SEL pMethodSelector = method_getName( pMethod );
        if( pMethodSelector == NULL )
        {
            printf_stderr("[error] empty method selector for %d. method of class '%s' - Skipping method.\n", methodIndex, pClassName );
            continue;
        }

        const char* pSelectorName = sel_getName( pMethodSelector );
        if( pSelectorName == NULL )
        {
            printf_stderr("[error] empty method selector name for %d. method of class '%s' - Skipping method.\n", methodIndex, pClassName );
            continue;
        }

        const int32_t selectorNameLength = getCStringLengthExclNullTerminator( pSelectorName );
        char* pMethodName = allocateCStringCopyWithAllocator( pStringAllocator, pSelectorName, selectorNameLength );
       
        const uint32_t argumentCount = method_getNumberOfArguments( pMethod );
        char* pArgumentTypes = getCurrentStringAllocatorBase( pStringAllocator );
        char* pCurrentArgumentType = pArgumentTypes;
        for( uint32_t argumentIndex = 0u; argumentIndex < argumentCount; ++argumentIndex )
        {
            const uint32_t stringAllocatorCapacity = getRemainingStringAllocatorCapacity( pStringAllocator );
            method_getArgumentType( pMethod, argumentIndex, pCurrentArgumentType, stringAllocatorCapacity );

            const int32_t argumentTypeLength = getCStringLengthExclNullTerminator( pCurrentArgumentType );

            const uint8_t addSpaceAtEnd = ( argumentIndex + 1 != argumentCount );
            if( addSpaceAtEnd )
            {
                pCurrentArgumentType[ argumentTypeLength ] = ' ';
            }

            //FK: +1 for space if not at end of iteration and for null terminator if last iteration
            decrementStringAllocatorCapacity( pStringAllocator, argumentTypeLength + 1);
            pCurrentArgumentType = pCurrentArgumentType + argumentTypeLength + 1;
        }

        const int32_t argumentCharacterWritten = castSizeToInt32( pCurrentArgumentType - pArgumentTypes );
        pArgumentTypes[ argumentCharacterWritten + 1] = 0;

        ParseResult parseResult;
        parseResult.pArguments          = pArgumentTypes;
        parseResult.argumentLength      = argumentCharacterWritten;
        parseResult.pFunctionName       = pMethodName;
        parseResult.functionNameLength  = selectorNameLength;
        parseResult.pReturnType         = pReturnType;
        parseResult.returnTypeLength    = returnTypeLength;

        CFunctionDefinition functionDefinition;
        const ConvertResult convertResult = convertParseResultToFunctionDefinition( pStringAllocator, &functionDefinition, pTypeDict, &parseResult, pClassName, classNameLength );
        switch( convertResult )
        {
            case ConvertResult_UnknownArgumentType:
                printf_stderr("[error] Skipping function '%s' because of unknown argument type.\n", parseResult.pFunctionName );
                break;

            case ConvertResult_UnknownReturnType:
                printf_stderr("[error] Skipping function '%s' because of unknown argument type.\n", parseResult.pFunctionName );
                break;

            case ConvertResult_InvalidFunctionName:
                //FK: Add log here?
                break;

            case ConvertResult_Success:
                writeCFunctionDeclaration( pHeaderFileHandle, &functionDefinition );
                writeCFunctionImplementation( pSourceFileHandle, &functionDefinition, pClassName, classNameLength );
                break;
        }
        resetStringAllocator( pStringAllocator );
    }

    free( ppMethods );

    writeCHeaderSuffix( pHeaderFileHandle );
    
    fclose( pHeaderFileHandle );
    fclose( pSourceFileHandle );

    return 1u;
};

int main(int argc, const char** argv)
{
    const size_t dictSizeInBytes = 1024*1024; //FK: 1 MiB
    ObjCTypeDict* pDict = createObjectiveCTypeDictionary( dictSizeInBytes );
    if( pDict == NULL )
    {
        printf_stderr( "Out of memory while trying to create objective c type dictionary of size %.3fkB.", (float)dictSizeInBytes/1024.f );
        return 2;
    }

    const size_t stringAllocatorSizeInBytes = 1024*1024; //FK: 1 MiB, quite a lot for a string allocator but better be safe than sorry
    StringAllocator stringAllocator;
    if( !createStringAllocator( &stringAllocator, stringAllocatorSizeInBytes ) )
    {
        printf_stderr( "Out of memory while trying to create string allocator of size %.3fKiB.", (float)stringAllocatorSizeInBytes/1024.f );
        return 2;
    }

    const int totalClassCount = objc_getClassList( NULL, 0 );
    const size_t totalClassBufferSizeInBytes = totalClassCount * sizeof( Class );
    Class* ppClasses = ( Class* )malloc( totalClassBufferSizeInBytes );
    if( ppClasses == NULL )
    {
        printf_stderr( "Out of memory - Couldn't allocate %.3fkB.", (float)totalClassBufferSizeInBytes/1024.f );
        return 2;
    }
    
    objc_getClassList(ppClasses, totalClassCount);
    
    const uint32_t maxClasses = totalClassCount;
    for( uint32_t classIndex = 0u; classIndex < maxClasses; ++classIndex )
    {
        Class pClass = ppClasses[ classIndex ];
        const char* pClassName = class_getName( pClass );
        if( areCStringsEqual(pClassName, "NSApplication") )
        {
            createCSourceCodeForClass( pDict, &stringAllocator, pClass, pClassName );
        }

        printf( "%s\n", pClassName );
    }
    return 0;
}