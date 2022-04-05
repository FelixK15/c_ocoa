#ifndef C_OCOA_API_GENERATOR_HEADER
#define C_OCOA_API_GENERATOR_HEADER

typedef enum
{
    ConvertResult_Success = 0,
    ConvertResult_OutOfMemory,
    ConvertResult_InvalidFunctionName,
    ConvertResult_UnknownReturnType,
    ConvertResult_UnknownArgumentType,
    ConvertResult_AlreadyKnown
} ConvertResult;

typedef enum
{
    MethodType_Class = 0,
    MethodType_Instance
} MethodType;

typedef uint8_t boolean8_t;

typedef struct 
{
    int32_t length;
    char* pName;
    char* pNameLower;
    char* pNameUpper;
} ObjCClassName;

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
} ObjCTypeResolveResult;

typedef struct
{
    const char* pResolvedReturnType;
    const char* pResolvedArgumentTypes[32];

    const ObjCClassName*    pClassName;
    char*                   pResolvedFunctionName;
    char*                   pOriginalFunctionName;
    char*                   pOriginalReturnType;
    char*                   pOriginalArgumentTypes;
    
    MethodType              methodType;

    uint32_t                returnValueSizeInBits;
    uint8_t                 argumentCount           : 4;
    boolean8_t              isVoidFunction          : 1;
    boolean8_t              isAllocFunction         : 1;
    boolean8_t              hasStructReturnValue    : 1;
    boolean8_t              hasFloatReturnValue     : 1;
} ObjCFunctionResolveResult;

typedef struct
{
    char*                   pHashParameter; //FK: Type name;
    ObjCTypeResolveResult   resolveResult;
    boolean8_t              isNew;
    boolean8_t              declarationWasWritten;
} ObjCTypeDictEntry;

typedef struct
{
    char*                       pHashParameter; //FK: Function name;
    ObjCFunctionResolveResult   resolveResult;
    boolean8_t                  isNew;
} ObjCFunctionDictEntry;

typedef struct
{
    ObjCFunctionDictEntry*  pFunctionEntries;
    ObjCTypeDictEntry*      pTypeEntries;
    uint32_t                typeEntryCapacity;
    uint32_t                functionEntryCapactiy;
} ObjCTypeDict;

typedef struct
{
    const char* pTestFilePath;
} CommandLineParseResult;

typedef struct
{
    FILE* pHeaderFileHandle;
    FILE* pSourceFileHandle;

    const char* pHeaderFileName;
    const char* pSourceFileName;
} ObjCConversionArguments;

typedef struct
{
    char*       pReturnType;
    char*       pFunctionName;
    char*       pArguments;

    int32_t     returnTypeLength;
    int32_t     functionNameLength;
    int32_t     argumentLength;

    MethodType  methodType;
} ParseResult;

typedef struct 
{
    char* pBufferStart;
    uint32_t bufferCapacity;
    uint32_t bufferSize;
} StringAllocator;

typedef struct
{
    int32_t     functionNameLength;
    const char* pFunctionName;
} ObjCFunctionCollectionEntry;

typedef struct 
{
    uint32_t                        capacity;
    uint32_t                        size;
    ObjCFunctionCollectionEntry*    pEntries;
} ObjCFunctionCollection;

//FK: Some helpful macros
#define printf_stderr(x, ...)   fprintf( stderr, x, __VA_ARGS__ )
#define ArrayCount(x)           (sizeof(x)/sizeof(x[0]))
#define getMax(a,b)             (a)>(b)?(a):(b)
#define getMin(a,b)             (a)>(b)?(b):(a)
#define UnusedArgument(x)       (void)x

void resetStringAllocator( StringAllocator* pStringAllocator )
{
    pStringAllocator->bufferSize = 0u;
}

char* getCurrentStringAllocatorBase( StringAllocator* pStringAllocator )
{
    return pStringAllocator->pBufferStart + pStringAllocator->bufferSize;
}

void decrementStringAllocatorCapacity( StringAllocator* pStringAllocator, uint32_t sizeInBytes )
{
    RuntimeAssert( pStringAllocator->bufferSize + sizeInBytes < pStringAllocator->bufferCapacity );
    pStringAllocator->bufferSize += sizeInBytes;
}

uint32_t getRemainingStringAllocatorCapacity( StringAllocator* pStringAllocator )
{
    return pStringAllocator->bufferCapacity - pStringAllocator->bufferSize;
}

boolean8_t createConversionArguments( ObjCConversionArguments* pOutArguments, const char* pHeaderFileName, const char* pSourceFileName )
{
    pOutArguments->pHeaderFileName = pHeaderFileName;
    pOutArguments->pSourceFileName = pSourceFileName;

    pOutArguments->pSourceFileHandle = fopen( pSourceFileName, "w" );
    if( pOutArguments->pSourceFileHandle == NULL )
    {
        printf("Couldn't open '%s' for writing.\n", pSourceFileName );
        return 0u;
    }

    pOutArguments->pHeaderFileHandle = fopen( pHeaderFileName, "w" );
    if( pOutArguments->pHeaderFileHandle == NULL )
    {
        fclose( pOutArguments->pSourceFileHandle );
        printf("Couldn't open '%s' for writing.\n", pSourceFileName );
        return 0u;
    }

    return 1u;
}

static inline uint32_t castSizeToUint32( size_t val )
{
    RuntimeAssert( val < UINT32_MAX );
    return (uint32_t)val;
}

static inline int32_t castSizeToInt32( size_t val )
{
    RuntimeAssert( val < INT32_MAX );
    return (int32_t)val;
}

static inline void fillMemoryWithZeroes( void* pMemory, const size_t sizeInBytes )
{
    //FK: TODO: use STOSB - currently this is slower than memset (but saves the string.h include)
    uint8_t* pMemoryAsByteArray = (uint8_t*)pMemory;
    for( size_t byteIndex = 0u; byteIndex < sizeInBytes; ++byteIndex )
    {
        *pMemoryAsByteArray = 0;
    }
}

//FK: Use custom string.h alternatives since we're not concerned with locale 
static inline boolean8_t isWhiteSpaceCharacter( const char character )
{
    return character == ' '  ||
           character == '\n' ||
           character == '\t' ||
           character == '\v' ||
           character == '\f' ||
           character == '\r';
}

static inline boolean8_t isAlphabeticalCharacter( const char character )
{
    return ( character >= 'A' && character <= 'Z' ) || ( character >= 'a' && character <= 'z' );
}

static inline char convertCharacterToLower( const char character )
{
    if( !isAlphabeticalCharacter( character ) )
    {
        return character;
    }

    return character <= 'Z' ? character + 32 : character;
}

static inline char convertCharacterToUpper( const char character )
{
    if( !isAlphabeticalCharacter( character ) )
    {
        return character;
    }

    return character >= 'a' ? character - 32 : character;
}

static inline boolean8_t areCStringsEqual( const char* pStringA, const char* pStringB, const int32_t stringLength )
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

static inline int32_t getCStringLengthExclNullTerminator( const char* pString )
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

    return castSizeToInt32( pString - pStringStart );
}

static inline int32_t getCStringLengthInclNullTerminator( const char* pString )
{
    const char* pStringStart = pString;
    while( *pString++ );

    return castSizeToInt32( pString - pStringStart );
}

static inline char* copyCStringAndAddNullTerminator( char* pDestination, const char* pSource, const int32_t stringLength )
{
    for( int32_t charIndex = 0; charIndex < stringLength; ++charIndex )
    {
        pDestination[ charIndex ] = pSource[ charIndex ];
    }

    pDestination[ stringLength ] = 0;
    return pDestination;
}

static inline char* convertCStringToLowerInplace( char* pString, const int32_t stringLength )
{
    for( int32_t charIndex = 0u; charIndex < stringLength; ++charIndex )
    {
        pString[ charIndex ] = convertCharacterToLower( pString[ charIndex ] );
    }
    return pString;
}

static inline char* convertCStringToLower( char* pDestination, const char* pSource, const int32_t sourceLength )
{
    for( int32_t charIndex = 0u; charIndex < sourceLength; ++charIndex )
    {
        pDestination[ charIndex ] = convertCharacterToLower( pSource[ charIndex ] );
    }

    return pDestination;
}

static inline char* convertCStringToUpperInplace( char* pString, const int32_t stringLength )
{
    for( int32_t charIndex = 0u; charIndex < stringLength; ++charIndex )
    {
        pString[ charIndex ] = convertCharacterToUpper( pString[ charIndex ] );
    }
    return pString;
}

static inline char* convertCStringToUpper( char* pDestination, const char* pSource, const int32_t sourceLength )
{
    for( int32_t charIndex = 0u; charIndex < sourceLength; ++charIndex )
    {
        pDestination[ charIndex ] = convertCharacterToUpper( pSource[ charIndex ] );
    }

    return pDestination;
}


static inline char* allocateCStringCopy( const char* pString, const int32_t stringLength )
{
    char* pStringCopyMemory = (char*)malloc( stringLength + 1 );
    if( pStringCopyMemory == NULL )
    {
        //FK: TODO: Handle out of memory
        return NULL;
    }

    return copyCStringAndAddNullTerminator( pStringCopyMemory, pString, stringLength );
}

static inline char* allocateLowerCStringCopy( const char* pString, const int32_t stringLength )
{
    char* pStringCopyMemory = (char*)malloc( stringLength + 1 );
    if( pStringCopyMemory == NULL )
    {
        //FK: TODO: Handle out of memory
        return NULL;
    }

    copyCStringAndAddNullTerminator( pStringCopyMemory, pString, stringLength );
    return convertCStringToLowerInplace( pStringCopyMemory, stringLength );
}

static inline char* allocateUpperCStringCopy( const char* pString, const int32_t stringLength )
{
    char* pStringCopyMemory = (char*)malloc( stringLength + 1 );
    if( pStringCopyMemory == NULL )
    {
        //FK: TODO: Handle out of memory
        return NULL;
    }

    copyCStringAndAddNullTerminator( pStringCopyMemory, pString, stringLength );
    return convertCStringToUpperInplace( pStringCopyMemory, stringLength );
}

static inline char* allocateCStringCopyWithAllocator( StringAllocator* pAllocator, const char* pString, const int32_t stringLength )
{
    char* pStringCopyMemory = getCurrentStringAllocatorBase( pAllocator );
    decrementStringAllocatorCapacity( pAllocator, stringLength + 1 );

    return copyCStringAndAddNullTerminator( pStringCopyMemory, pString, stringLength );
}

static inline char* allocateCStringWithAllocator( StringAllocator* pAllocator, const int32_t stringLength )
{
    char* pStringMemory = getCurrentStringAllocatorBase( pAllocator );
    decrementStringAllocatorCapacity( pAllocator, stringLength + 1);
    return pStringMemory;
}

static inline const char* findLastCharacterPositionInCString( const char* pString, char character )
{
    const char* pLastOccurance = NULL;
    const char* pStringStart = pString;
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

static inline const char* findNextCharacterPositionInCString( const char* pString, char character )
{
    const char* pStringStart = pString;
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

static inline boolean8_t isStdStringType( const char* pTypeName )
{
    //FK: TODO
    ++pTypeName; //FK: Eat leading '{'
    return 0u;
    //return areCStringsEqual( pTypeName, "basic_string<char");
}

CommandLineParseResult parseCommandLineArguments( const int argc, const char** argv )
{
    typedef enum
    {
        NextArgument,
        ParseTestArg
    } ParseState;

    ParseState state = NextArgument;
    CommandLineParseResult result;

    for( int argIndex = 0u; argIndex < argc; ++argIndex )
    {
        switch( state )
        {
            case NextArgument:
            {
                if( areCStringsEqual( argv[argIndex], "--test", 6u ) )
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
                InvalidCodePath();
                break;
            }
        }
    }

    return result;
}

boolean8_t createObjectiveCTypeDictionary( ObjCTypeDict* pOutTypeDict, const size_t typeDictSizeInBytes, const size_t functionDictSizeInBytes )
{
    //FK: TODO: accept 'entry dict size' & 'function dict size' arguments
    const size_t totalSizeInBytes = typeDictSizeInBytes + functionDictSizeInBytes;
    uint8_t* pDictionaryMemory = (uint8_t*)malloc( totalSizeInBytes );
    if( pDictionaryMemory == NULL )
    {
        return 0;
    }

    const uint32_t typeEntryCount       = typeDictSizeInBytes / sizeof( ObjCTypeDictEntry );
    const uint32_t functionEntryCount   = functionDictSizeInBytes / sizeof( ObjCFunctionDictEntry );

    pOutTypeDict->pFunctionEntries         = (ObjCFunctionDictEntry*)pDictionaryMemory;
    pOutTypeDict->pTypeEntries             = (ObjCTypeDictEntry*)( pDictionaryMemory + typeDictSizeInBytes  );
    pOutTypeDict->typeEntryCapacity        = typeEntryCount;
    pOutTypeDict->functionEntryCapactiy    = functionEntryCount;

    //FK: mark all entries as `isNew`
    for( size_t entryIndex = 0u; entryIndex < pOutTypeDict->typeEntryCapacity; ++entryIndex )
    {
        pOutTypeDict->pTypeEntries[ entryIndex ].isNew = 1;
    }

    for( size_t entryIndex = 0u; entryIndex < pOutTypeDict->functionEntryCapactiy; ++entryIndex )
    {
        pOutTypeDict->pFunctionEntries[ entryIndex ].isNew = 1;
    }

    return 1;
}

boolean8_t createObjectiveCFunctionCollection( ObjCFunctionCollection* pOutFunctionCollection )
{
    pOutFunctionCollection->capacity = 16u;
    pOutFunctionCollection->size = 0u;
    pOutFunctionCollection->pEntries = ( ObjCFunctionCollectionEntry* )malloc( sizeof( ObjCFunctionCollectionEntry ) * 16u );

    if( pOutFunctionCollection->pEntries == NULL )
    {
        return 0;
    }

    return 1;
}

boolean8_t stringsAreEqual( const char* pStringA, const char* pStringB, const int32_t stringLength )
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

uint32_t sdbm_hash( const char* pString, const int32_t stringLength )
{
    uint32_t hash = 0;

    for( int32_t charIndex = 0u; charIndex < stringLength; ++charIndex )
    {
        hash = pString[ charIndex ] + ( hash << 6 ) + ( hash << 16 ) - hash;
    }

    return hash;
    
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

ObjCTypeDictEntry* findTypeDictionaryEntry( ObjCTypeDict* pDict, const char* pTypeName, const int32_t typeNameLength )
{
    const uint32_t hashValue = djb2_hash( pTypeName, typeNameLength );
    const uint32_t entryIndex = hashValue % pDict->typeEntryCapacity;
    ObjCTypeDictEntry* pEntry = pDict->pTypeEntries + entryIndex;
    return pEntry->isNew ? NULL : pEntry;
}

#if 0
ObjCFunctionDictEntry* insertFunctionDictEntry( ObjCTypeDict* restrict_modifier pDict, const char* restrict_modifier pFunctionName, const int32_t functionNameLength, boolean8_t* restrict_modifier pOutIsNew )
{
    //FK: TODO: Check hash distribution
    const uint32_t hashValue = sdbm_hash( pFunctionName, functionNameLength );
    const uint32_t entryIndex = hashValue % pDict->functionEntryCapactiy;
    ObjCFunctionDictEntry* pEntry = pDict->pFunctionEntries + entryIndex;

    *pOutIsNew = pEntry->isNew;
    if( pEntry->isNew )
    {
        pEntry->pHashParameter = (char*)malloc( functionNameLength + 1 );
        if( pEntry->pHashParameter == NULL )
        {
            //FK: out of memory.
            //FK: TODO: Print message, let the user know what's going on
            return NULL;
        }

        sprintf( pEntry->pHashParameter, "%.*s", functionNameLength, pFunctionName );
        pEntry->isNew = 0;
    }

    //FK: Check for hash collision
    RuntimeAssert( stringsAreEqual( pEntry->pHashParameter, pFunctionName, functionNameLength ) );

    return pEntry;
}
#endif

ObjCTypeDictEntry* insertTypeDictionaryEntry( ObjCTypeDict* restrict_modifier pDict, const char* restrict_modifier pTypeName, const int32_t typeNameLength, boolean8_t* restrict_modifier pOutIsNew )
{
    //FK: TODO: Check hash distribution
    const uint32_t hashValue = djb2_hash( pTypeName, typeNameLength );
    const uint32_t entryIndex = hashValue % pDict->typeEntryCapacity;
    ObjCTypeDictEntry* pEntry = pDict->pTypeEntries + entryIndex;

    *pOutIsNew = pEntry->isNew;
    if( pEntry->isNew )
    {
        //FK: TODO: use custom linear allocator?
        pEntry->pHashParameter = (char*)malloc( typeNameLength + 1 ); 
        if( pEntry->pHashParameter == NULL )
        {
            //FK: out of memory.
            //FK: TODO: Print message, let the user know what's going on
            return NULL;
        }
        sprintf( pEntry->pHashParameter, "%.*s", typeNameLength, pTypeName );
        pEntry->isNew = 0;
    }

    //FK: Check for hash collision
    RuntimeAssert( stringsAreEqual( pEntry->pHashParameter, pTypeName, typeNameLength ) );

    return pEntry;
}

const char* parseObjectiveCStruct( const char* pTypeName, const int32_t typeNameLength )
{
    char* pTemp = (char*)malloc( typeNameLength + 1 );
    sprintf( pTemp, "%.*s", typeNameLength, pTypeName );
    return pTemp;

    UnusedArgument(pTypeName);
    UnusedArgument(typeNameLength);

    //FK: Optimistically alloca 1KiB on the stack to have 
    //    a big enough scratchpad to resolve the typename
    char* pTempMemory = (char*)alloca(1024);

    //FK: TODO!
    return pTypeName;
}

const char* parseObjectiveCType( const char* pTypeName, const int32_t typeNameLength )
{
    char* pTemp = (char*)malloc( typeNameLength + 1 );
    sprintf( pTemp, "%.*s", typeNameLength, pTypeName );
    return pTemp;
}

int32_t getTypenameReferencePosition( const char* pTypeName, const int32_t typenameLength )
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

boolean8_t isObjectiveCStructType( const char* pTypeName )
{
    return *pTypeName == '{';
}

boolean8_t isObjectiveCReference( const char* pTypeName, const int32_t typeNameLength )
{
    const int32_t referencePosition = getTypenameReferencePosition( pTypeName, typeNameLength );
    return referencePosition != INT32_MAX;
}

boolean8_t resolveBaseType( ObjCTypeResolveResult* pOutResult, const char* pObjectiveCTypeName )
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

    const int32_t resolvedTypeLength = getCStringLengthExclNullTerminator( pResolvedBaseType );
    pOutResult->isReference              = ( *pObjectiveCTypeName == '*' );
    pOutResult->isBaseType               = 1;
    pOutResult->isFloatingType           = isFloatingType;
    pOutResult->originalTypeLength       = 1;
    pOutResult->pResolvedType            = allocateCStringCopy( pResolvedBaseType, resolvedTypeLength );
    pOutResult->pOriginalType            = allocateCStringCopy( pObjectiveCTypeName, 1 );
    pOutResult->resolvedTypeLength       = resolvedTypeLength;
    pOutResult->typeSizeInBits           = typeSizeInBits;
    return 1;
}

boolean8_t resolveBaseTypeIntoTypeDictionary( ObjCTypeResolveResult* pOutResult, ObjCTypeDict* pDict, const char* pObjectiveCTypeName )
{
    boolean8_t isNewDictEntry = 0;
    ObjCTypeDictEntry* pDictEntry = insertTypeDictionaryEntry( pDict, pObjectiveCTypeName, 1, &isNewDictEntry );
    if( !isNewDictEntry )
    {
        *pOutResult = pDictEntry->resolveResult;
        return 1;
    }

    const boolean8_t resolvedSuccessful = resolveBaseType( pOutResult, pObjectiveCTypeName );
    if( !resolvedSuccessful )
    {
        pDictEntry->isNew = 1;
        return 0;
    }

    pDictEntry->resolveResult = *pOutResult;
    return 1;
}

boolean8_t resolveStructTypeIntoTypeDictionary( ObjCTypeResolveResult* pOutResult, ObjCTypeDict* pDict, const char* pTypeName, int32_t typeNameLength );

boolean8_t resolveStructType( ObjCTypeResolveResult* pOutResult, ObjCTypeDict* pDict, const char* pTypeName, int32_t typeNameLength )
{
    const char* pTypeDefinitionStart        = pTypeName;
    const char* pTypeNameStart              = pTypeName + 1; //FK: +1 to skip initial '{' of struct definition
    const char* pTypeNameEnd                = findNextCharacterPositionInCString( pTypeDefinitionStart, '=' );
    const int32_t resolvedTypeNameLength    = castSizeToInt32( pTypeNameEnd - pTypeNameStart );

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
            const char* pEmbeddedTypeNameEnd = findNextCharacterPositionInCString( pStructDefinition, '}' );
            const int32_t embeddedTypeNameLength = castSizeToInt32( pEmbeddedTypeNameEnd - pEmbeddedTypeNameStart );

            ObjCTypeResolveResult resolveResult = {};
            resolveStructTypeIntoTypeDictionary( &resolveResult, pDict, pEmbeddedTypeNameStart, embeddedTypeNameLength );

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
            ObjCTypeResolveResult resolveResult = {};
            resolveBaseType( &resolveResult, pStructDefinition );

            structSizeInBits += resolveResult.typeSizeInBits;
            ++pStructDefinition;
        }

    }

    const char* pTypeDefinitionEnd = pStructDefinition;
    const int32_t originalTypeLength = castSizeToInt32( pTypeDefinitionEnd - pTypeDefinitionStart );

    pOutResult->isBaseType               = 0;
    pOutResult->isReference              = 0;
    pOutResult->pOriginalType            = (const char*)allocateCStringCopy( pTypeDefinitionStart, originalTypeLength );
    pOutResult->pResolvedType            = (const char*)allocateCStringCopy( pTypeNameStart, resolvedTypeNameLength );
    pOutResult->resolvedTypeLength       = resolvedTypeNameLength;
    pOutResult->originalTypeLength       = typeNameLength;
    pOutResult->typeSizeInBits           = structSizeInBits;
    return 1;
}

boolean8_t isStructTypeUnknown( const char* pTypeNameStart, const int32_t typeNameLength )
{
    return typeNameLength == 1u && *pTypeNameStart == '?';
}

boolean8_t resolveStructTypeIntoTypeDictionary( ObjCTypeResolveResult* pOutResult, ObjCTypeDict* pDict, const char* pTypeName, int32_t typeNameLength )
{
    const char* pTypeDefinitionStart = pTypeName;
    const char* pTypeNameStart = pTypeName + 1; //FK: +1 to skip initial '{' of struct definition
    
    // FK: End of typename is equal to the beginning of the struct definition
    //     which is why we search for the next '='
    const char* pTypeNameEnd = findNextCharacterPositionInCString( pTypeDefinitionStart, '=' );
    RuntimeAssert( pTypeNameEnd != NULL );

    boolean8_t isNewTypeDictEntry = 0;
    const int32_t resolvedTypeNameLength = castSizeToInt32( pTypeNameEnd - pTypeNameStart );

    if( isStructTypeUnknown( pTypeNameStart, resolvedTypeNameLength ) )
    {
        printf_stderr("[warning] typename '%.*s' is an unknown struct type. Skipping type resolve...", typeNameLength, pTypeName );
        return 0u;
    }

    ObjCTypeDictEntry* pEntry = insertTypeDictionaryEntry( pDict, pTypeNameStart, resolvedTypeNameLength, &isNewTypeDictEntry );
    if( !isNewTypeDictEntry )
    {
        *pOutResult = pEntry->resolveResult;
        return 1;
    }

    const boolean8_t resolvedSuccessfully = resolveStructType( pOutResult, pDict, pTypeName, typeNameLength );
    if( !resolvedSuccessfully )
    {
        pEntry->isNew = 1;
        return 0;
    }
    
    pEntry->resolveResult = *pOutResult;
    return 1;
}

static inline void extractStructName( const char** pOutStructName, int32_t* pOutStructNameLength, const char* pStructDeclaration )
{
    if( *pStructDeclaration == '{' )
    {
        ++pStructDeclaration;
    }

    const char* pStructNameStart = pStructDeclaration;
    const char* pStructNameEnd = findNextCharacterPositionInCString( pStructNameStart, '=' );
    RuntimeAssert( pStructNameEnd != NULL );

    *pOutStructName = pStructNameStart;
    *pOutStructNameLength = castSizeToInt32( pStructNameEnd - pStructNameStart ); //FK: -1 to not include '=' in the struct name
}

static inline boolean8_t isUnknownStructName( const char* pStructName, const int32_t structNameLength )
{
    return structNameLength == 1 && pStructName[0] == '?';
}

boolean8_t resolveReferenceType( ObjCTypeResolveResult* pOutResult, const char* pTypeName, int32_t typeNameLength )
{
    const boolean8_t isConst = ( pTypeName[0] == 'r' );
    if( isConst )
    {
        pTypeName = pTypeName + 1;
        --typeNameLength;
    }

    //FK: Check if the typename is a reference and return the position of the reference identifier ('^' or '*')
    const int32_t referencePosition = getTypenameReferencePosition( pTypeName, typeNameLength );
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

        if( isObjectiveCStructType( pTypeName ) )
        {
            extractStructName( &pReferenceType, &referenceTypeLength, pTypeName );
            if( isUnknownStructName( pReferenceType, referenceTypeLength ) )
            {
                //FK: We don't resolve unknown structs (yet?) 
                //    "unknown structs": Structs with a name of '?'
                return 0;
            }
            pOutResult->isBaseType = 0;
        }   
        else
        {
            if( !resolveBaseType( pOutResult, pTypeName ) )
            {
                return 0;
            }

            pReferenceType      = pOutResult->pResolvedType;
            referenceTypeLength = pOutResult->resolvedTypeLength;
        }
    }

    RuntimeAssert( pReferenceType != NULL && referenceTypeLength > 0 );

    char* pResolvedReferenceType = (char*)alloca( referenceTypeLength + 10u ); //FK: +10 for reference indicator ('*') + NULL terminator + const modified (if needed)
    const int32_t resolvedReferenceTypeLength = sprintf( pResolvedReferenceType, "%.*s*", referenceTypeLength, pReferenceType );
    
    pOutResult->pResolvedType       = allocateCStringCopy( pResolvedReferenceType, resolvedReferenceTypeLength );
    pOutResult->pOriginalType       = allocateCStringCopy( pTypeName, typeNameLength );
    pOutResult->resolvedTypeLength  = referenceTypeLength;
    pOutResult->originalTypeLength  = typeNameLength;
    pOutResult->isReference         = 1;
    pOutResult->isConst             = isConst;
    pOutResult->typeSizeInBits      = 64u;
    return 1;
}

boolean8_t resolveReferenceTypeIntoTypeDictionary( ObjCTypeResolveResult* pOutResult, ObjCTypeDict* pDict, const char* pTypeName, int32_t typeNameLength )
{
    boolean8_t isNewDictEntry = 0;
    ObjCTypeDictEntry* pDictEntry = insertTypeDictionaryEntry( pDict, pTypeName, typeNameLength, &isNewDictEntry );
    if( !isNewDictEntry )
    {
        *pOutResult = pDictEntry->resolveResult;
        return 1;
    }

    const boolean8_t resolvedSuccessfully = resolveReferenceType( pOutResult, pTypeName, typeNameLength );
    if( !resolvedSuccessfully )
    {
        pDictEntry->isNew = 1;
        return 0;
    }

    pDictEntry->resolveResult = *pOutResult;
    return 1;
}

boolean8_t resolveObjectiveCTypeName( ObjCTypeResolveResult* pOutResult, ObjCTypeDict* pTypeDict, const char* pTypeName, int32_t typeNameLength )
{
    boolean8_t resolvedSuccessfully = 0;
    if( isObjectiveCStructType( pTypeName ) )
    {
        resolvedSuccessfully = resolveStructTypeIntoTypeDictionary( pOutResult, pTypeDict, pTypeName, typeNameLength );
    }
    else if( isObjectiveCReference( pTypeName, typeNameLength ) )
    {
        resolvedSuccessfully = resolveReferenceTypeIntoTypeDictionary( pOutResult, pTypeDict, pTypeName, typeNameLength );
    }
    else if( typeNameLength == 1u )
    {
        resolvedSuccessfully = resolveBaseTypeIntoTypeDictionary( pOutResult, pTypeDict, pTypeName );
    }
    else
    {
        resolvedSuccessfully = 0;
        printf_stderr("[error] Couldn't resolve type '%.*s' - neither struct nor reference nor base type.", typeNameLength, pTypeName );
    }

    return resolvedSuccessfully;
}

const char* findNextNewline( const char* pText )
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

const char* findNextWhitespace( const char* pText )
{
    while( *pText )
    {
        if( isWhiteSpaceCharacter( *pText ) )
        {
            break;
        }

        ++pText;
    }

    return pText;
}

const char* findPreviousWhitespace( const char* pTextStart, const char* pText )
{
    while( pText != pTextStart )
    {
        if( isWhiteSpaceCharacter( *pText ) )
        {
            ++pText;
            break;
        }

        --pText;
    }

    return pText;
}

boolean8_t isValidFunctionName( const char* pFunctionName )
{
    //FK: Filter (apparently) internal functions that we're not interested in
    if( *pFunctionName == '_' || *pFunctionName == '.' )
    {
        return 0;
    }

#if 0
    const char* pFirstColon = findNextCharacterPositionInCString( pFunctionName, ':' );
    if( pFirstColon != NULL )
    {
        const char* pSecondColon = findNextCharacterPositionInCString( pFirstColon + 1, ':' );
        if( pSecondColon != NULL )
        {
            return 0;
        }
    }
#endif

    return 1;
}

const char* convertToCFunctionName( char* pObjectiveCFunctionName, int32_t functionNameLength )
{
    //FK: Some functions end with a colon (not sure yet what this indicates exactly)
    char* pColonPos = (char*)findNextCharacterPositionInCString( pObjectiveCFunctionName, ':' );
    if( pColonPos != NULL )
    {
        *pColonPos = 0;
    }

    return pObjectiveCFunctionName;
}

boolean8_t createStringAllocator( StringAllocator* pOutStringAllocator, const uint32_t capacityInBytes )
{
    StringAllocator allocator;
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

boolean8_t functionNameEndsWithColon( const char* pFunctionName, const int32_t functionNameLength )
{
    RuntimeAssert( functionNameLength > 0u );
    RuntimeAssert( pFunctionName != NULL );
    return pFunctionName[ functionNameLength - 1u ] == ':';
}

boolean8_t isFunctionPartOfCollection( const ObjCFunctionCollection* pFunctionCollection, const ObjCClassName* pClassName, const char* pFunctionName, const int32_t functionNameLength )
{
    //FK: Skip the 'classname_' part of the function name (to save an allocation in the calling code)
    //    eg: 'nsapplication_shutdown' -> skip 'nsapplication_'
    const int32_t functionCharSkipCount = pClassName->length;

    for( uint32_t functionIndex = 0u; functionIndex < pFunctionCollection->size; ++functionIndex )
    {
        const ObjCFunctionCollectionEntry* pFunctionNameEntry   = pFunctionCollection->pEntries + functionIndex;
        const int32_t functionBaseNameLength                    = pFunctionNameEntry->functionNameLength - functionCharSkipCount;
        RuntimeAssert( functionBaseNameLength > 0u );

        if( functionBaseNameLength == functionNameLength )
        {
            const char* pFunctionBaseName = pFunctionNameEntry->pFunctionName + functionCharSkipCount;
            if( areCStringsEqual( pFunctionBaseName, pFunctionName, functionNameLength ) )
            {
                return 1;
            }
        }
    }

    return 0;
}

void copyMemoryNonOverlapping( void* pDestination, const void* pSource, const size_t sizeInBytes )
{
    RuntimeAssert( ( pDestination < pSource && ( pDestination + sizeInBytes ) <= pSource ) || 
                   ( pDestination > pSource && ( pDestination + sizeInBytes ) >= pSource ) );

    const char* restrict_modifier pSourcePtr = (const char*)pSource;
    char* restrict_modifier pDestinationPtr = (char*)pDestination;

    //FK: TODO: Use STOS eventually - this approach is super slow
    for( size_t byteIndex = 0u; byteIndex < sizeInBytes; ++byteIndex )
    {
        *pDestinationPtr++ = *pSourcePtr++;
    }
}

void resetFunctionCollection( ObjCFunctionCollection* pFunctionCollection )
{
    pFunctionCollection->size = 0u;
}

void resizeFunctionCollection( ObjCFunctionCollection* pFunctionCollection )
{
    const uint32_t newSize = pFunctionCollection->capacity * 2;
    ObjCFunctionCollectionEntry* pNewEntries = ( ObjCFunctionCollectionEntry* )malloc( sizeof( ObjCFunctionCollectionEntry ) * newSize );
    if( pNewEntries == NULL )
    {
        //FK: TODO: Error handling
        return;
    }

    copyMemoryNonOverlapping( pNewEntries, pFunctionCollection->pEntries, pFunctionCollection->capacity * sizeof( ObjCFunctionCollectionEntry ) );
    pFunctionCollection->capacity = newSize;

    free( pFunctionCollection->pEntries );
    pFunctionCollection->pEntries = pNewEntries;
}

void addFunctionToCollection( ObjCFunctionCollection* pFunctionCollection, const char* pFunctionName, const int32_t functionNameLength )
{
    const boolean8_t isFunctionCollectionFull = pFunctionCollection->capacity == pFunctionCollection->size;
    if( isFunctionCollectionFull )
    {
        resizeFunctionCollection( pFunctionCollection );
    }

    const uint32_t entryIndex = pFunctionCollection->size;
    pFunctionCollection->size++;

    pFunctionCollection->pEntries[ entryIndex ].pFunctionName       = pFunctionName;
    pFunctionCollection->pEntries[ entryIndex ].functionNameLength  = functionNameLength;
}

ConvertResult convertParseResultToFunctionDefinition( StringAllocator* pStringAllocator, ObjCFunctionResolveResult* pOutFunctionResolveResult, ObjCFunctionCollection* pFunctionCollection, ObjCTypeDict* pDict, ParseResult* pParseResult, const ObjCClassName* pClassName )
{
    if( !isValidFunctionName( pParseResult->pFunctionName ) )
    {
        //FK: Add log here?
        return ConvertResult_InvalidFunctionName;
    }

    int32_t functionNameLength = pParseResult->functionNameLength;
    const char* pFunctionNameColonPosition = findNextCharacterPositionInCString( pParseResult->pFunctionName, ':' );
    if( pFunctionNameColonPosition != NULL )
    {
        functionNameLength = castSizeToInt32( pFunctionNameColonPosition - pParseResult->pFunctionName );
    }

    if( isFunctionPartOfCollection( pFunctionCollection, pClassName, pParseResult->pFunctionName, functionNameLength ) )
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
    addFunctionToCollection( pFunctionCollection, pResolvedFunctionName, resolvedFunctionNameLength );

    ObjCFunctionResolveResult resolveResult = {};
    resolveResult.pOriginalReturnType          = allocateCStringCopyWithAllocator( pStringAllocator, pParseResult->pReturnType, pParseResult->returnTypeLength );
    resolveResult.pOriginalArgumentTypes       = allocateCStringCopyWithAllocator( pStringAllocator, pParseResult->pArguments, pParseResult->argumentLength );
    resolveResult.pOriginalFunctionName        = allocateCStringCopyWithAllocator( pStringAllocator, pParseResult->pFunctionName, pParseResult->functionNameLength );
    resolveResult.pResolvedFunctionName        = pResolvedFunctionName;

    ObjCTypeResolveResult returnTypeResolveResult = {};
    if( !resolveObjectiveCTypeName( &returnTypeResolveResult, pDict, pParseResult->pReturnType, pParseResult->returnTypeLength ) )
    {
        printf_stderr( "[error] Couldn't resolve return type '%.*s' of function '%s'.\n", pParseResult->returnTypeLength, pParseResult->pReturnType, resolveResult.pResolvedFunctionName );
        return ConvertResult_UnknownReturnType;
    }

    resolveResult.pResolvedReturnType = returnTypeResolveResult.pResolvedType;
    resolveResult.pClassName          = pClassName;

    const char* pArguments = pParseResult->pArguments;
    uint8_t argumentCount = 0u;
    const uint8_t maxArgumentCount = ArrayCount( resolveResult.pResolvedArgumentTypes );
    const uint8_t argumentsToSkip = 2; //FK: Skip first two arguments since these are always the object + the selector.

    while( *pArguments )
    {
        const char* pArgumentStart = pArguments;
        const char* pArgumentEnd = findNextWhitespace( pArgumentStart );
        const int32_t argumentLength = castSizeToInt32( pArgumentEnd - pArgumentStart );

        if( argumentCount >= argumentsToSkip )
        {
            const uint8_t argumentIndex = argumentCount - argumentsToSkip;
            RuntimeAssert( argumentIndex < maxArgumentCount );

            ObjCTypeResolveResult argumentResolveResult = {};
            if( !resolveObjectiveCTypeName( &argumentResolveResult, pDict, pArgumentStart, argumentLength ) )
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

    //FK: If this triggers, increase ObjCFunctionResolveResult::argumentCount resolution
    RuntimeAssert( argumentCount <= 15 );

    resolveResult.argumentCount         = argumentCount < argumentsToSkip ? 0 : argumentCount - argumentsToSkip;
    resolveResult.isVoidFunction        = !returnTypeResolveResult.isReference && areCStringsEqual( resolveResult.pResolvedReturnType, "void", 5u );
    resolveResult.isAllocFunction       = strcmp( resolveResult.pOriginalFunctionName, "alloc" ) == 0u;
    resolveResult.hasStructReturnValue  = !returnTypeResolveResult.isReference && !returnTypeResolveResult.isBaseType;
    resolveResult.returnValueSizeInBits = returnTypeResolveResult.typeSizeInBits;
    resolveResult.hasFloatReturnValue   = returnTypeResolveResult.isFloatingType;
    resolveResult.methodType            = pParseResult->methodType;

    *pOutFunctionResolveResult = resolveResult;

    return ConvertResult_Success;
}

void writeCFunctionSignature( FILE* pResultFileHandle, const ObjCFunctionResolveResult* pFunctionDefinition )
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

void writeCFunctionDeclaration( FILE* pResultFileHandle, const ObjCFunctionResolveResult* pFunctionDefinition )
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
    const int32_t tabCountForReturnType = getMax( 0u, maxTabCount - ( returnTypeLength / spacesForTab ) );
    for( int32_t tabIndex = 0u; tabIndex < tabCountForReturnType; ++tabIndex )
    {
        fprintf( pResultFileHandle, "\t" );
    }

    writeCFunctionSignature( pResultFileHandle, pFunctionDefinition );

    fprintf( pResultFileHandle, ";\n\n" );
    fflush( pResultFileHandle );
}

void writeAutomicGeneratedComment( FILE* pFileHandle )
{
    fprintf( pFileHandle, "/*\n" );
    fprintf( pFileHandle, "\tThis file has been automatically generated by the shimmer industries c-ocoa API generator\n" );
    fprintf( pFileHandle, "\tThus, manual changes to this file will be lost if the file is re-generated.\n" );
    fprintf( pFileHandle, "*/\n\n" );
}

void writeCTypesHeaderPrefix( FILE* pTypesFileHandle )
{
    writeAutomicGeneratedComment( pTypesFileHandle );
    fprintf( pTypesFileHandle, "#ifndef C_OCOA_TYPES_HEADER\n");
    fprintf( pTypesFileHandle, "#define C_OCOA_TYPES_HEADER\n\n");
    fprintf( pTypesFileHandle, "#include <stdbool.h>\n\n");
    fprintf( pTypesFileHandle, "typedef void*\tnsobject_t;\n" );
    fprintf( pTypesFileHandle, "typedef void*\tnsselector_t;\n" );
    fprintf( pTypesFileHandle, "typedef void*\tnsclass_t;\n\n" );
}

void writeCTypesHeaderSuffix( FILE* pTypesFileHandle )
{
    fprintf( pTypesFileHandle, "#endif" );
}

void writeCHeaderPrefix( FILE* pHeaderFileHandle, const ObjCClassName* pClassName )
{
    writeAutomicGeneratedComment( pHeaderFileHandle );

    //FK: Header guard
    fprintf( pHeaderFileHandle, "#ifndef SHIMMER_C_OCOA_%s_HEADER\n#define SHIMMER_C_OCOA_%s_HEADER\n\n", pClassName->pNameUpper, pClassName->pNameUpper );
    fprintf( pHeaderFileHandle, "typedef void*\t%s_t;\n", pClassName->pNameLower );
    fprintf( pHeaderFileHandle, "#include \"c_ocoa_types.h\"\n\n");
}

void writeCHeaderSuffix( FILE* pHeaderFileHandle )
{
    //FK: End of header guard
    fprintf( pHeaderFileHandle, "#endif");
}

void writeCSourcePrefix( FILE* pSourceFileHandle, const char* pHeaderFileName, const ObjCClassName* pClassName) 
{
    writeAutomicGeneratedComment( pSourceFileHandle );
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

const char* findCorrectMsgSendCall( const ObjCFunctionResolveResult* pFunctionResolveResult )
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

void writeCFunctionImplementation( FILE* pSourceFileHandle, const ObjCFunctionResolveResult* pFunctionResolveResult, const ObjCClassName* pClassName )
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

    writeCFunctionSignature( pSourceFileHandle, pFunctionResolveResult );

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

    const char* msgSendCall = findCorrectMsgSendCall( pFunctionResolveResult );
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

boolean8_t createObjCClassName( ObjCClassName* pOutClassName, const char* pClassNameStart, const int32_t classNameLength )
{
    pOutClassName->length       = classNameLength;
    pOutClassName->pName        = allocateCStringCopy( pClassNameStart, classNameLength );
    pOutClassName->pNameLower   = allocateLowerCStringCopy( pClassNameStart, classNameLength );
    pOutClassName->pNameUpper   = allocateUpperCStringCopy( pClassNameStart, classNameLength );

    if( pOutClassName->pName == NULL || pOutClassName->pNameLower == NULL )
    {
        free( pOutClassName->pName );
        free( pOutClassName->pNameUpper );
        free( pOutClassName->pNameLower );
        return 0;
    }

    return 1;
}

#if 0
void parseTestFile( ObjCConversionArguments* pParseArguments, const char* pFileContentBuffer, size_t fileContentBufferSizeInBytes )
{
    const size_t dictTypeSizeInBytes = 1024*1024;
    const size_t dictFunctionSizeInBytes = 1024*1024*20;

    ObjCTypeDict typeDict;
    createObjectiveCTypeDictionary( &typeDict, dictTypeSizeInBytes, dictFunctionSizeInBytes );

    ObjCFunctionCollection functionCollection;
    createObjectiveCFunctionCollection( &functionCollection );

    const char* pClassNameEnd = findNextCharacterPositionInCString( pFileContentBuffer, ':' );
    const char* pClassNameStart = findPreviousWhitespace( pFileContentBuffer, pClassNameEnd );

    const int32_t classNameLength = castSizeToInt32( pClassNameEnd - pClassNameStart );
    pFileContentBuffer = findNextNewline( pFileContentBuffer ) + 1;

    typedef enum
    {
        EatWhitespace = 0,
        EatUntilNewLine,
        ParseReturnType,
        ParseArgumenType,
        ParseFunctionName
    } State;

    const uint32_t stringAllocatorSizeInBytes = 1024u*32u; //FK: 32KiB
    StringAllocator stringAllocator;
    if( !createStringAllocator( &stringAllocator, stringAllocatorSizeInBytes ) )
    {
        printf_stderr( "[error] Couldn't allocate %.3fKiB of memory for string allocation.\n", (float)stringAllocatorSizeInBytes/1024.f );
        return;
    }

    ObjCClassName className;
    if( !createObjCClassName( &className, pClassNameStart, classNameLength ) )
    {
        printf_stderr( "[error] Couldn't allocate %d byte of memory for creating classname.\n", classNameLength * 2 );
        return;
    }

    writeCHeaderPrefix( pParseArguments->pHeaderFileHandle, &className );
    writeCSourcePrefix( pParseArguments->pSourceFileHandle, pParseArguments->pHeaderFileName );

    ParseResult parseResult = {};
    const uint8_t ArgumentsToSkip = 2;
    uint8_t argumentCount = 0u;
    State state = ParseReturnType;
    State previousState = ParseReturnType;
    while( *pFileContentBuffer != 0 )
    {
        switch( state )
        {
            case EatWhitespace:
            {
                if( !isWhiteSpaceCharacter( *pFileContentBuffer ) )
                {
                    if( previousState == ParseArgumenType ||
                        previousState == ParseFunctionName )
                    {
                        state = ParseArgumenType;
                    }                    
                    else if( previousState == ParseReturnType )
                    {
                        state = ParseFunctionName;
                    }
                    break;
                }
                
                ++pFileContentBuffer;
                break;
            }

            case EatUntilNewLine:
            {
                if( *pFileContentBuffer == '\n' )
                {
                    ObjCFunctionResolveResult functionResolveResult;
                    const ConvertResult convertResult = convertParseResultToFunctionDefinition( &stringAllocator, &functionResolveResult, &functionCollection, &typeDict, &parseResult, &className );
                    switch( convertResult )
                    {
                        case ConvertResult_OutOfMemory:
                            printf_stderr("[error] Out of memory while trying to parse function of class '%s'!\n", className.pName );
                            break;

                        case ConvertResult_UnknownArgumentType:
                            printf_stderr("[error] Skipping function '%s' because of unknown argument type.\n", parseResult.pFunctionName );
                            break;

                        case ConvertResult_UnknownReturnType:
                            printf_stderr("[error] Skipping function '%s' because of unknown argument type.\n", parseResult.pFunctionName );
                            break;

                        case ConvertResult_AlreadyKnown:
                        case ConvertResult_InvalidFunctionName:
                            //FK: Add log here?
                            break;

                        case ConvertResult_Success:
                            writeCFunctionDeclaration( pParseArguments->pHeaderFileHandle, &functionResolveResult );
                            writeCFunctionImplementation( pParseArguments->pSourceFileHandle, &functionResolveResult, &className );
                            break;
                    }

                    resetStringAllocator( &stringAllocator );
                    
                    ++pFileContentBuffer;
                    state = ParseReturnType;
                    break;
                }

                ++pFileContentBuffer;
                break;
            }

            case ParseFunctionName:
            {
                const char* pFunctionNameStart      = pFileContentBuffer;
                const char* pNextWhiteSpacePosition = findNextCharacterPositionInCString( pFunctionNameStart, ' ' );
                const char* pNextColonPosition      = findNextCharacterPositionInCString( pFunctionNameStart, ':' );

                //FK: Ignore function name extension naming optional arguments
                //    eg: nextEventMatchingMask:untilDate:inMode:dequeue:
                int32_t functionNameLength = castSizeToInt32( pNextWhiteSpacePosition - pFunctionNameStart );
                if( pNextColonPosition < pNextWhiteSpacePosition )
                {
                    functionNameLength = castSizeToInt32( pNextColonPosition - pFunctionNameStart );
                }
            
                parseResult.pFunctionName = getCurrentStringAllocatorBase( &stringAllocator );
                if( functionNameEndsWithColon( pFunctionNameStart, functionNameLength ) )
                {
                    --functionNameLength;
                }

                const int32_t numCharactersWritten = sprintf( parseResult.pFunctionName, "%.*s", functionNameLength, pFunctionNameStart );
                parseResult.functionNameLength = numCharactersWritten;
                
                decrementStringAllocatorCapacity( &stringAllocator, numCharactersWritten + 1 );

                pFileContentBuffer += ( pNextWhiteSpacePosition - pFunctionNameStart );
                state = EatWhitespace;
                
                break;
            }

            case ParseReturnType:
            {
                const char* pReturnTypeStart = pFileContentBuffer;
                const char* pReturnTypeEnd   = findNextWhitespace( pReturnTypeStart );
                const int32_t typeLength = castSizeToInt32( pReturnTypeEnd - pReturnTypeStart );

                parseResult.pReturnType = getCurrentStringAllocatorBase( &stringAllocator );

                const int32_t numCharactersWritten = sprintf( parseResult.pReturnType, "%.*s", typeLength, pReturnTypeStart );
                parseResult.returnTypeLength = numCharactersWritten;
                
                decrementStringAllocatorCapacity( &stringAllocator, numCharactersWritten + 1 );

                pFileContentBuffer += typeLength;
                state = EatWhitespace;

                break;
            }
            case ParseArgumenType:
            {
                const char* pArgumentsStart = pFileContentBuffer;
                const char* pArgumentsEnd = findNextNewline( pArgumentsStart );
                const int32_t argumentLength = castSizeToInt32( pArgumentsEnd - pArgumentsStart );
               
                parseResult.pArguments = getCurrentStringAllocatorBase( &stringAllocator );

                const int32_t numCharactersWritten = sprintf( parseResult.pArguments, "%.*s", argumentLength, pArgumentsStart );
                parseResult.argumentLength = numCharactersWritten;

                decrementStringAllocatorCapacity( &stringAllocator, numCharactersWritten + 1 );

                state = EatUntilNewLine;
                pFileContentBuffer += argumentLength;

                break;
            }

            default:
                InvalidCodePath();
                break;
        }

        if( state != EatWhitespace )
        {
            previousState = state;
        }
    }

    writeCHeaderSuffix( pParseArguments->pHeaderFileHandle );
}
#endif

void writeCTypeForwardDeclarations( FILE* pTypesFileHandle, ObjCTypeDict* pTypeDict )
{
    fprintf( pTypesFileHandle, "// Forward declarations:\n" );
    for( uint32_t typeEntryIndex = 0u; typeEntryIndex < pTypeDict->typeEntryCapacity; ++typeEntryIndex )
    {
        const ObjCTypeDictEntry* pEntry = pTypeDict->pTypeEntries + typeEntryIndex;
        if( pEntry->isNew )
        {
            continue;
        }

        const ObjCTypeResolveResult* pResolveResult = &pEntry->resolveResult;
        if( !pResolveResult->isReference )
        {
            continue;
        }

        if( pResolveResult->isBaseType )
        {
            continue;
        }

        const int32_t resolvedTypeLength = pResolveResult->resolvedTypeLength; //FK: -1 to not include '*' (since this is a reference type...)
        fprintf( pTypesFileHandle, "struct _%.*s_;\n", resolvedTypeLength, pResolveResult->pResolvedType );
        fprintf( pTypesFileHandle, "typedef struct _%.*s_ %.*s;\n", resolvedTypeLength, pResolveResult->pResolvedType, resolvedTypeLength, pResolveResult->pResolvedType  );
    }

    fprintf( pTypesFileHandle, "\n" );
}

void writeSingleCTypeStructDefinition( FILE* pTypesFileHandle, ObjCTypeDictEntry* pStructEntry, ObjCTypeDict* pTypeDict, const ObjCTypeResolveResult* pObjCTypeDefinition )
{
    RuntimeAssert( !pStructEntry->declarationWasWritten );
    pStructEntry->declarationWasWritten = 1;

    const int32_t structResolveBufferCapacity = 2048;
    int32_t structResolveBufferLength   = 0u;
    char* pStructResolveBuffer          = (char*)alloca( structResolveBufferCapacity );

    char* pCurrentStructResolveBufferPosition = pStructResolveBuffer;

    const char* pCurrentStructMember = findNextCharacterPositionInCString( pObjCTypeDefinition->pOriginalType, '=' ) + 1; //FK: +1 to skip '='
    RuntimeAssert( pCurrentStructMember != NULL );

    const char* pStructEnd = findLastCharacterPositionInCString( pCurrentStructMember, '}' );
    RuntimeAssert( pStructEnd != NULL );

    boolean8_t skipStruct = 0;
    uint32_t structMemberCount = 0u;
    while( 1 )
    {
        int32_t structMemberTypeLength = 1;
        const boolean8_t isStructType = isObjectiveCStructType( pCurrentStructMember );
        if( isStructType )
        {
            const char* pEndCurrentStructMember = findNextCharacterPositionInCString( pCurrentStructMember, '}' );
            structMemberTypeLength = castSizeToInt32( pEndCurrentStructMember - pCurrentStructMember ) + 1; //FK: +1 to include '}' as part of the name
        }

        ObjCTypeResolveResult resolveResult = {};
        if( !resolveObjectiveCTypeName( &resolveResult, pTypeDict, pCurrentStructMember, structMemberTypeLength ) )
        {
            printf_stderr( "[error] skipped struct generation of struct '%s' because struct member of type '%c' couldn't be resolved.\n", pObjCTypeDefinition->pResolvedType, *pCurrentStructMember );
            skipStruct = 1;
            break;
        }

        if( isStructType )
        {   
            const char* pStructName = NULL;
            int32_t structNameLength = 0u;
            extractStructName( &pStructName, &structNameLength, pCurrentStructMember );

            ObjCTypeDictEntry* pEntry = findTypeDictionaryEntry( pTypeDict, pStructName, structNameLength );
            if( !pEntry->declarationWasWritten )
            {
                //FK: Type is not known yet, write first before continuing
                writeSingleCTypeStructDefinition( pTypesFileHandle, pEntry, pTypeDict, &resolveResult );
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

void writeCTypeStructDefinitions( FILE* pTypesFileHandle, ObjCTypeDict* pTypeDict )
{
    for( uint32_t typeEntryIndex = 0u; typeEntryIndex < pTypeDict->typeEntryCapacity; ++typeEntryIndex )
    {
        ObjCTypeDictEntry* pEntry = pTypeDict->pTypeEntries + typeEntryIndex;
        if( pEntry->isNew )
        {
            continue;
        }

        if( pEntry->declarationWasWritten )
        {
            continue;
        }

        const ObjCTypeResolveResult* pResolveResult = &pEntry->resolveResult;
        if( pResolveResult->isReference )
        {
            continue;
        }

        if( pResolveResult->isBaseType )
        {
            continue;
        }

        writeSingleCTypeStructDefinition( pTypesFileHandle, pEntry, pTypeDict, pResolveResult );
    }
}

void resolveStructTypesInTypeDictionary( ObjCTypeDict* pTypeDict, StringAllocator* pStringAllocator )
{
    FILE* pTypesFileHandle = fopen( "c_ocoa_types.h", "w" );
    if( pTypesFileHandle == NULL )
    {
        printf_stderr( "[error] Couldn't open '%s' for writing.\n", "c_cocoa_types.h" );
        return;
    }

    //FK: Write struct references first:
    writeCTypesHeaderPrefix( pTypesFileHandle );

    writeCTypeForwardDeclarations( pTypesFileHandle, pTypeDict );
    writeCTypeStructDefinitions( pTypesFileHandle, pTypeDict );

    //FK: Now write struct types:
    writeCTypesHeaderSuffix( pTypesFileHandle );
}

#endif