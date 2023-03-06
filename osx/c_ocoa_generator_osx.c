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

typedef struct
{
    Method* ppClassMethods;
    Method* ppInstanceMethods;

    uint32_t classMethodCount;
    uint32_t instanceMethodCount;
} c_ocoa_class_method_collection;

typedef struct
{
    c_ocoa_objc_function_collection* pFunctionCollection;
    c_ocoa_objc_type_dictionary*     pTypeDict;
    const c_ocoa_objc_class_name*    pClassName;
    c_ocoa_string_allocator*         pStringAllocator;
    FILE*                            pHeaderFileHandle;
    FILE*                            pSourceFileHandle;
} c_ocoa_source_code_generator_input;

bool objc_runtime_add_class_methods_to_collection( c_ocoa_class_method_collection* pMethodCollection, Method* pMethods, uint32_t methodCount )
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

    memory_copy_non_overlapping( pNewMethodArray, pMethodCollection->ppClassMethods, sizeof(Method) * oldMethodCount );
    memory_copy_non_overlapping( pNewMethodArray + oldMethodCount, pMethods, sizeof(Method) * methodCount );

    pMethodCollection->ppClassMethods = pNewMethodArray;
    pMethodCollection->classMethodCount = newMethodCount;

    return true;
}

boolean8_t objc_runtime_add_instance_methods_to_collection( c_ocoa_class_method_collection* pMethodCollection, Method* pMethods, uint32_t methodCount )
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

    memory_copy_non_overlapping( pNewMethodArray, pMethodCollection->ppInstanceMethods, sizeof(Method) * oldMethodCount );
    memory_copy_non_overlapping( pNewMethodArray + oldMethodCount, pMethods, sizeof(Method) * methodCount );

    free( pMethodCollection->ppInstanceMethods );

    pMethodCollection->ppInstanceMethods = pNewMethodArray;
    pMethodCollection->instanceMethodCount = newMethodCount;

    return 1;
}

boolean8_t objc_runtime_collect_methods_from_class( c_ocoa_class_method_collection* pMethodCollection, Class pClass, const char* pClassName )
{
    Class pMetaClass = object_getClass( (id)pClass );
    uint32_t instanceMethodCount    = 0u;
    uint32_t classMethodCount       = 0u;

    Method* pInstanceMethods    = class_copyMethodList( pClass, &instanceMethodCount );
    Method* pClassMethods       = class_copyMethodList( pMetaClass, &classMethodCount );

    const boolean8_t addedInstanceMethods = objc_runtime_add_instance_methods_to_collection( pMethodCollection, pInstanceMethods, instanceMethodCount );
    const boolean8_t addedClassMethods    = objc_runtime_add_class_methods_to_collection( pMethodCollection, pClassMethods, classMethodCount );

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

void c_ocoa_create_source_code_for_objc_methods( c_ocoa_method_type c_ocoa_method_type, Method* ppMethods, const uint32_t methodCount, c_ocoa_source_code_generator_input* pCodeGenInput )
{
    c_ocoa_string_allocator*        pStringAllocator    = pCodeGenInput->pStringAllocator;
    c_ocoa_objc_type_dictionary*           pTypeDict           = pCodeGenInput->pTypeDict;
    c_ocoa_objc_function_collection* pFunctionCollection = pCodeGenInput->pFunctionCollection;
    const c_ocoa_objc_class_name*    pClassName          = pCodeGenInput->pClassName;
    FILE*                   pSourceFileHandle   = pCodeGenInput->pSourceFileHandle;
    FILE*                   pHeaderFileHandle   = pCodeGenInput->pHeaderFileHandle;

    c_ocoa_parse_result parseResult = {};
    for( uint32_t methodIndex = 0u; methodIndex < methodCount; ++methodIndex )
    {
        Method pMethod = ppMethods[ methodIndex ];

        const size_t stringAllocatorCapacity = (size_t)string_allocator_get_remaining_capacity( pStringAllocator );
        char* pReturnType = string_allocator_get_current_base( pStringAllocator );
        
        //FK: There seems to be no way to know the return type length beforehand...?
        //    NOTE: Potential memory trampler here since we don't know beforehand
        //          if the string allocator can satisfy this allocation or not
        //          Alternativaly, we could use 'method_copyReturnType()' but that
        //          calls 'malloc()' each time internally...
        method_getReturnType( pMethod, pReturnType, stringAllocatorCapacity );

        const int32_t returnTypeLength = string_get_length_excl_null_terminator( pReturnType );
        string_allocator_decrement_capacity( pStringAllocator, returnTypeLength + 1 );

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

        const int32_t selectorNameLength = string_get_length_excl_null_terminator( pSelectorName );
        char* pMethodName = string_allocate_copy_with_allocator( pStringAllocator, pSelectorName, selectorNameLength );

        const uint32_t argumentCount = method_getNumberOfArguments( pMethod );
        char* pArgumentTypes = string_allocator_get_current_base( pStringAllocator );
        char* pCurrentArgumentType = pArgumentTypes;
        for( uint32_t argumentIndex = 0u; argumentIndex < argumentCount; ++argumentIndex )
        {
            const uint32_t stringAllocatorCapacity = string_allocator_get_remaining_capacity( pStringAllocator );
            method_getArgumentType( pMethod, argumentIndex, pCurrentArgumentType, stringAllocatorCapacity );

            const int32_t argumentTypeLength = string_get_length_excl_null_terminator( pCurrentArgumentType );

            const uint8_t addSpaceAtEnd = ( argumentIndex + 1 != argumentCount );
            if( addSpaceAtEnd )
            {
                pCurrentArgumentType[ argumentTypeLength ] = ' ';
            }

            //FK: +1 for space if not at end of iteration and for null terminator if last iteration
            string_allocator_decrement_capacity( pStringAllocator, argumentTypeLength + 1);
            pCurrentArgumentType = pCurrentArgumentType + argumentTypeLength + 1;
        }

        const int32_t argumentCharacterWritten = cast_size_to_int32( pCurrentArgumentType - pArgumentTypes );
        pArgumentTypes[ argumentCharacterWritten + 1] = 0;

        parseResult.pArguments          = pArgumentTypes;
        parseResult.argumentLength      = argumentCharacterWritten;
        parseResult.pFunctionName       = pMethodName;
        parseResult.functionNameLength  = selectorNameLength;
        parseResult.pReturnType         = pReturnType;
        parseResult.returnTypeLength    = returnTypeLength;
        parseResult.c_ocoa_method_type  = c_ocoa_method_type;

        c_ocoa_objc_function_resolve_result functionResolveResult;
        const c_ocoa_convert_result c_ocoa_convert_result = objc_parse_result_convert_to_function_definition( pStringAllocator, &functionResolveResult, pFunctionCollection, pTypeDict, &parseResult, pClassName );
        switch( c_ocoa_convert_result )
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
                cfunction_write_declaration( pHeaderFileHandle, &functionResolveResult );
                file_write_c_function_implementation( pSourceFileHandle, &functionResolveResult, pClassName );
                break;
        }
        string_allocator_reset( pStringAllocator );
    }
}

void c_ocoa_create_source_code_for_objc_method_collection( const c_ocoa_class_method_collection* pMethodCollection, c_ocoa_source_code_generator_input* pCodeGenInput )
{
    c_ocoa_create_source_code_for_objc_methods( MethodType_Instance, pMethodCollection->ppInstanceMethods, pMethodCollection->instanceMethodCount, pCodeGenInput );
    c_ocoa_create_source_code_for_objc_methods( MethodType_Class, pMethodCollection->ppClassMethods, pMethodCollection->classMethodCount, pCodeGenInput );

    objc_function_collection_reset( pCodeGenInput->pFunctionCollection );
}

boolean8_t c_ocoa_create_source_code_for_objc_class( c_ocoa_objc_type_dictionary* pTypeDict, c_ocoa_objc_function_collection* pFunctionCollection, c_ocoa_string_allocator* pStringAllocator, Class pClass, const char* pClassName, int32_t classNameLength )
{
    c_ocoa_class_method_collection c_ocoa_class_method_collection = {};
    if( !objc_runtime_collect_methods_from_class( &c_ocoa_class_method_collection, pClass, pClassName ) )
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
        if( !objc_runtime_collect_methods_from_class( &c_ocoa_class_method_collection, pSuperClass, pSuperClassName ) )
        {
            return 0u;
        }

        pSuperClass = class_getSuperclass( pSuperClass );
    }

    c_ocoa_objc_class_name className;
    if( !objc_create_class_name( &className, pClassName, classNameLength ) )
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

    file_write_c_header_prefix( pHeaderFileHandle, &className );
    file_write_c_source_prefix( pSourceFileHandle, pHeaderFileName, &className );

    c_ocoa_source_code_generator_input codeGenInput;
    codeGenInput.pHeaderFileHandle      = pHeaderFileHandle;
    codeGenInput.pSourceFileHandle      = pSourceFileHandle;
    codeGenInput.pStringAllocator       = pStringAllocator;
    codeGenInput.pTypeDict              = pTypeDict;
    codeGenInput.pFunctionCollection    = pFunctionCollection;
    codeGenInput.pClassName             = &className;
    c_ocoa_create_source_code_for_objc_method_collection( &c_ocoa_class_method_collection, &codeGenInput );
    
    file_write_c_header_suffix( pHeaderFileHandle );
    
    fclose( pHeaderFileHandle );
    fclose( pSourceFileHandle );

    return 1u;
};

int c_ocoa_create_classes_api( c_ocoa_objc_type_dictionary* pTypeDict, c_ocoa_objc_function_collection* pFunctionCollection, c_ocoa_string_allocator* pStringAllocator )
{
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
        const int32_t classNameLength = string_get_length_incl_null_terminator( pClassName );

        if( ( pClassName[0] == 'U' && pClassName[1] == 'I' ) ||
            ( pClassName[0] == 'N' && pClassName[1] == 'S' ) ||
           ( pClassName[0] == 'G' && pClassName[1] == 'L' ) )
        {
            //c_ocoa_create_source_code_for_objc_class( pTypeDict, pFunctionCollection, pStringAllocator, pClass, pClassName, classNameLength );
        }

        string_allocator_reset( pStringAllocator );
    }

    free( ppClasses );

    return 0;
}

int main(int argc, const char** argv)
{
    const size_t dictTypeSizeInBytes = 1024*1024;
    const size_t dictFunctionSizeInBytes = 1024*1024*2;

    c_ocoa_objc_type_dictionary typeDict;
    if( !objc_type_dict_create( &typeDict, dictTypeSizeInBytes, dictFunctionSizeInBytes ) )
    {
        printf_stderr( "Out of memory while trying to create objective c type dictionary of size %.3fkB.", (float)(dictTypeSizeInBytes+dictFunctionSizeInBytes)/1024.f );
        return 2;
    }

    c_ocoa_objc_function_collection functionCollection;
    if( !objc_function_collection_create( &functionCollection ) )
    {
        printf_stderr( "Out of memory while trying to create objective c function collection of size %.3fkB", (float)(sizeof( char* ) * functionCollection.capacity)/1024.f );
        return 2;
    }

    const size_t stringAllocatorSizeInBytes = 1024*1024; //FK: 1 MiB, quite a lot for a string allocator but better be safe than sorry
    c_ocoa_string_allocator stringAllocator;
    if( !string_allocator_create( &stringAllocator, stringAllocatorSizeInBytes ) )
    {
        printf_stderr( "Out of memory while trying to create string allocator of size %.3fKiB.", (float)stringAllocatorSizeInBytes/1024.f );
        return 2;
    }
    
    FILE* pTypesFileHandle = fopen( "c_ocoa_types.h", "w" );
    if( pTypesFileHandle == NULL )
    {
        printf_stderr( "Could not open '%s' for writing.", "c_ocoa_types.h" );
        return 2;
    }

    const int returnValue = c_ocoa_create_classes_api( &typeDict, &functionCollection, &stringAllocator );
    if( returnValue != 0 )
    {
        return returnValue;
    }

    objc_type_dict_resolve_struct_types( &typeDict, pTypesFileHandle );
    
    fflush( pTypesFileHandle );
    fclose( pTypesFileHandle );
    
    return 0;
}
