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
#include "../api_generator.h"

#include <objc/runtime.h>

typedef struct 
{
    Method* ppClassMethods;
    Method* ppInstanceMethods;

    uint32_t classMethodCount;
    uint32_t instanceMethodCount;
} ClassMethodCollection;

typedef struct 
{
    ObjCFunctionCollection* pFunctionCollection;
    ObjCTypeDict*           pTypeDict;
    const ObjCClassName*    pClassName;
    StringAllocator*        pStringAllocator;
    FILE*                   pHeaderFileHandle;
    FILE*                   pSourceFileHandle;
} SourceCodeGeneratorInput;

bool addClassMethodsToMethodCollection( ClassMethodCollection* pMethodCollection, Method* pMethods, uint32_t methodCount )
{
    if( methodCount == 0u )
    {
        return true;
    }

    const uint32_t oldMethodCount = pMethodCollection->classMethodCount;
    const uint32_t newMethodCount = methodCount + oldMethodCount;
    Method* pNewMethodArray = (Method*)malloc( newMethodCount * sizeof( Method ) );
    if( pNewMethodArray == NULL )
    {
        return false;
    }

    copyMemoryNonOverlapping( pNewMethodArray, pMethodCollection->ppClassMethods, sizeof(Method) * oldMethodCount );
    copyMemoryNonOverlapping( pNewMethodArray + oldMethodCount, pMethods, sizeof(Method) * methodCount );

    pMethodCollection->ppClassMethods = pNewMethodArray;
    pMethodCollection->classMethodCount = newMethodCount;

    return true;
}

boolean8_t addInstanceMethodsToMethodCollection( ClassMethodCollection* pMethodCollection, Method* pMethods, uint32_t methodCount )
{
    if( methodCount == 0u )
    {
        return 1;
    }

    const uint32_t oldMethodCount = pMethodCollection->instanceMethodCount;
    const uint32_t newMethodCount = methodCount + oldMethodCount;
    Method* pNewMethodArray = (Method*)malloc( newMethodCount * sizeof( Method ) );
    if( pNewMethodArray == NULL )
    {
        return 0;
    }

    copyMemoryNonOverlapping( pNewMethodArray, pMethodCollection->ppInstanceMethods, sizeof(Method) * oldMethodCount );
    copyMemoryNonOverlapping( pNewMethodArray + oldMethodCount, pMethods, sizeof(Method) * methodCount );

    free( pMethodCollection->ppInstanceMethods );

    pMethodCollection->ppInstanceMethods = pNewMethodArray;
    pMethodCollection->instanceMethodCount = newMethodCount;

    return 1;
}

boolean8_t collectMethodsFromClass( ClassMethodCollection* pMethodCollection, Class pClass, const char* pClassName )
{
    Class pMetaClass = object_getClass( (id)pClass );
    uint32_t instanceMethodCount    = 0u;
    uint32_t classMethodCount       = 0u;

    Method* pInstanceMethods    = class_copyMethodList( pClass, &instanceMethodCount );
    Method* pClassMethods       = class_copyMethodList( pMetaClass, &classMethodCount );

    const boolean8_t addedInstanceMethods = addInstanceMethodsToMethodCollection( pMethodCollection, pInstanceMethods, instanceMethodCount );
    const boolean8_t addedClassMethods    = addClassMethodsToMethodCollection( pMethodCollection, pClassMethods, classMethodCount );

    free( pInstanceMethods );
    free( pClassMethods );

    if( !addedInstanceMethods )
    {
        printf("[error] out of memory while trying to add %u instance methods from class '%s'.\n", instanceMethodCount, pClassName );
        return 0;
    }

    if( !addedClassMethods )
    {
        printf("[error] out of memory while trying to add %u class methods from class '%s'.\n", classMethodCount, pClassName );
        return 0;
    }

    return 1;
}

void createCSourceCodeForMethods( MethodType methodType, Method* ppMethods, const uint32_t methodCount, SourceCodeGeneratorInput* pCodeGenInput )
{
    StringAllocator*        pStringAllocator    = pCodeGenInput->pStringAllocator;
    ObjCTypeDict*           pTypeDict           = pCodeGenInput->pTypeDict;
    ObjCFunctionCollection* pFunctionCollection = pCodeGenInput->pFunctionCollection;    
    const ObjCClassName*    pClassName          = pCodeGenInput->pClassName;
    FILE*                   pSourceFileHandle   = pCodeGenInput->pSourceFileHandle;
    FILE*                   pHeaderFileHandle   = pCodeGenInput->pHeaderFileHandle;

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
            printf_stderr("[error] empty method selector for %d. method of class '%s' - Skipping method.\n", methodIndex, pClassName->pName );
            continue;
        }

        const char* pSelectorName = sel_getName( pMethodSelector );
        if( pSelectorName == NULL )
        {
            printf_stderr("[error] empty method selector name for %d. method of class '%s' - Skipping method.\n", methodIndex, pClassName->pName );
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
        parseResult.methodType          = methodType;

        ObjCFunctionResolveResult functionResolveResult;
        const ConvertResult convertResult = convertParseResultToFunctionDefinition( pStringAllocator, &functionResolveResult, pFunctionCollection, pTypeDict, &parseResult, pClassName );
        switch( convertResult )
        {
            case ConvertResult_OutOfMemory:
                printf_stderr("[error] Out of memory while trying to parse function of class '%s'.\n", pClassName->pName );
                break;

            case ConvertResult_UnknownArgumentType:
                printf_stderr("[error] Skipping function '%s' of class '%s' because of unknown argument type.\n", parseResult.pFunctionName, pClassName->pName );
                break;

            case ConvertResult_UnknownReturnType:
                printf_stderr("[error] Skipping function '%s' of class '%s' because of unknown argument type.\n", parseResult.pFunctionName, pClassName->pName );
                break;

            case ConvertResult_InvalidFunctionName:
            case ConvertResult_AlreadyKnown:
                //FK: Add log here?
                break;

            case ConvertResult_Success:
                writeCFunctionDeclaration( pHeaderFileHandle, &functionResolveResult );
                writeCFunctionImplementation( pSourceFileHandle, &functionResolveResult, pClassName );
                break;
        }
        resetStringAllocator( pStringAllocator );
    }
}

void createCSoureCodeForMethodCollection( const ClassMethodCollection* pMethodCollection, SourceCodeGeneratorInput* pCodeGenInput )
{
    createCSourceCodeForMethods( MethodType_Instance, pMethodCollection->ppInstanceMethods, pMethodCollection->instanceMethodCount, pCodeGenInput );
    createCSourceCodeForMethods( MethodType_Class, pMethodCollection->ppClassMethods, pMethodCollection->classMethodCount, pCodeGenInput );

    resetFunctionCollection( pCodeGenInput->pFunctionCollection );
}

boolean8_t createCSourceCodeForClass( ObjCTypeDict* pTypeDict, ObjCFunctionCollection* pFunctionCollection, StringAllocator* pStringAllocator, Class pClass, const char* pClassName, int32_t classNameLength )
{
    ClassMethodCollection classMethodCollection = {};
    if( !collectMethodsFromClass( &classMethodCollection, pClass, pClassName ) )
    {
        return 0u;
    }

    //FK: TODO: Instead of completely reparsing the base class over-and-over again,
    //    we should use the type dict to speed up parsing...

    //FK: Move up the class hiearchy
    Class pSuperClass = class_getSuperclass( pClass );
    while( pSuperClass != NULL )
    {
        const char* pSuperClassName = class_getName( pSuperClass );
        if( !collectMethodsFromClass( &classMethodCollection, pSuperClass, pSuperClassName ) )
        {
            return 0u;
        }

        pSuperClass = class_getSuperclass( pSuperClass );
    }

    ObjCClassName className;
    if( !createObjCClassName( &className, pClassName, classNameLength ) )
    {
        return 0u;
    }

    //FK: +2 for file extension (.c/.h)
    char* pHeaderFileName = (char*)alloca( classNameLength + 2);
    char* pSourceFileName = (char*)alloca( classNameLength + 2);

    sprintf( pHeaderFileName, "%s.h", className.pNameLower );
    sprintf( pSourceFileName, "%s.c", className.pNameLower );

    FILE* pSourceFileHandle = fopen( pSourceFileName, "w" );
    FILE* pHeaderFileHandle = fopen( pHeaderFileName, "w" );

    if( pSourceFileHandle == NULL )
    {
        printf_stderr( "[error] Couldn't open '%s' for writing.\n", pSourceFileName );
        return 0u;
    }

    if( pHeaderFileName == NULL )
    {
        printf_stderr( "[error] Couldn't open '%s' for writing.\n", pHeaderFileName );
        return 0u;
    }

    writeCHeaderPrefix( pHeaderFileHandle, &className );
    writeCSourcePrefix( pSourceFileHandle, pHeaderFileName, &className );

    SourceCodeGeneratorInput codeGenInput;
    codeGenInput.pHeaderFileHandle      = pHeaderFileHandle;
    codeGenInput.pSourceFileHandle      = pSourceFileHandle;
    codeGenInput.pStringAllocator       = pStringAllocator;
    codeGenInput.pTypeDict              = pTypeDict;
    codeGenInput.pFunctionCollection    = pFunctionCollection;
    codeGenInput.pClassName             = &className;
    createCSoureCodeForMethodCollection( &classMethodCollection, &codeGenInput );
    
    writeCHeaderSuffix( pHeaderFileHandle );
    
    fclose( pHeaderFileHandle );
    fclose( pSourceFileHandle );

    return 1u;
};

int main(int argc, const char** argv)
{
    const size_t dictTypeSizeInBytes = 1024*1024;
    const size_t dictFunctionSizeInBytes = 1024*1024*2;

    ObjCTypeDict typeDict;
    if( !createObjectiveCTypeDictionary( &typeDict, dictTypeSizeInBytes, dictFunctionSizeInBytes ) )
    {
        printf_stderr( "Out of memory while trying to create objective c type dictionary of size %.3fkB.", (float)(dictTypeSizeInBytes+dictFunctionSizeInBytes)/1024.f );
        return 2;
    }

    ObjCFunctionCollection functionCollection;
    if( !createObjectiveCFunctionCollection( &functionCollection ) )
    {
        printf_stderr( "Out of memory while trying to create objective c function collection of size %.3fkB", (float)(sizeof( char* ) * functionCollection.capacity)/1024.f );
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
        const int32_t classNameLength = getCStringLengthInclNullTerminator( pClassName );
        //if( classNameLength >= 2u && pClassName[0] == 'N' && pClassName[1] == 'S' )
        if( strcmp( pClassName, "NSOpenGLPixelFormat" ) == 0u ||
            strcmp( pClassName, "NSMenu" ) == 0u ||
            strcmp( pClassName, "NSApplication" ) == 0u || 
            strcmp( pClassName, "NSColor" ) == 0u ||
            strcmp( pClassName, "NSProcessInfo" ) == 0u ||
            strcmp( pClassName, "NSMenuItem") == 0 ||
            strcmp( pClassName, "NSScreen" ) == 0u || 
            strcmp( pClassName, "NSArray" ) == 0 || 
            strcmp( pClassName, "NSObject") == 0 ||
            strcmp( pClassName, "NSView" ) == 0u ||
            strcmp( pClassName, "NSOpenGLContext" ) == 0u ||
            strcmp( pClassName, "NSWindow" ) == 0u ||
            strcmp( pClassName, "NSString" ) == 0u ||
            strcmp( pClassName, "NSEvent") == 0u ||
            strcmp( pClassName, "NSDate" ) == 0u ||
            strcmp( pClassName, "UIFont" ) == 0u ||
            strcmp( pClassName, "UIFontStyle" ) == 0u )
        {
            createCSourceCodeForClass( &typeDict, &functionCollection, &stringAllocator, pClass, pClassName, classNameLength );
        }

        resetStringAllocator( &stringAllocator );
    }

    resolveStructTypesInTypeDictionary( &typeDict, &stringAllocator );
    return 0;
}