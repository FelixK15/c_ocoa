#ifndef C_OCOA_GENERATOR_HEADER
#define C_OCOA_GENERATOR_HEADER
#define C_OCOA_GENERATOR_MAJOR_VERSION 1
#define C_OCOA_GENERATOR_MINOR_VERSION 0

#include <stdint.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#define assert_runtime(x)    if(!(x)) raise(SIGTRAP)
#define code_path_invalid()  assert_runtime(0)
#define hook_breakpoint()    asm("nop")
#define restrict_modifier   __restrict__

#include <objc/runtime.h>

typedef enum
{
    ConvertResult_Success = 0,
    ConvertResult_OutOfMemory,
    ConvertResult_InvalidFunctionName,
    ConvertResult_UnknownReturnType,
    ConvertResult_UnknownArgumentType,
    ConvertResult_AlreadyKnown,
} c_ocoa_convert_result;

typedef enum
{
    MethodType_Class = 0,
    MethodType_Instance
} c_ocoa_method_type;

typedef uint8_t boolean8_t;

typedef struct 
{
    int32_t length;
    char* pName;
    char* pNameLower;
    char* pNameUpper;
} c_ocoa_objc_class_name;

typedef struct
{
    const char* pOriginalType;
    const char* pResolvedType;
    int32_t     resolvedTypeLength;
    int32_t     originalTypeLength;
    uint32_t    typeSizeInBits;

    boolean8_t  isReference;
    boolean8_t  isBaseType;
    boolean8_t  isFloatingType;
    boolean8_t  isConst;
} c_ocoa_objc_type_resolve_result;

typedef struct
{
    const char* pResolvedReturnType;
    const char* pResolvedArgumentTypes[32];

    const c_ocoa_objc_class_name*    pClassName;
    char*                   pResolvedFunctionName;
    char*                   pOriginalFunctionName;
    char*                   pOriginalReturnType;
    char*                   pOriginalArgumentTypes;
    
    c_ocoa_method_type              methodType;

    uint32_t                returnValueSizeInBits;
    uint8_t                 argumentCount;
    boolean8_t              isVoidFunction          : 1;
    boolean8_t              isAllocFunction         : 1;
    boolean8_t              hasStructReturnValue    : 1;
    boolean8_t              hasFloatReturnValue     : 1;
} c_ocoa_objc_function_resolve_result;

typedef FILE*(*fopen_fn)(const char* pPath, const char* pMode);
typedef int(*fclose_fn)(FILE* pStream);
typedef int(*fflush_fn)(FILE* pStream);

typedef struct
{
    const char* pOutputPath;
    const char* pPrefix;
    const char* pClassNameFilter;
    
    fopen_fn    fopen;
    fclose_fn   fclose;
    fflush_fn   fflush;
} c_ocoa_code_generator_parameter;

typedef struct
{
    void*                               pNext;
    char                                hashValue[128];
    char*                               pHashValue; //FK: dynamically allocated in case the hash value is > 128
    c_ocoa_objc_type_resolve_result     resolveResult;
    uint32_t                            index;
    boolean8_t                          declarationWasWritten;
} c_ocoa_objc_type_dictionary_entry;

typedef struct
{
    void* pNext;
    c_ocoa_objc_type_dictionary_entry* pEntries;
    uint32_t entryCount;
    uint32_t entryCapacity;
} c_ocoa_objc_type_dictionary_entry_block;

typedef struct
{
    c_ocoa_objc_type_dictionary_entry_block*    pFreeEntries;
    c_ocoa_objc_type_dictionary_entry**         ppTypeEntries;
    uint32_t                                    typeEntryCapacity;
    uint32_t                                    typeEntryCount;
} c_ocoa_objc_type_dictionary;

typedef struct
{
    const char* pTestFilePath;
} c_ocoa_command_line_parse_result;

typedef struct
{
    FILE* pHeaderFileHandle;
    FILE* pSourceFileHandle;

    const char* pHeaderFileName;
    const char* pSourceFileName;
} c_ocoa_objc_conversion_arguments;

typedef struct
{
    char*       pReturnType;
    char*       pFunctionName;
    char*       pArguments;

    int32_t     returnTypeLength;
    int32_t     functionNameLength;
    int32_t     argumentLength;

    c_ocoa_method_type  c_ocoa_method_type;
} c_ocoa_parse_result;

typedef struct 
{
    char* pBufferStart;
    uint32_t bufferCapacity;
    uint32_t bufferSize;
} c_ocoa_string_allocator;

typedef struct
{
    int32_t     functionNameLength;
    const char* pFunctionName;
} c_ocoa_objc_function_collection_entry;

typedef struct 
{
    uint32_t                        capacity;
    uint32_t                        size;
    c_ocoa_objc_function_collection_entry*    pEntries;
} c_ocoa_objc_function_collection;


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

typedef struct
{
    c_ocoa_objc_type_dictionary     typeDict;
    c_ocoa_objc_function_collection functionCollection;
    c_ocoa_string_allocator         stringAllocator;
} c_ocoa_code_gen_context;

//FK: Some helpful macros
#define printf_stderr(x, ...)    fprintf( stderr, x, ## __VA_ARGS__ )
#define array_count(x)           (sizeof(x)/sizeof(x[0]))
#define get_max(a,b)             (a)>(b)?(a):(b)
#define unused_argument(x)       (void)x

void c_ocoa_create_source_code_for_objc_methods( c_ocoa_method_type c_ocoa_method_type, Method* ppMethods, const uint32_t methodCount, c_ocoa_source_code_generator_input* pCodeGenInput );
c_ocoa_convert_result objc_parse_result_convert_to_function_definition( c_ocoa_string_allocator* pStringAllocator, c_ocoa_objc_function_resolve_result* pOutFunctionResolveResult, c_ocoa_objc_function_collection* pFunctionCollection, c_ocoa_objc_type_dictionary* pDict, c_ocoa_parse_result* pParseResult, const c_ocoa_objc_class_name* pClassName );
void file_write_c_function_implementation( FILE* pSourceFileHandle, const c_ocoa_objc_function_resolve_result* pFunctionResolveResult, const c_ocoa_objc_class_name* pClassName );
void cfunction_write_declaration( FILE* pResultFileHandle, const c_ocoa_objc_function_resolve_result* pFunctionDefinition );
boolean8_t objc_create_class_name( c_ocoa_objc_class_name* pOutClassName, const char* pClassNameStart, const int32_t classNameLength );
void objc_type_dict_resolve_struct_types( c_ocoa_objc_type_dictionary* pTypeDict, FILE* pTypesFileHandle );


void string_allocator_reset( c_ocoa_string_allocator* pStringAllocator )
{
    pStringAllocator->bufferSize = 0u;
}

char* string_allocator_get_current_base( c_ocoa_string_allocator* pStringAllocator )
{
    return pStringAllocator->pBufferStart + pStringAllocator->bufferSize;
}

void string_allocator_decrement_capacity( c_ocoa_string_allocator* pStringAllocator, uint32_t sizeInBytes )
{
    assert_runtime( pStringAllocator->bufferSize + sizeInBytes < pStringAllocator->bufferCapacity );
    pStringAllocator->bufferSize += sizeInBytes;
}

uint32_t string_allocator_get_remaining_capacity( c_ocoa_string_allocator* pStringAllocator )
{
    return pStringAllocator->bufferCapacity - pStringAllocator->bufferSize;
}

void memory_copy_non_overlapping( void* pDestination, const void* pSource, const size_t sizeInBytes )
{
    memcpy(pDestination, pSource, sizeInBytes);
}

static inline int32_t cast_size_to_int32( size_t val )
{
    assert_runtime( val < INT32_MAX );
    return (int32_t)val;
}

//FK: Use custom string.h alternatives since we're not concerned with locale 
static inline boolean8_t char_is_white_space( const char character )
{
    return character == ' '  ||
           character == '\n' ||
           character == '\t' ||
           character == '\v' ||
           character == '\f' ||
           character == '\r';
}

static inline boolean8_t char_is_alpha( const char character )
{
    return ( character >= 'A' && character <= 'Z' ) || ( character >= 'a' && character <= 'z' );
}

static inline char convertCharacterToLower( const char character )
{
    if( !char_is_alpha( character ) )
    {
        return character;
    }

    return character <= 'Z' ? character + 32 : character;
}

static inline char char_convert_to_upper( const char character )
{
    if( !char_is_alpha( character ) )
    {
        return character;
    }

    return character >= 'a' ? character - 32 : character;
}

static inline boolean8_t string_is_equal( const char* pStringA, const char* pStringB, const int32_t stringLength )
{
    for( int32_t charIndex = 0u; charIndex < stringLength; ++charIndex )
    {
        if( pStringA[ charIndex ] != pStringB[ charIndex ] )
        {
            return 0u;
        }
    }

    return 1u;
}

static inline int32_t string_get_length_excl_null_terminator( const char* pString )
{
    const char* pStringStart = pString;
    while( *pString )
    {
        if( *pString == 0 )
        {
            break; 
        }

        ++pString;
    }

    return cast_size_to_int32( pString - pStringStart );
}

static inline int32_t string_get_length_incl_null_terminator( const char* pString )
{
    const char* pStringStart = pString;
    while( *pString++ );

    return cast_size_to_int32( pString - pStringStart );
}

static inline char* string_copy_and_add_null_terminator( char* pDestination, const char* pSource, const int32_t stringLength )
{
    for( int32_t charIndex = 0; charIndex < stringLength; ++charIndex )
    {
        pDestination[ charIndex ] = pSource[ charIndex ];
    }

    pDestination[ stringLength ] = 0;
    return pDestination;
}

static inline char* string_convert_to_lower_inplace( char* pString, const int32_t stringLength )
{
    for( int32_t charIndex = 0u; charIndex < stringLength; ++charIndex )
    {
        pString[ charIndex ] = convertCharacterToLower( pString[ charIndex ] );
    }
    return pString;
}

static inline char* string_convert_to_upper_inplace( char* pString, const int32_t stringLength )
{
    for( int32_t charIndex = 0u; charIndex < stringLength; ++charIndex )
    {
        pString[ charIndex ] = char_convert_to_upper( pString[ charIndex ] );
    }
    return pString;
}

static inline char* string_allocate_copy( const char* pString, const int32_t stringLength )
{
    char* pStringCopyMemory = (char*)malloc( stringLength + 1 );
    if( pStringCopyMemory == NULL )
    {
        //FK: TODO: Handle out of memory
        return NULL;
    }

    return string_copy_and_add_null_terminator( pStringCopyMemory, pString, stringLength );
}

static inline char* string_allocate_copy_lower( const char* pString, const int32_t stringLength )
{
    char* pStringCopyMemory = (char*)malloc( stringLength + 1 );
    if( pStringCopyMemory == NULL )
    {
        //FK: TODO: Handle out of memory
        return NULL;
    }

    string_copy_and_add_null_terminator( pStringCopyMemory, pString, stringLength );
    return string_convert_to_lower_inplace( pStringCopyMemory, stringLength );
}

static inline char* string_allocator_copy_upper( const char* pString, const int32_t stringLength )
{
    char* pStringCopyMemory = (char*)malloc( stringLength + 1 );
    if( pStringCopyMemory == NULL )
    {
        //FK: TODO: Handle out of memory
        return NULL;
    }

    string_copy_and_add_null_terminator( pStringCopyMemory, pString, stringLength );
    return string_convert_to_upper_inplace( pStringCopyMemory, stringLength );
}

static inline char* string_allocator_allocate( c_ocoa_string_allocator* pStringAllocator, const uint32_t length )
{
    char* pReturnString = string_allocator_get_current_base( pStringAllocator );
    string_allocator_decrement_capacity( pStringAllocator, length );
    return pReturnString;
}

static inline char* string_allocate_copy_with_allocator( c_ocoa_string_allocator* pAllocator, const char* pString, const int32_t stringLength )
{
    char* pStringCopyMemory = string_allocator_allocate(pAllocator, stringLength + 1);
    return string_copy_and_add_null_terminator( pStringCopyMemory, pString, stringLength );
}

static inline const char* string_find_last_position( const char* pString, char character )
{
    const char* pLastOccurance = NULL;
    while( *pString )
    {
        if( *pString == character )
        {
            pLastOccurance = pString;
        }

        ++pString;
    }

    return pLastOccurance;
}

static inline const char* string_find_next_char_position( const char* pString, char character )
{
    while( *pString )
    {
        if( *pString == character )
        {
            return pString;
        }

        ++pString;
    }

    return NULL;
}

c_ocoa_command_line_parse_result command_line_arguments_parse( const int argc, const char** argv )
{
    typedef enum
    {
        NextArgument,
        ParseTestArg
    } command_line_parse_state;

    command_line_parse_state state = NextArgument;
    c_ocoa_command_line_parse_result result = {};

    for( int argIndex = 0u; argIndex < argc; ++argIndex )
    {
        switch( state )
        {
            case NextArgument:
            {
                if( string_is_equal( argv[argIndex], "--test", 6u ) )
                {
                    state = ParseTestArg;
                    continue;
                }
                break;
            }

            case ParseTestArg:
            {
                result.pTestFilePath = argv[argIndex];
                state = NextArgument;
                break;
            }

            default:
            {
                code_path_invalid();
                break;
            }
        }
    }

    return result;
}

boolean8_t string_name_matches_filter( const char* restrict_modifier pName, const char* restrict_modifier pNameFilter)
{
    boolean8_t wildcard = 0u;
    while( 1 )
    {
        if( *pName == 0u )
        {
            return pNameFilter == NULL || *pNameFilter == 0;
        }
        else if( *pName != 0u && *pNameFilter == 0u && !wildcard )
        {
            return 0u;
        }

        if( *pNameFilter != '*' )
        {
            if( wildcard )
            {
                if( *pNameFilter == 0u )
                {
                    //FK: early-out - there are only wildcards left
                    return 1u;
                }
                else 
                {
                    if( *pNameFilter == *pName )
                    {
                        wildcard = 0u;
                        ++pName;
                        ++pNameFilter;
                    }
                    else
                    {
                        ++pName;
                    }
                }
            }
            else
            {
                if( *pName == *pNameFilter )
                {
                    ++pName;
                    ++pNameFilter;
                    continue;
                }
                else
                {
                    return 0u;
                }
            }
           
        }
        else
        {
            wildcard = 1u;
            ++pNameFilter;
        }
    }

    return 1u;
}

void print_help(void)
{
    printf("C OCOA CODE GENERATOR VERSION %d.%d\n", C_OCOA_GENERATOR_MAJOR_VERSION, C_OCOA_GENERATOR_MINOR_VERSION);
    printf("Usage: c_ocoa_generator [OPTIONS] [FILTER]\n\n");
    printf("OPTIONS are:\n");
    printf("-o {path}    set output path (output directory)\n");
    printf("-p {prefix}  set prefix for output source files\n");
    printf("-h           print this help text\n\n");
    printf("FILTER is:\n");
    printf("A filter for the class name to be exported. Supports wildcard.\n\n");
    printf("eg: 'c_ocoa_generator NS*'  - export all classes that start with 'NS'.\n");
}

void evaluate_code_generator_argv_arguments( int argc, const char** argv, c_ocoa_code_generator_parameter* pOutArguments )
{
    for( int i = 1; i < argc; ++i )
    {
        const char* pArg = argv[i];
        if( pArg[0] == '-' )
        {
            switch( pArg[1] )
            {
                case 'o':
                    pOutArguments->pOutputPath = argv[i+1];
                    ++i;
                break;

                case 'p':
                    pOutArguments->pPrefix = argv[i+1];
                    ++i;
                break;
   
                case 'h':
                    print_help();
                    exit(0);
                break;
            }
        }
        else
        {
            //FK: it is assumed that the class name filter is the last argument
            pOutArguments->pClassNameFilter = pArg;
            break;
        }
    }
}

boolean8_t c_ocoa_objc_type_dictionary_create_entry_block( c_ocoa_objc_type_dictionary_entry_block** pOutEntryBlock, uint32_t entryCapacityPerBlock )
{
    const uint32_t entrySizeInBytes = sizeof(c_ocoa_objc_type_dictionary_entry) * entryCapacityPerBlock;
    c_ocoa_objc_type_dictionary_entry* pEntries = (c_ocoa_objc_type_dictionary_entry*)malloc(entrySizeInBytes);
    
    if( pEntries == NULL )
    {
        return 0;
    }
    
    c_ocoa_objc_type_dictionary_entry_block* pBlock = (c_ocoa_objc_type_dictionary_entry_block*)malloc(sizeof(c_ocoa_objc_type_dictionary_entry));
    
    if( pBlock == NULL )
    {
        free(pEntries);
        return 0;
    }
    
    memset(pEntries, 0, entrySizeInBytes);
    
    pBlock->pEntries        = pEntries;
    pBlock->entryCapacity   = entryCapacityPerBlock;
    pBlock->entryCount      = 0;
    pBlock->pNext           = NULL;
    
    *pOutEntryBlock = pBlock;
    return 1;
}

boolean8_t objc_type_dict_create( c_ocoa_objc_type_dictionary* pOutTypeDict )
{
    const size_t typeEntryCount = 4096u;
    const size_t freeEntryBlockCount = 512u;
    
    const size_t typeEntrySizeInBytes = typeEntryCount * sizeof( void* );
    c_ocoa_objc_type_dictionary_entry** ppEntries = ( c_ocoa_objc_type_dictionary_entry** )malloc(typeEntrySizeInBytes);
    if( ppEntries == NULL )
    {
        return 0;
    }
    
    if( !c_ocoa_objc_type_dictionary_create_entry_block( &pOutTypeDict->pFreeEntries, freeEntryBlockCount ) )
    {
        free(ppEntries);
        return 0;
    }
       
    memset(ppEntries, 0, typeEntrySizeInBytes);
    pOutTypeDict->ppTypeEntries         = ppEntries;
    pOutTypeDict->typeEntryCapacity     = typeEntryCount;
    pOutTypeDict->typeEntryCount        = 0u;

    return 1;
}

boolean8_t objc_function_collection_create( c_ocoa_objc_function_collection* pOutFunctionCollection )
{
    pOutFunctionCollection->capacity = 64u;
    pOutFunctionCollection->size = 0u;
    pOutFunctionCollection->pEntries = ( c_ocoa_objc_function_collection_entry* )malloc( sizeof( c_ocoa_objc_function_collection_entry ) * 64u );

    if( pOutFunctionCollection->pEntries == NULL )
    {
        return 0;
    }

    return 1;
}

boolean8_t strings_are_equal( const char* pStringA, const char* pStringB, const int32_t stringLength )
{
    for( int32_t charIndex = 0u; charIndex < stringLength; ++charIndex )
    {
        if( pStringA[ charIndex ] != pStringB[ charIndex ] )
        {
            return 0;
        }
    }

    return 1;
}

uint32_t djb2_hash( const char* pString, const int32_t stringLength )
{
    uint32_t hash = 5381;

    for( int32_t charIndex = 0u; charIndex < stringLength; ++charIndex )
    {
        const char c = pString[ charIndex ];
        hash = ((hash << 5) + hash) + c;
    }
        
    return hash;
}

c_ocoa_objc_type_dictionary_entry* objc_type_dict_find_entry( c_ocoa_objc_type_dictionary* pDict, const char* pTypeName, const int32_t typeNameLength )
{
    const uint32_t hashValue = djb2_hash( pTypeName, typeNameLength );
    const uint32_t entryIndex = hashValue % pDict->typeEntryCapacity;
    c_ocoa_objc_type_dictionary_entry* pEntry = pDict->ppTypeEntries[entryIndex];
    while( pEntry != NULL && strcmp( pEntry->pHashValue, pTypeName ) != 0 )
    {
        pEntry = (c_ocoa_objc_type_dictionary_entry*)pEntry->pNext;
    }
    
    return pEntry;
}

c_ocoa_objc_type_dictionary_entry* c_ocoa_type_dict_get_free_entry_from_block( c_ocoa_objc_type_dictionary_entry_block* pBlock )
{
    c_ocoa_objc_type_dictionary_entry_block* pCurrentBlock = pBlock;
    c_ocoa_objc_type_dictionary_entry_block* pPrevBlock = NULL;
    while( pCurrentBlock != NULL && pCurrentBlock->entryCount == pCurrentBlock->entryCapacity )
    {
        pPrevBlock = pCurrentBlock;
        pCurrentBlock = (c_ocoa_objc_type_dictionary_entry_block*)pCurrentBlock->pNext;
    }
    
    if( pCurrentBlock == NULL )
    {
        if( !c_ocoa_objc_type_dictionary_create_entry_block((c_ocoa_objc_type_dictionary_entry_block**)&pPrevBlock->pNext, pPrevBlock->entryCapacity ))
        {
            return NULL;
        }
        
        pCurrentBlock = (c_ocoa_objc_type_dictionary_entry_block*)pPrevBlock->pNext;
    }
    
    const uint32_t entryIndex = pCurrentBlock->entryCount++;
    return pCurrentBlock->pEntries + entryIndex;
}

c_ocoa_objc_type_dictionary_entry* objc_type_dict_insert_or_get_entry( c_ocoa_objc_type_dictionary* restrict_modifier pDict, const char* restrict_modifier pTypeName, const int32_t typeNameLength, boolean8_t* restrict_modifier pOutIsNew )
{
    //FK: TODO: Check hash distribution
    const uint32_t hashValue = djb2_hash( pTypeName, typeNameLength );
    const uint32_t entryIndex = hashValue % pDict->typeEntryCapacity;
    c_ocoa_objc_type_dictionary_entry* pEntry = pDict->ppTypeEntries[entryIndex];
    c_ocoa_objc_type_dictionary_entry* pPrev = NULL;
    
    const boolean8_t isFirstEntry = pEntry == NULL;
    
    while( pEntry != NULL )
    {
        if( strcmp( pEntry->pHashValue, pTypeName ) == 0 )
        {
            *pOutIsNew = 0;
            return pEntry;
        }
        
        pPrev = pEntry;
        pEntry = (c_ocoa_objc_type_dictionary_entry*)pEntry->pNext;
    }
    
    pEntry = c_ocoa_type_dict_get_free_entry_from_block(pDict->pFreeEntries);
    if( pEntry == NULL )
    {
        return NULL;
    }
    
    if( typeNameLength >= sizeof(pEntry->hashValue) )
    {
        char* pHashValue = string_allocate_copy( pTypeName, typeNameLength );
        if( pHashValue == NULL )
        {
            return NULL;
        }
        
        pEntry->pHashValue = pHashValue;
    }
    else
    {
        pEntry->pHashValue = pEntry->hashValue;
    }
    
    string_copy_and_add_null_terminator(pEntry->pHashValue, pTypeName, typeNameLength);
    
    *pOutIsNew = 1;
    
    pEntry->index = entryIndex;
    
    if(isFirstEntry)
    {
        pDict->ppTypeEntries[entryIndex] = pEntry;
        ++pDict->typeEntryCount;
        
        assert_runtime(pDict->typeEntryCount < pDict->typeEntryCapacity);
    }
    
    if( pPrev != NULL )
    {
        pPrev->pNext = pEntry;
    }
    
    return pEntry;
}

int32_t typename_get_reference_position( const char* pTypeName, const int32_t typenameLength )
{
    for( int32_t charIndex = 0u; charIndex < typenameLength; ++charIndex )
    {
        if( pTypeName[ charIndex ] == '*' || pTypeName[ charIndex ] == '^' || pTypeName[ charIndex ] == '?' )
        {
            return charIndex;
        }
    }
   
    return INT32_MAX;
}

boolean8_t objc_is_struct_type( const char* pTypeName )
{
    return *pTypeName == '{';
}

boolean8_t objc_is_reference_type( const char* pTypeName, const int32_t typeNameLength )
{
    const int32_t referencePosition = typename_get_reference_position( pTypeName, typeNameLength );
    return referencePosition != INT32_MAX;
}

boolean8_t base_type_resolve( c_ocoa_objc_type_resolve_result* pOutResult, const char* pObjectiveCTypeName )
{
    //FK: types are defined in objc/runtime.h
    //    TODO: Double check with static assert that these still map to what I think they map to
    const char* pResolvedBaseType = NULL;
    boolean8_t isFloatingType = 0;
    uint32_t typeSizeInBits = 0;

    switch( *pObjectiveCTypeName )
    {
        case '@':
            pResolvedBaseType = "nsobject_t";
            typeSizeInBits = 64u;
            break;
        case ':':
            pResolvedBaseType = "nsselector_t";
            typeSizeInBits = 64u;
            break;
        case '#':
            pResolvedBaseType = "nsclass_t";
            typeSizeInBits = 64u;
            break;
        case 'q':
            pResolvedBaseType = "long long";
            typeSizeInBits = 64u;
            break;
        case 'Q':
            pResolvedBaseType = "unsigned long long";
            typeSizeInBits = 64u;
            break;
        case 'l':
            pResolvedBaseType = "long";
            typeSizeInBits = 32u;
            break;
        case 'L':
            pResolvedBaseType = "unsigned long";
            typeSizeInBits = 32u;
            break;
        case 'i':
            pResolvedBaseType = "int";
            typeSizeInBits = 32u;
            break;
        case 'I':
            pResolvedBaseType = "unsigned int";
            typeSizeInBits = 32u;
            break;
        case 's':
            pResolvedBaseType = "short";
            typeSizeInBits = 16u;
            break;
        case 'S':
            pResolvedBaseType = "unsigned short";
            typeSizeInBits = 16u;
            break;
        case 'c':
            pResolvedBaseType = "char";
            typeSizeInBits = 8u;
            break;
        case 'C':
            pResolvedBaseType = "unsigned char";
            typeSizeInBits = 8u;
            break;
        case 'r':
            pResolvedBaseType = "const char";
            typeSizeInBits = 8u;
            break;
        case '*':
            pResolvedBaseType = "char *";
            typeSizeInBits = 64u;
            break;
        case 'v':
        case 'V':
            pResolvedBaseType = "void";
            break;
        case 'd':
        case 'D':
            pResolvedBaseType = "double";
            typeSizeInBits = 64u;
            isFloatingType = 1u;
            break;
        case 'f':
        case 'F':
            pResolvedBaseType = "float";
            typeSizeInBits = 32u;
            isFloatingType = 1u;
            break;
        case 'B':
            pResolvedBaseType = "bool";
            typeSizeInBits = 8u;
            break;
        default:
            return 0;
    }

    const int32_t resolvedTypeLength = string_get_length_excl_null_terminator( pResolvedBaseType );
    pOutResult->isReference              = ( *pObjectiveCTypeName == '*' );
    pOutResult->isBaseType               = 1;
    pOutResult->isFloatingType           = isFloatingType;
    pOutResult->originalTypeLength       = 1;
    pOutResult->pResolvedType            = string_allocate_copy( pResolvedBaseType, resolvedTypeLength );
    pOutResult->pOriginalType            = string_allocate_copy( pObjectiveCTypeName, 1 );
    pOutResult->resolvedTypeLength       = resolvedTypeLength;
    pOutResult->typeSizeInBits           = typeSizeInBits;
    return 1;
}

void objc_type_dict_remove_entry( c_ocoa_objc_type_dictionary* pDict, c_ocoa_objc_type_dictionary_entry* pEntryToRemove )
{
    const uint32_t entryIndex = pEntryToRemove->index;
    c_ocoa_objc_type_dictionary_entry* pEntry = pDict->ppTypeEntries[entryIndex];
    c_ocoa_objc_type_dictionary_entry* pPrev = NULL;
    while( strcmp( pEntry->pHashValue, pEntryToRemove->pHashValue ) != 0 )
    {
        pPrev = pEntry;
        pEntry = ( c_ocoa_objc_type_dictionary_entry* )pEntry->pNext;
    }
    
    assert_runtime( pEntry != NULL );

    //FK: is first entry in linked list?
    if( pPrev == NULL )
    {
        pDict->ppTypeEntries[entryIndex] = NULL;
        
        assert_runtime( pDict->typeEntryCount > 0 );
        --pDict->typeEntryCount;
    }
    else
    {
        pPrev->pNext = pEntry->pNext;
    }
}

boolean8_t objc_type_dict_resolve_base_type( c_ocoa_objc_type_resolve_result* pOutResult, c_ocoa_objc_type_dictionary* pDict, const char* pObjectiveCTypeName )
{
    boolean8_t isNewDictEntry = 0;
    c_ocoa_objc_type_dictionary_entry* pDictEntry = objc_type_dict_insert_or_get_entry( pDict, pObjectiveCTypeName, 1, &isNewDictEntry );
    if( pDictEntry == NULL )
    {
        printf_stderr( "[error] Could not resolve type name '%s' because we ran out of memory.", pObjectiveCTypeName );
        return 0;
    }
    
    if( !isNewDictEntry )
    {
        *pOutResult = pDictEntry->resolveResult;
        return 1;
    }

    const boolean8_t resolvedSuccessful = base_type_resolve( pOutResult, pObjectiveCTypeName );
    if( !resolvedSuccessful )
    {
        objc_type_dict_remove_entry( pDict, pDictEntry );
        return 0;
    }

    pDictEntry->resolveResult = *pOutResult;
    return 1;
}

boolean8_t objc_type_dict_resolve_struct_type( c_ocoa_objc_type_resolve_result* pOutResult, c_ocoa_objc_type_dictionary* pDict, const char* pTypeName, int32_t typeNameLength );

boolean8_t struct_type_resolve( c_ocoa_objc_type_resolve_result* pOutResult, c_ocoa_objc_type_dictionary* pDict, const char* pTypeName, int32_t typeNameLength )
{
    const char* pTypeDefinitionStart        = pTypeName;
    const char* pTypeNameStart              = pTypeName + 1; //FK: +1 to skip initial '{' of struct definition
    const char* pTypeNameEnd                = string_find_next_char_position( pTypeDefinitionStart, '=' );
    const int32_t resolvedTypeNameLength    = cast_size_to_int32( pTypeNameEnd - pTypeNameStart );

    const char* pStructDefinition = pTypeNameEnd;

    uint32_t structSizeInBits = 0u;
    uint8_t scopeCount = 1u; //FK: start at one to include first leading '{'
    ++pStructDefinition;

    while( scopeCount > 0u )
    {
        if( *pStructDefinition == '{' )
        {
            ++scopeCount;

            const char* pEmbeddedTypeNameStart = pStructDefinition;
            const char* pEmbeddedTypeNameEnd = string_find_next_char_position( pStructDefinition, '}' );
            const int32_t embeddedTypeNameLength = cast_size_to_int32( pEmbeddedTypeNameEnd - pEmbeddedTypeNameStart );

            c_ocoa_objc_type_resolve_result resolveResult = {};
            objc_type_dict_resolve_struct_type( &resolveResult, pDict, pEmbeddedTypeNameStart, embeddedTypeNameLength );

            structSizeInBits += resolveResult.typeSizeInBits;
            pStructDefinition += embeddedTypeNameLength;
        }
        else if( *pStructDefinition == '}' )
        {
            --scopeCount;
            ++pStructDefinition;
        }
        else if( scopeCount == 1u )
        {
            //FK: Resolve struct POD member in struct to calculate total struct size
            //NOTE: We assume that there are only non references in structs, lets see how that turns out...
            c_ocoa_objc_type_resolve_result resolveResult = {};
            base_type_resolve( &resolveResult, pStructDefinition );

            structSizeInBits += resolveResult.typeSizeInBits;
            ++pStructDefinition;
        }

    }

    const char* pTypeDefinitionEnd = pStructDefinition;
    const int32_t originalTypeLength = cast_size_to_int32( pTypeDefinitionEnd - pTypeDefinitionStart );

    pOutResult->isBaseType               = 0;
    pOutResult->isReference              = 0;
    pOutResult->pOriginalType            = (const char*)string_allocate_copy( pTypeDefinitionStart, originalTypeLength );
    pOutResult->pResolvedType            = (const char*)string_allocate_copy( pTypeNameStart, resolvedTypeNameLength );
    pOutResult->resolvedTypeLength       = resolvedTypeNameLength;
    pOutResult->originalTypeLength       = typeNameLength;
    pOutResult->typeSizeInBits           = structSizeInBits;
    return 1;
}

boolean8_t struct_type_is_unknown( const char* pTypeNameStart, const int32_t typeNameLength )
{
    return typeNameLength == 1u && *pTypeNameStart == '?';
}

boolean8_t objc_type_dict_resolve_struct_type( c_ocoa_objc_type_resolve_result* pOutResult, c_ocoa_objc_type_dictionary* pDict, const char* pTypeName, int32_t typeNameLength )
{
    const char* pTypeDefinitionStart = pTypeName;
    const char* pTypeNameStart = pTypeName + 1; //FK: +1 to skip initial '{' of struct definition
    
    // FK: End of typename is equal to the beginning of the struct definition
    //     which is why we search for the next '='
    const char* pTypeNameEnd = string_find_next_char_position( pTypeDefinitionStart, '=' );
    if( pTypeNameEnd == NULL )
    {
        return 0u;
    }

    boolean8_t isNewTypeDictEntry = 0;
    const int32_t resolvedTypeNameLength = cast_size_to_int32( pTypeNameEnd - pTypeNameStart );

    if( struct_type_is_unknown( pTypeNameStart, resolvedTypeNameLength ) )
    {
        printf_stderr( "[warning] typename '%.*s' is an unknown struct type. Skipping type resolve...\n", typeNameLength, pTypeName );
        return 0u;
    }

    c_ocoa_objc_type_dictionary_entry* pEntry = objc_type_dict_insert_or_get_entry( pDict, pTypeNameStart, resolvedTypeNameLength, &isNewTypeDictEntry );
    if( pEntry == NULL )
    {
        printf_stderr( "[error] could not resolve typename '%.*s' because we ran out of memory.\n", typeNameLength, pTypeName );
        return 0u;
    }
    
    if( !isNewTypeDictEntry )
    {
        *pOutResult = pEntry->resolveResult;
        return 1;
    }

    const boolean8_t resolvedSuccessfully = struct_type_resolve( pOutResult, pDict, pTypeName, typeNameLength );
    if( !resolvedSuccessfully )
    {
        objc_type_dict_remove_entry( pDict, pEntry );
        return 0;
    }
    
    pEntry->resolveResult = *pOutResult;
    return 1;
}

static inline void struct_extract_name( const char** pOutStructName, int32_t* pOutStructNameLength, const char* pStructDeclaration )
{
    if( *pStructDeclaration == '{' )
    {
        ++pStructDeclaration;
    }

    const char* pStructNameStart = pStructDeclaration;
    const char* pStructNameEnd = string_find_next_char_position( pStructNameStart, '=' );
    assert_runtime( pStructNameEnd != NULL );

    *pOutStructName = pStructNameStart;
    *pOutStructNameLength = cast_size_to_int32( pStructNameEnd - pStructNameStart ); //FK: -1 to not include '=' in the struct name
}

static inline boolean8_t struct_is_unknown( const char* pStructName, const int32_t structNameLength )
{
    return structNameLength == 1 && pStructName[0] == '?';
}

boolean8_t reference_type_resolve( c_ocoa_objc_type_resolve_result* pOutResult, const char* pTypeName, int32_t typeNameLength )
{
    const boolean8_t isConst = ( pTypeName[0] == 'r' );
    if( isConst )
    {
        pTypeName = pTypeName + 1;
        --typeNameLength;
    }

    //FK: Check if the typename is a reference and return the position of the reference identifier ('^' or '*')
    const int32_t referencePosition = typename_get_reference_position( pTypeName, typeNameLength );
    if( !( referencePosition == 0u || referencePosition == ( typeNameLength - 1u ) ) )
    {
        return 0u;
    }

    const char* pReferenceType = NULL;
    int32_t referenceTypeLength = 0;

    //FK: Special case: Reference without a typename ... this should be a char pointer??? (example seen: NSString stringWithUTF8String:)
    if( typeNameLength == 1u && *pTypeName == '*' )
    {
        //FK: hack typename to resolve to char - this is some ugly shit
        pReferenceType      = "char";
        referenceTypeLength = 5;

        pOutResult->isBaseType = 1;
    }
    else
    {
         //FK: Remove reference modifier from typename to expose underlying base type
        --typeNameLength;
        if( referencePosition == 0u )
        {
            pTypeName = pTypeName + 1;
        }

        if( objc_is_struct_type( pTypeName ) )
        {
            struct_extract_name( &pReferenceType, &referenceTypeLength, pTypeName );
            if( struct_is_unknown( pReferenceType, referenceTypeLength ) )
            {
                //FK: We don't resolve unknown structs (yet?) 
                //    "unknown structs": Structs with a name of '?'
                return 0;
            }
            pOutResult->isBaseType = 0;
        }   
        else
        {
            if( !base_type_resolve( pOutResult, pTypeName ) )
            {
                return 0;
            }

            pReferenceType      = pOutResult->pResolvedType;
            referenceTypeLength = pOutResult->resolvedTypeLength;
        }
    }

    assert_runtime( pReferenceType != NULL && referenceTypeLength > 0 );

    char* pResolvedReferenceType = (char*)alloca( referenceTypeLength + 10u ); //FK: +10 for reference indicator ('*') + NULL terminator + const modified (if needed)
    const int32_t resolvedReferenceTypeLength = sprintf( pResolvedReferenceType, "%.*s*", referenceTypeLength, pReferenceType );
    
    pOutResult->pResolvedType       = string_allocate_copy( pResolvedReferenceType, resolvedReferenceTypeLength );
    pOutResult->pOriginalType       = string_allocate_copy( pTypeName, typeNameLength );
    pOutResult->resolvedTypeLength  = referenceTypeLength;
    pOutResult->originalTypeLength  = typeNameLength;
    pOutResult->isReference         = 1;
    pOutResult->isConst             = isConst;
    pOutResult->typeSizeInBits      = 64u;
    return 1;
}

boolean8_t objc_type_dict_resolve_reference_type( c_ocoa_objc_type_resolve_result* pOutResult, c_ocoa_objc_type_dictionary* pDict, const char* pTypeName, int32_t typeNameLength )
{
    boolean8_t isNewDictEntry = 0;
    c_ocoa_objc_type_dictionary_entry* pDictEntry = objc_type_dict_insert_or_get_entry( pDict, pTypeName, typeNameLength, &isNewDictEntry );
    
    if( pDictEntry == NULL )
    {
        printf_stderr( "[error] Could not resolve typename '%.*s' because we ran out of memory.\n", typeNameLength, pTypeName );
        return 0u;
    }
    
    if( !isNewDictEntry )
    {
        *pOutResult = pDictEntry->resolveResult;
        return 1;
    }

    const boolean8_t resolvedSuccessfully = reference_type_resolve( pOutResult, pTypeName, typeNameLength );
    if( !resolvedSuccessfully )
    {
        printf_stderr( "[warning] could not resolve type name '%.*s'.\n", typeNameLength, pTypeName );
        objc_type_dict_remove_entry( pDict, pDictEntry );
        return 0;
    }

    pDictEntry->resolveResult = *pOutResult;
    return 1;
}

boolean8_t objc_resolve_c_type_name( c_ocoa_objc_type_resolve_result* pOutResult, c_ocoa_objc_type_dictionary* pTypeDict, const char* pTypeName, int32_t typeNameLength )
{
    boolean8_t resolvedSuccessfully = 0;
    if( objc_is_struct_type( pTypeName ) )
    {
        resolvedSuccessfully = objc_type_dict_resolve_struct_type( pOutResult, pTypeDict, pTypeName, typeNameLength );
    }
    else if( objc_is_reference_type( pTypeName, typeNameLength ) )
    {
        resolvedSuccessfully = objc_type_dict_resolve_reference_type( pOutResult, pTypeDict, pTypeName, typeNameLength );
    }
    else if( typeNameLength == 1u )
    {
        resolvedSuccessfully = objc_type_dict_resolve_base_type( pOutResult, pTypeDict, pTypeName );
    }
    else
    {
        resolvedSuccessfully = 0;
        printf_stderr("[error] Couldn't resolve type '%.*s' - neither struct nor reference nor base type.\n", typeNameLength, pTypeName );
    }

    return resolvedSuccessfully;
}

const char* newline_find_next( const char* pText )
{
    while( *pText )
    {
        if( *pText == '\n' )
        {
            break;
        }

        ++pText;
    }
    return pText;
}

const char* whitespace_find_next( const char* pText )
{
    while( *pText )
    {
        if( char_is_white_space( *pText ) )
        {
            break;
        }

        ++pText;
    }

    return pText;
}

const char* whitespace_find_prev( const char* pTextStart, const char* pText )
{
    while( pText != pTextStart )
    {
        if( char_is_white_space( *pText ) )
        {
            ++pText;
            break;
        }

        --pText;
    }

    return pText;
}

boolean8_t function_name_is_valid( const char* pFunctionName )
{
    //FK: Filter (apparently) internal functions that we're not interested in
    if( *pFunctionName == '_' || *pFunctionName == '.' )
    {
        return 0;
    }

#if 0
    const char* pFirstColon = string_find_next_char_position( pFunctionName, ':' );
    if( pFirstColon != NULL )
    {
        const char* pSecondColon = string_find_next_char_position( pFirstColon + 1, ':' );
        if( pSecondColon != NULL )
        {
            return 0;
        }
    }
#endif

    return 1;
}

const char* function_name_convert_to_c_function_name( char* pObjectiveCFunctionName, int32_t functionNameLength )
{
    //FK: Some functions end with a colon (not sure yet what this indicates exactly)
    char* pColonPos = (char*)string_find_next_char_position( pObjectiveCFunctionName, ':' );
    if( pColonPos != NULL )
    {
        *pColonPos = 0;
    }

    return pObjectiveCFunctionName;
}

boolean8_t string_allocator_create( c_ocoa_string_allocator* pOutStringAllocator, const uint32_t capacityInBytes )
{
    c_ocoa_string_allocator allocator;
    allocator.pBufferStart      = (char*)calloc( 1u, capacityInBytes );
    allocator.bufferSize        = 0u;
    allocator.bufferCapacity    = capacityInBytes;

    if( allocator.pBufferStart == NULL )
    {
        return 0u;
    }

    *pOutStringAllocator = allocator;
    return 1u;
}


boolean8_t function_name_ends_with_colon( const char* pFunctionName, const int32_t functionNameLength )
{
    assert_runtime( functionNameLength > 0u );
    assert_runtime( pFunctionName != NULL );
    return pFunctionName[ functionNameLength - 1u ] == ':';
}

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
    c_ocoa_string_allocator*            pStringAllocator    = pCodeGenInput->pStringAllocator;
    c_ocoa_objc_type_dictionary*        pTypeDict           = pCodeGenInput->pTypeDict;
    c_ocoa_objc_function_collection*    pFunctionCollection = pCodeGenInput->pFunctionCollection;
    const c_ocoa_objc_class_name*       pClassName          = pCodeGenInput->pClassName;
    FILE*                               pSourceFileHandle   = pCodeGenInput->pSourceFileHandle;
    FILE*                               pHeaderFileHandle   = pCodeGenInput->pHeaderFileHandle;

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

c_ocoa_code_generator_parameter c_ocoa_default_code_generator_parameter(void)
{
    c_ocoa_code_generator_parameter parameter = {};
    parameter.fopen = fopen;
    parameter.fclose = fclose;
    parameter.fflush = fflush;
    
    return parameter;
}

void objc_function_collection_reset( c_ocoa_objc_function_collection* pFunctionCollection )
{
    for( uint32_t functionIndex = 0u; functionIndex < pFunctionCollection->size; ++functionIndex )
    {
        free((char*)pFunctionCollection->pEntries[functionIndex].pFunctionName);
    }
    
    pFunctionCollection->size = 0u;
}

void c_ocoa_create_source_code_for_objc_method_collection( const c_ocoa_class_method_collection* pMethodCollection, c_ocoa_source_code_generator_input* pCodeGenInput )
{
    c_ocoa_create_source_code_for_objc_methods( MethodType_Instance, pMethodCollection->ppInstanceMethods, pMethodCollection->instanceMethodCount, pCodeGenInput );
    c_ocoa_create_source_code_for_objc_methods( MethodType_Class, pMethodCollection->ppClassMethods, pMethodCollection->classMethodCount, pCodeGenInput );

    objc_function_collection_reset( pCodeGenInput->pFunctionCollection );
}

void file_write_generated_comment( FILE* pFileHandle )
{
    fprintf( pFileHandle, "/*\n" );
    fprintf( pFileHandle, "\tThis file has been automatically generated by the shimmer industries c-ocoa API generator\n" );
    fprintf( pFileHandle, "\tThus, manual changes to this file will be lost if the file is re-generated.\n" );
    fprintf( pFileHandle, "*/\n\n" );
}

void file_write_c_type_header_prefix( FILE* pTypesFileHandle )
{
    file_write_generated_comment( pTypesFileHandle );
    fprintf( pTypesFileHandle, "#ifndef C_OCOA_TYPES_HEADER\n");
    fprintf( pTypesFileHandle, "#define C_OCOA_TYPES_HEADER\n\n");
    fprintf( pTypesFileHandle, "#include <stdbool.h>\n\n");
    fprintf( pTypesFileHandle, "typedef void*\tnsobject_t;\n" );
    fprintf( pTypesFileHandle, "typedef void*\tnsselector_t;\n" );
    fprintf( pTypesFileHandle, "typedef void*\tnsclass_t;\n\n" );
}

void file_write_c_type_header_suffix( FILE* pTypesFileHandle )
{
    fprintf( pTypesFileHandle, "#endif" );
}

void file_write_c_header_prefix( FILE* pHeaderFileHandle, const c_ocoa_objc_class_name* pClassName )
{
    file_write_generated_comment( pHeaderFileHandle );

    //FK: Header guard
    fprintf( pHeaderFileHandle, "#ifndef SHIMMER_C_OCOA_%s_HEADER\n#define SHIMMER_C_OCOA_%s_HEADER\n\n", pClassName->pNameUpper, pClassName->pNameUpper );
    fprintf( pHeaderFileHandle, "typedef void*\t%s_t;\n", pClassName->pNameLower );
    fprintf( pHeaderFileHandle, "#include \"c_ocoa_types.h\"\n\n");
}

void file_write_c_header_suffix( FILE* pHeaderFileHandle )
{
    //FK: End of header guard
    fprintf( pHeaderFileHandle, "#endif");
}

void file_write_c_source_prefix( FILE* pSourceFileHandle, const char* pHeaderFileName, const c_ocoa_objc_class_name* pClassName)
{
    file_write_generated_comment( pSourceFileHandle );
    fprintf( pSourceFileHandle, "#if defined(__OBJC__) && __has_feature(objc_arc)\n" );
    fprintf( pSourceFileHandle, "#define ARC_AVAILABLE\n" );
    fprintf( pSourceFileHandle, "#endif\n\n" );

    fprintf( pSourceFileHandle, "// ABI is a bit different between platforms\n" );
    fprintf( pSourceFileHandle, "#ifdef __arm64__\n" );
    fprintf( pSourceFileHandle, "#define abi_objc_msgSend_stret objc_msgSend\n" );
    fprintf( pSourceFileHandle, "#else\n" );
    fprintf( pSourceFileHandle, "#define abi_objc_msgSend_stret objc_msgSend_stret\n" );
    fprintf( pSourceFileHandle, "#endif\n" );
    fprintf( pSourceFileHandle, "#ifdef __i386__\n" );
    fprintf( pSourceFileHandle, "#define abi_objc_msgSend_fpret objc_msgSend_fpret\n" );
    fprintf( pSourceFileHandle, "#else\n" );
    fprintf( pSourceFileHandle, "#define abi_objc_msgSend_fpret objc_msgSend\n" );
    fprintf( pSourceFileHandle, "#endif\n\n" );
    fprintf( pSourceFileHandle, "#include \"%s\"\n\n", pHeaderFileName );
}

boolean8_t c_ocoa_create_source_code_for_objc_class( const c_ocoa_code_generator_parameter* pParameter, c_ocoa_objc_type_dictionary* pTypeDict, c_ocoa_objc_function_collection* pFunctionCollection, c_ocoa_string_allocator* pStringAllocator, Class pClass, const char* pClassName, int32_t classNameLength )
{
    if( pParameter->pClassNameFilter != NULL )
    {
        if( !string_name_matches_filter( pClassName, pParameter->pClassNameFilter ) )
        {
            return 0u;
        }
    }

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

    const char* pOutputPath = pParameter->pOutputPath == NULL ? "" : pParameter->pOutputPath;
    const char* pFilePrefix = pParameter->pPrefix == NULL ? "" : pParameter->pPrefix;
    const int32_t outputPathLength = string_get_length_excl_null_terminator( pOutputPath );
    const int32_t filePrefixLength = string_get_length_excl_null_terminator( pFilePrefix );

    //FK: +2 for file extension (.c/.h) +1 for null terminator
    char* pHeaderFileName = string_allocator_allocate( pStringAllocator, outputPathLength + filePrefixLength + classNameLength + 2 + 1 );
    char* pSourceFileName = string_allocator_allocate( pStringAllocator, outputPathLength + filePrefixLength + classNameLength + 2 + 1 );

    sprintf( pHeaderFileName, "%s%s%s.h", pOutputPath, pFilePrefix, className.pNameLower );
    sprintf( pSourceFileName, "%s%s%s.c", pOutputPath, pFilePrefix, className.pNameLower );

    FILE* pSourceFileHandle = pParameter->fopen( pSourceFileName, "w" );
    FILE* pHeaderFileHandle = pParameter->fopen( pHeaderFileName, "w" );

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
    
    pParameter->fclose( pHeaderFileHandle );
    pParameter->fclose( pSourceFileHandle );

    return 1u;
};

int c_ocoa_create_classes_api( const c_ocoa_code_generator_parameter* pCodeGeneratorParameter, c_ocoa_code_gen_context* pContext )
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

        c_ocoa_create_source_code_for_objc_class( pCodeGeneratorParameter, &pContext->typeDict, &pContext->functionCollection, &pContext->stringAllocator, pClass, pClassName, classNameLength );
        string_allocator_reset( &pContext->stringAllocator );
    }

    free( ppClasses );

    FILE* pTypesFileHandle = pCodeGeneratorParameter->fopen( "c_ocoa_types.h", "w" );
    if( pTypesFileHandle == NULL )
    {
        printf_stderr( "Could not open '%s' for writing.", "c_ocoa_types.h" );
        return 0;
    }

    objc_type_dict_resolve_struct_types( &pContext->typeDict, pTypesFileHandle );
    
    pCodeGeneratorParameter->fflush( pTypesFileHandle );
    pCodeGeneratorParameter->fclose( pTypesFileHandle );
    
    return 0;
}


boolean8_t objc_function_collection_contains_function( const c_ocoa_objc_function_collection* pFunctionCollection, const c_ocoa_objc_class_name* pClassName, const char* pFunctionName, const int32_t functionNameLength )
{
    //FK: Skip the 'classname_' part of the function name (to save an allocation in the calling code)
    //    eg: 'nsapplication_shutdown' -> skip 'nsapplication_'
    const int32_t functionCharSkipCount = pClassName->length;

    for( uint32_t functionIndex = 0u; functionIndex < pFunctionCollection->size; ++functionIndex )
    {
        const c_ocoa_objc_function_collection_entry* pFunctionNameEntry   = pFunctionCollection->pEntries + functionIndex;
        const int32_t functionBaseNameLength                    = pFunctionNameEntry->functionNameLength - functionCharSkipCount;
        assert_runtime( functionBaseNameLength > 0u );

        if( functionBaseNameLength == functionNameLength )
        {
            const char* pFunctionBaseName = pFunctionNameEntry->pFunctionName + functionCharSkipCount;
            if( string_is_equal( pFunctionBaseName, pFunctionName, functionNameLength ) )
            {
                return 1;
            }
        }
    }

    return 0;
}

void objc_function_collection_resize( c_ocoa_objc_function_collection* pFunctionCollection )
{
    const uint32_t newSize = pFunctionCollection->capacity * 2;
    c_ocoa_objc_function_collection_entry* pNewEntries = ( c_ocoa_objc_function_collection_entry* )malloc( sizeof( c_ocoa_objc_function_collection_entry ) * newSize );
    if( pNewEntries == NULL )
    {
        //FK: TODO: Error handling
        return;
    }

    memory_copy_non_overlapping( pNewEntries, pFunctionCollection->pEntries, pFunctionCollection->capacity * sizeof( c_ocoa_objc_function_collection_entry ) );
    pFunctionCollection->capacity = newSize;

    free( pFunctionCollection->pEntries );
    pFunctionCollection->pEntries = pNewEntries;
}

void objc_function_collection_add( c_ocoa_objc_function_collection* pFunctionCollection, const char* pFunctionName, const int32_t functionNameLength )
{
    const boolean8_t isFunctionCollectionFull = pFunctionCollection->capacity == pFunctionCollection->size;
    if( isFunctionCollectionFull )
    {
        objc_function_collection_resize( pFunctionCollection );
    }

    const uint32_t entryIndex = pFunctionCollection->size;
    pFunctionCollection->size++;

    pFunctionCollection->pEntries[ entryIndex ].pFunctionName       = pFunctionName;
    pFunctionCollection->pEntries[ entryIndex ].functionNameLength  = functionNameLength;
}

c_ocoa_convert_result objc_parse_result_convert_to_function_definition( c_ocoa_string_allocator* pStringAllocator, c_ocoa_objc_function_resolve_result* pOutFunctionResolveResult, c_ocoa_objc_function_collection* pFunctionCollection, c_ocoa_objc_type_dictionary* pDict, c_ocoa_parse_result* pParseResult, const c_ocoa_objc_class_name* pClassName )
{
    if( !function_name_is_valid( pParseResult->pFunctionName ) )
    {
        //FK: Add log here?
        return ConvertResult_InvalidFunctionName;
    }

    int32_t functionNameLength = pParseResult->functionNameLength;
    const char* pFunctionNameColonPosition = string_find_next_char_position( pParseResult->pFunctionName, ':' );
    if( pFunctionNameColonPosition != NULL )
    {
        functionNameLength = cast_size_to_int32( pFunctionNameColonPosition - pParseResult->pFunctionName );
    }

    if( objc_function_collection_contains_function( pFunctionCollection, pClassName, pParseResult->pFunctionName, functionNameLength ) )
    {
        return ConvertResult_AlreadyKnown;
    }

    const int32_t resolvedFunctionNameLength = functionNameLength + pClassName->length;
    char* pResolvedFunctionName = (char*)malloc( resolvedFunctionNameLength + 1u );
    if( pResolvedFunctionName == NULL )
    {
        return ConvertResult_OutOfMemory;
    }

    sprintf( pResolvedFunctionName, "%s_%.*s", pClassName->pNameLower, functionNameLength, pParseResult->pFunctionName );

    //FK: Will take ownership of pResolvedFunctionName
    objc_function_collection_add( pFunctionCollection, pResolvedFunctionName, resolvedFunctionNameLength );

    c_ocoa_objc_function_resolve_result resolveResult = {};
    resolveResult.pOriginalReturnType          = string_allocate_copy_with_allocator( pStringAllocator, pParseResult->pReturnType, pParseResult->returnTypeLength );
    resolveResult.pOriginalArgumentTypes       = string_allocate_copy_with_allocator( pStringAllocator, pParseResult->pArguments, pParseResult->argumentLength );
    resolveResult.pOriginalFunctionName        = string_allocate_copy_with_allocator( pStringAllocator, pParseResult->pFunctionName, pParseResult->functionNameLength );
    resolveResult.pResolvedFunctionName        = pResolvedFunctionName;

    c_ocoa_objc_type_resolve_result returnTypeResolveResult = {};
    if( !objc_resolve_c_type_name( &returnTypeResolveResult, pDict, pParseResult->pReturnType, pParseResult->returnTypeLength ) )
    {
        printf_stderr( "[error] Couldn't resolve return type '%.*s' of function '%s'.\n", pParseResult->returnTypeLength, pParseResult->pReturnType, resolveResult.pResolvedFunctionName );
        return ConvertResult_UnknownReturnType;
    }

    resolveResult.pResolvedReturnType = returnTypeResolveResult.pResolvedType;
    resolveResult.pClassName          = pClassName;

    const char* pArguments = pParseResult->pArguments;
    uint8_t argumentCount = 0u;
    const uint8_t maxArgumentCount = array_count( resolveResult.pResolvedArgumentTypes );
    const uint8_t argumentsToSkip = 2; //FK: Skip first two arguments since these are always the object + the selector.

    while( *pArguments )
    {
        const char* pArgumentStart = pArguments;
        const char* pArgumentEnd = whitespace_find_next( pArgumentStart );
        const int32_t argumentLength = cast_size_to_int32( pArgumentEnd - pArgumentStart );

        if( argumentCount >= argumentsToSkip )
        {
            const uint8_t argumentIndex = argumentCount - argumentsToSkip;
            assert_runtime( argumentIndex < maxArgumentCount );

            c_ocoa_objc_type_resolve_result argumentResolveResult = {};
            if( !objc_resolve_c_type_name( &argumentResolveResult, pDict, pArgumentStart, argumentLength ) )
            {
                printf_stderr( "[error] Couldn't resolve %d. argument type name '%.*s' of function '%s'.\n", argumentCount, argumentLength, pArgumentStart, resolveResult.pResolvedFunctionName );
                return ConvertResult_UnknownArgumentType;
            }

            resolveResult.pResolvedArgumentTypes[ argumentIndex ] = argumentResolveResult.pResolvedType;
        }

        ++argumentCount;
        if( *pArgumentEnd == 0 )
        {
            break;
        }

        pArguments = pArgumentEnd + 1;
    }

    resolveResult.argumentCount         = argumentCount < argumentsToSkip ? 0 : argumentCount - argumentsToSkip;
    resolveResult.isVoidFunction        = !returnTypeResolveResult.isReference && string_is_equal( resolveResult.pResolvedReturnType, "void", 5u );
    resolveResult.isAllocFunction       = strcmp( resolveResult.pOriginalFunctionName, "alloc" ) == 0u;
    resolveResult.hasStructReturnValue  = !returnTypeResolveResult.isReference && !returnTypeResolveResult.isBaseType;
    resolveResult.returnValueSizeInBits = returnTypeResolveResult.typeSizeInBits;
    resolveResult.hasFloatReturnValue   = returnTypeResolveResult.isFloatingType;
    resolveResult.methodType            = pParseResult->c_ocoa_method_type;

    *pOutFunctionResolveResult = resolveResult;

    return ConvertResult_Success;
}

void cfunction_write_signature( FILE* pResultFileHandle, const c_ocoa_objc_function_resolve_result* pFunctionDefinition )
{
    //FK: Function name
    fprintf( pResultFileHandle, "%s(", pFunctionDefinition->pResolvedFunctionName );

    //FK: Special handling for init functions - don't pass class object (will be passed internally for better API usage)
    if( pFunctionDefinition->methodType == MethodType_Instance && !pFunctionDefinition->isAllocFunction)
    {
        //FK: add object as first parameter instance methods
        fprintf( pResultFileHandle, " %s_t object", pFunctionDefinition->pClassName->pNameLower );

        for( uint8_t argumentIndex = 0u; argumentIndex < pFunctionDefinition->argumentCount; ++argumentIndex )
        {
            fprintf( pResultFileHandle, ", %s arg%u", pFunctionDefinition->pResolvedArgumentTypes[ argumentIndex ], argumentIndex );
        }
    }
    else
    {
        if( pFunctionDefinition->argumentCount > 0u )
        {
            fprintf( pResultFileHandle, " %s arg%u", pFunctionDefinition->pResolvedArgumentTypes[ 0 ], 0 );

            for( uint8_t argumentIndex = 1u; argumentIndex < pFunctionDefinition->argumentCount; ++argumentIndex )
            {   
                fprintf( pResultFileHandle, ", %s arg%u", pFunctionDefinition->pResolvedArgumentTypes[ argumentIndex ], argumentIndex );
            }
        }
    }

    fprintf( pResultFileHandle, " )");
}

void cfunction_write_declaration( FILE* pResultFileHandle, const c_ocoa_objc_function_resolve_result* pFunctionDefinition )
{   
    //FK: "guess" maximum tab count for nice formatting
    //    getting the correct tab count for the longest
    //    return type name would be too involved since
    //    we currently run on a function by function basis
    //    and thus don't know the maximum type name length
    //    beforehand
    const int32_t maxTabCount = 5u;
    int32_t returnTypeLength = 0u;

    fprintf(pResultFileHandle, "// Signature from Objective-C Runtime: %s %s %s\n", pFunctionDefinition->pOriginalReturnType, pFunctionDefinition->pOriginalFunctionName, pFunctionDefinition->pOriginalArgumentTypes );

    if( pFunctionDefinition->isAllocFunction )
    {
        //FK: little syntactic sugar, return correct type for init function(s)
        returnTypeLength = fprintf( pResultFileHandle, "%s_t ", pFunctionDefinition->pClassName->pNameLower );
    }
    else
    {
        returnTypeLength = fprintf( pResultFileHandle, "%s ", pFunctionDefinition->pResolvedReturnType );
    }

    const int32_t spacesForTab = 4u;
    const int32_t tabCountForReturnType = get_max( 0u, maxTabCount - ( returnTypeLength / spacesForTab ) );
    for( int32_t tabIndex = 0u; tabIndex < tabCountForReturnType; ++tabIndex )
    {
        fprintf( pResultFileHandle, "\t" );
    }

    cfunction_write_signature( pResultFileHandle, pFunctionDefinition );

    fprintf( pResultFileHandle, ";\n\n" );
    fflush( pResultFileHandle );
}

const char* objc_find_msgsend_call( const c_ocoa_objc_function_resolve_result* pFunctionResolveResult )
{
    //FK: This is sort of handwavy after some trial & error
    if( pFunctionResolveResult->hasFloatReturnValue )
    {
        return "abi_objc_msgSend_fpret";
    }
    if( pFunctionResolveResult->hasStructReturnValue && pFunctionResolveResult->returnValueSizeInBits > 128 )
    {
        return "abi_objc_msgSend_stret";
    }

    return "objc_msgSend";
}

void file_write_c_function_implementation( FILE* pSourceFileHandle, const c_ocoa_objc_function_resolve_result* pFunctionResolveResult, const c_ocoa_objc_class_name* pClassName )
{   
    size_t returnTypeLength = 0u;
    if( pFunctionResolveResult->isAllocFunction )
    {
        //FK: little syntactic sugar, return correct type for init function(s)
        returnTypeLength = fprintf( pSourceFileHandle, "%s_t ", pClassName->pNameLower );
    }
    else
    {
        returnTypeLength = fprintf( pSourceFileHandle, "%s ", pFunctionResolveResult->pResolvedReturnType );
    }

    cfunction_write_signature( pSourceFileHandle, pFunctionResolveResult );

    fprintf( pSourceFileHandle, "\n{\n" );

    fprintf( pSourceFileHandle, "\tSEL methodSelector = sel_registerName( \"%s\" );\n", pFunctionResolveResult->pOriginalFunctionName );

    if( pFunctionResolveResult->methodType == MethodType_Class )
    {
        fprintf( pSourceFileHandle, "\tClass internalClassObject = objc_getClass( \"%s\" );\n", pClassName->pName );
    }

    fprintf( pSourceFileHandle, "\t#define %s_call( obj, selector", pFunctionResolveResult->pResolvedFunctionName );
    for( uint8_t argumentIndex = 0u; argumentIndex < pFunctionResolveResult->argumentCount; ++argumentIndex )
    {
        fprintf( pSourceFileHandle, ", arg%hhu", argumentIndex );
    }

    fprintf( pSourceFileHandle, " ) ((%s (*)( id, SEL", pFunctionResolveResult->pResolvedReturnType );
    for( uint8_t argumentIndex = 0u; argumentIndex < pFunctionResolveResult->argumentCount; ++argumentIndex )
    {
        const char* pArgumentType = pFunctionResolveResult->pResolvedArgumentTypes[ argumentIndex ];
        fprintf( pSourceFileHandle, ", %s", pArgumentType );
    }

    const char* msgSendCall = objc_find_msgsend_call( pFunctionResolveResult );
    fprintf( pSourceFileHandle, " ))%s) ( obj, selector", msgSendCall );
    for( uint8_t argumentIndex = 0u; argumentIndex < pFunctionResolveResult->argumentCount; ++argumentIndex )
    {
        fprintf( pSourceFileHandle, ", arg%hhu", argumentIndex );
    }
    fprintf( pSourceFileHandle, " )\n");

    fprintf( pSourceFileHandle, "\t" );
    const boolean8_t hasReturnValue = !pFunctionResolveResult->isVoidFunction;
    if( hasReturnValue )
    {
        fprintf( pSourceFileHandle, "return " );
    }

    switch( pFunctionResolveResult->methodType )
    {
        case MethodType_Class:
            fprintf( pSourceFileHandle, "%s_call( (id)internalClassObject, methodSelector", pFunctionResolveResult->pResolvedFunctionName );
            break;

        case MethodType_Instance:
            fprintf( pSourceFileHandle, "%s_call( (id)object, methodSelector", pFunctionResolveResult->pResolvedFunctionName );
            break;
    }

    for( uint8_t argumentIndex = 0u; argumentIndex < pFunctionResolveResult->argumentCount; ++argumentIndex )
    {
        fprintf( pSourceFileHandle, ", arg%hhu", argumentIndex );
    }
    fprintf( pSourceFileHandle, " );\n" );
    fprintf( pSourceFileHandle, "\t#undef %s_call\n", pFunctionResolveResult->pResolvedFunctionName );
    fprintf( pSourceFileHandle, "}\n\n");
    
    fflush( pSourceFileHandle );
}

boolean8_t objc_create_class_name( c_ocoa_objc_class_name* pOutClassName, const char* pClassNameStart, const int32_t classNameLength )
{
    pOutClassName->length       = classNameLength;
    pOutClassName->pName        = string_allocate_copy( pClassNameStart, classNameLength );
    pOutClassName->pNameLower   = string_allocate_copy_lower( pClassNameStart, classNameLength );
    pOutClassName->pNameUpper   = string_allocator_copy_upper( pClassNameStart, classNameLength );

    if( pOutClassName->pName == NULL || pOutClassName->pNameLower == NULL || pOutClassName->pNameUpper == NULL )
    {
        free( pOutClassName->pName );
        free( pOutClassName->pNameUpper );
        free( pOutClassName->pNameLower );
        return 0;
    }

    return 1;
}

void file_write_c_type_forward_declarations( FILE* pTypesFileHandle, c_ocoa_objc_type_dictionary* pTypeDict )
{
    fprintf( pTypesFileHandle, "// Forward declarations:\n" );
    for( uint32_t typeEntryIndex = 0u; typeEntryIndex < pTypeDict->typeEntryCapacity; ++typeEntryIndex )
    {
        const c_ocoa_objc_type_dictionary_entry* pEntry = pTypeDict->ppTypeEntries[typeEntryIndex];
        while( pEntry != NULL )
        {
            const c_ocoa_objc_type_resolve_result* pResolveResult = &pEntry->resolveResult;
            if( !pResolveResult->isReference )
            {
                goto loop_end;
            }

            if( pResolveResult->isBaseType )
            {
                goto loop_end;
            }

            const int32_t resolvedTypeLength = pResolveResult->resolvedTypeLength;
            fprintf( pTypesFileHandle, "struct _%.*s_;\n", resolvedTypeLength, pResolveResult->pResolvedType );
            fprintf( pTypesFileHandle, "typedef struct _%.*s_ %.*s;\n", resolvedTypeLength, pResolveResult->pResolvedType, resolvedTypeLength, pResolveResult->pResolvedType  );
            
        loop_end:
            pEntry = (c_ocoa_objc_type_dictionary_entry*)pEntry->pNext;
        }
 
    }

    fprintf( pTypesFileHandle, "\n" );
}

void file_write_c_type_single_type_struct_definition( FILE* pTypesFileHandle, c_ocoa_objc_type_dictionary_entry* pStructEntry, c_ocoa_objc_type_dictionary* pTypeDict, const c_ocoa_objc_type_resolve_result* pObjCTypeDefinition )
{
    assert_runtime( !pStructEntry->declarationWasWritten );
    pStructEntry->declarationWasWritten = 1;

    const int32_t structResolveBufferCapacity = 2048;
    int32_t structResolveBufferLength   = 0u;
    char* pStructResolveBuffer          = (char*)alloca( structResolveBufferCapacity );

    char* pCurrentStructResolveBufferPosition = pStructResolveBuffer;

    const char* pCurrentStructMember = string_find_next_char_position( pObjCTypeDefinition->pOriginalType, '=' ) + 1; //FK: +1 to skip '='
    assert_runtime( pCurrentStructMember != NULL );

    const char* pStructEnd = string_find_last_position( pCurrentStructMember, '}' );
    assert_runtime( pStructEnd != NULL );

    boolean8_t skipStruct = 0;
    uint32_t structMemberCount = 0u;
    while( 1 )
    {
        int32_t structMemberTypeLength = 1;
        const boolean8_t isStructType = objc_is_struct_type( pCurrentStructMember );
        if( isStructType )
        {
            const char* pEndCurrentStructMember = string_find_next_char_position( pCurrentStructMember, '}' );
            structMemberTypeLength = cast_size_to_int32( pEndCurrentStructMember - pCurrentStructMember ) + 1; //FK: +1 to include '}' as part of the name
        }

        c_ocoa_objc_type_resolve_result resolveResult = {};
        if( !objc_resolve_c_type_name( &resolveResult, pTypeDict, pCurrentStructMember, structMemberTypeLength ) )
        {
            printf_stderr( "[error] skipped struct generation of struct '%s' because struct member of type '%c' couldn't be resolved.\n", pObjCTypeDefinition->pResolvedType, *pCurrentStructMember );
            skipStruct = 1;
            break;
        }

        if( isStructType )
        {   
            const char* pStructName = NULL;
            int32_t structNameLength = 0u;
            struct_extract_name( &pStructName, &structNameLength, pCurrentStructMember );

            c_ocoa_objc_type_dictionary_entry* pEntry = objc_type_dict_find_entry( pTypeDict, pStructName, structNameLength );
            if( !pEntry->declarationWasWritten )
            {
                //FK: Type is not known yet, write first before continuing
                file_write_c_type_single_type_struct_definition( pTypesFileHandle, pEntry, pTypeDict, &resolveResult );
            }
        }

        const uint32_t structMemberLength = resolveResult.resolvedTypeLength + 13;
        if( structResolveBufferLength + structMemberLength >= structResolveBufferCapacity )
        {
            printf_stderr( "[error] skipped struct generation of struct '%s' we've run out of memory while trying to assemble the struct.\n", pObjCTypeDefinition->pResolvedType );
            skipStruct = 1;
            break;
        }

        const int32_t characterWritten = sprintf( pCurrentStructResolveBufferPosition, "\t%s member%d;", resolveResult.pResolvedType, structMemberCount );

        pCurrentStructMember        += structMemberTypeLength;
        structResolveBufferLength   += characterWritten;

        if( pCurrentStructMember == pStructEnd )
        {
            break;
        }

        pCurrentStructResolveBufferPosition += characterWritten;

        sprintf( pCurrentStructResolveBufferPosition, "\n" );
        ++pCurrentStructResolveBufferPosition;
        ++structResolveBufferLength;
        ++structMemberCount;
    }

    if( !skipStruct )
    {
        fprintf( pTypesFileHandle, "//Original struct: %s\n", pObjCTypeDefinition->pOriginalType );
        fprintf( pTypesFileHandle, "typedef struct _%s_\n{\n%.*s\n} %s;\n\n", pObjCTypeDefinition->pResolvedType, structResolveBufferLength, pStructResolveBuffer, pObjCTypeDefinition->pResolvedType );
    }
}

void file_write_c_type_struct_definitions( FILE* pTypesFileHandle, c_ocoa_objc_type_dictionary* pTypeDict )
{
    for( uint32_t typeEntryIndex = 0u; typeEntryIndex < pTypeDict->typeEntryCapacity; ++typeEntryIndex )
    {
        c_ocoa_objc_type_dictionary_entry* pEntry = pTypeDict->ppTypeEntries[typeEntryIndex];
        while( pEntry != NULL )
        {
            if( pEntry->declarationWasWritten )
            {
                goto loop_end;
            }

            const c_ocoa_objc_type_resolve_result* pResolveResult = &pEntry->resolveResult;
            if( pResolveResult->isReference )
            {
                goto loop_end;
            }

            if( pResolveResult->isBaseType )
            {
                goto loop_end;
            }

            file_write_c_type_single_type_struct_definition( pTypesFileHandle, pEntry, pTypeDict, pResolveResult );
            
        loop_end:
            pEntry = (c_ocoa_objc_type_dictionary_entry*)pEntry->pNext;
        }
    }
}

void objc_type_dict_resolve_struct_types( c_ocoa_objc_type_dictionary* pTypeDict, FILE* pTypesFileHandle )
{
    //FK: Write struct references first:
    file_write_c_type_header_prefix( pTypesFileHandle );

    file_write_c_type_forward_declarations( pTypesFileHandle, pTypeDict );
    file_write_c_type_struct_definitions( pTypesFileHandle, pTypeDict );

    //FK: Now write struct types:
    file_write_c_type_header_suffix( pTypesFileHandle );
}

boolean8_t c_ocoa_create_code_gen_context( c_ocoa_code_gen_context* pCodeGenContext )
{
    if( !objc_type_dict_create( &pCodeGenContext->typeDict ) )
    {
        printf_stderr( "[error] Out of memory while trying to create objective c type dictionary." );
        return 0;
    }

    if( !objc_function_collection_create( &pCodeGenContext->functionCollection ) )
    {
        printf_stderr( "[error] Out of memory while trying to create objective c function collection." );
        return 0;
    }

    const size_t stringAllocatorSizeInBytes = 1024*1024; //FK: 1 MiB, quite a lot for a string allocator but better be safe than sorry
    if( !string_allocator_create( &pCodeGenContext->stringAllocator, stringAllocatorSizeInBytes ) )
    {
        printf_stderr( "Out of memory while trying to create string allocator of size %.3fKiB.", (float)stringAllocatorSizeInBytes/1024.f );
        return 0;
    }
    
    return 1;
}

#endif //C_OCOA_GENERATOR_HEADER
