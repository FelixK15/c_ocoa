typedef uint8_t boolean8_t;


typedef struct
{
    const char* pOriginalType;
    const char* pResolvedType;
    int32_t     resolvedTypeLength;
    int32_t     originalTypeLength;

    boolean8_t isReference;
    boolean8_t isBaseType;
    boolean8_t isStdType;
} ObjCTypeResolveResult;

typedef struct
{
    char*                   pHashParameter;
    ObjCTypeResolveResult   resolveResult;
    boolean8_t              isNew;

} ObjCTypeDictEntry;

typedef struct
{
    ObjCTypeDictEntry* pEntries;
    uint32_t           entryCapacity;
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
    char* pReturnType;
    char* pFunctionName;
    char* pArguments;

    int32_t returnTypeLength;
    int32_t functionNameLength;
    int32_t argumentLength;
} ParseResult;

typedef struct 
{
    char* pBufferStart;
    uint32_t bufferCapacity;
    uint32_t bufferSize;
} StringAllocator;

typedef struct
{
    const char* pResolvedReturnType;
    const char* pResolvedArgumentTypes[32];

    char* pClassName;
    char* pLowerClassName;
    char* pResolvedFunctionName;
    char* pOriginalFunctionName;
    char* pOriginalReturnType;
    char* pOriginalArgumentTypes;
    
    uint8_t argumentCount   : 6;
    boolean8_t isVoidFunction  : 1;
    boolean8_t isInitFunction  : 1;
} CFunctionDefinition;

typedef enum
{
    ConvertResult_Success = 0,
    ConvertResult_InvalidFunctionName,
    ConvertResult_UnknownReturnType,
    ConvertResult_UnknownArgumentType
} ConvertResult;

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

static inline boolean8_t areCStringsEqual( const char* pStringA, const char* pStringB )
{
    while( *pStringA && *pStringB )
    {
        if( *pStringA++ != *pStringB++ )
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

static inline char* allocateCStringCopyWithAllocator( StringAllocator* pAllocator, const char* pString, const int32_t stringLength )
{
    char* pStringCopyMemory = getCurrentStringAllocatorBase( pAllocator );
    decrementStringAllocatorCapacity( pAllocator, stringLength + 1 );

    return copyCStringAndAddNullTerminator( pStringCopyMemory, pString, stringLength );
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
    ++pTypeName; //FK: Eat leading '{'
    return areCStringsEqual( pTypeName, "basic_string<char");
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
                if( areCStringsEqual( argv[argIndex], "--test" ) )
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

ObjCTypeDict* createObjectiveCTypeDictionary( size_t sizeInBytes )
{
    RuntimeAssert(sizeInBytes > sizeof( ObjCTypeDict ));
    uint8_t* pDictionaryMemory = (uint8_t*)malloc( sizeInBytes );
    ObjCTypeDict* pTypeDict = (ObjCTypeDict*)pDictionaryMemory;

    pDictionaryMemory += sizeof( ObjCTypeDict );

    sizeInBytes -= sizeof( ObjCTypeDict );

    pTypeDict->pEntries = (ObjCTypeDictEntry*)pDictionaryMemory;
    pTypeDict->entryCapacity = sizeInBytes / sizeof( ObjCTypeDictEntry );

    //FK: mark all entries as `isNew`
    for( size_t entryIndex = 0u; entryIndex < pTypeDict->entryCapacity; ++entryIndex )
    {
        pTypeDict->pEntries[ entryIndex ].isNew = 1;
    }

    return pTypeDict;
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
    const uint32_t entryIndex = hashValue % pDict->entryCapacity;
    ObjCTypeDictEntry* pEntry = pDict->pEntries + entryIndex;
    return pEntry->isNew ? NULL : pEntry;
}

ObjCTypeDictEntry* insertTypeDictionaryEntry( ObjCTypeDict* pDict, const char* restrict_modifier pTypeName, const int32_t typeNameLength, boolean8_t* restrict_modifier pOutIsNew )
{
    //FK: TODO: Check hash distribution
    const uint32_t hashValue = djb2_hash( pTypeName, typeNameLength );
    const uint32_t entryIndex = hashValue % pDict->entryCapacity;
    ObjCTypeDictEntry* pEntry = pDict->pEntries + entryIndex;

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
    const char* pResolvedBaseType = NULL;
    switch( *pObjectiveCTypeName )
    {
        case '@':
            pResolvedBaseType = "nsobject_t";
            break;
        case ':':
            pResolvedBaseType = "nsselector_t";
            break;
        case '#':
            pResolvedBaseType = "nsclass_t";
            break;
        case 'q':
        case 'Q':
            pResolvedBaseType = "uint32_t";
            break;
        case 's':
        case 'S':
            pResolvedBaseType = "uint16_t";
            break;
        case 'c':
        case 'C':
            pResolvedBaseType = "uint8_t";
            break;
        case 'v':
        case 'V':
            pResolvedBaseType = "void";
            break;
        case 'd':
        case 'D':
            pResolvedBaseType = "double";
            break;
        default:
            return 0;
    }

    const int32_t resolvedTypeLength = getCStringLengthExclNullTerminator( pResolvedBaseType );
    pOutResult->isStdType                = 0;
    pOutResult->isReference              = 0;
    pOutResult->isBaseType               = 1;
    pOutResult->originalTypeLength       = 1;
    pOutResult->pResolvedType            = allocateCStringCopy( pResolvedBaseType, resolvedTypeLength );
    pOutResult->pOriginalType            = allocateCStringCopy( pObjectiveCTypeName, 1 );
    pOutResult->resolvedTypeLength       = resolvedTypeLength;
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

    uint8_t scopeCount = 1u; //FK: start at one to include first leading '{'
    ++pTypeName;

    while( scopeCount > 0u )
    {
        if( *pTypeName == '{' )
        {
            ++scopeCount;

            const int32_t newTypeNameLength = typeNameLength - castSizeToInt32( pTypeName - pTypeDefinitionStart );

            ObjCTypeResolveResult resolveResult = {};
            resolveStructTypeIntoTypeDictionary( &resolveResult, pDict, pTypeName, newTypeNameLength );

            //FK: Advance by resolveResult.originalTypeLength ?
        }

        if( *pTypeName == '}' )
        {
            --scopeCount;
        }

        ++pTypeName;
    }

    const char* pTypeDefinitionEnd = pTypeName;
    const int32_t originalTypeLength = castSizeToInt32( pTypeDefinitionEnd - pTypeDefinitionStart );

    pOutResult->isStdType                = 0;
    pOutResult->isBaseType               = 0;
    pOutResult->isReference              = 0;
    pOutResult->pOriginalType            = (const char*)allocateCStringCopy( pTypeDefinitionStart, originalTypeLength );
    pOutResult->pResolvedType            = (const char*)allocateCStringCopy( pTypeNameStart, resolvedTypeNameLength );
    pOutResult->resolvedTypeLength       = resolvedTypeNameLength;
    pOutResult->originalTypeLength       = typeNameLength;
    return 1;
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

boolean8_t resolveReferenceType( ObjCTypeResolveResult* pOutResult, const char* pTypeName, int32_t typeNameLength )
{
    //FK: Check if the typename is a reference and return the position of the reference identifier ('^' or '*')
    const int32_t referencePosition = getTypenameReferencePosition( pTypeName, typeNameLength );

    //FK: Reference indicator should always be at the end or beginning of the type name (objective-c runtime returns both)
    RuntimeAssert( referencePosition == 0u || referencePosition == ( typeNameLength - 1u ) );

    //FK: Remove reference modifier from typename to expose underlying base type
    --typeNameLength;
    if( referencePosition == 0u )
    {
        pTypeName = pTypeName + 1;
    }

    const char* pReferenceType = NULL;
    int32_t referenceTypeLength = 0;
    if( isObjectiveCStructType( pTypeName) )
    {
        extractStructName( &pReferenceType, &referenceTypeLength, pTypeName );
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

    RuntimeAssert( pReferenceType != NULL && referenceTypeLength > 0 );
    char* pResolvedReferenceType = (char*)alloca( referenceTypeLength + 2u ); //FK: +12for reference indicator ('*') + NULL terminator
    const int32_t resolvedReferenceTypeLength = sprintf( pResolvedReferenceType, "%.*s*", referenceTypeLength, pReferenceType );
    
    pOutResult->pResolvedType       = allocateCStringCopy( pResolvedReferenceType, resolvedReferenceTypeLength );
    pOutResult->pOriginalType       = allocateCStringCopy( pTypeName, typeNameLength );
    pOutResult->resolvedTypeLength  = referenceTypeLength;
    pOutResult->originalTypeLength  = typeNameLength;
    pOutResult->isReference         = 1;
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

boolean8_t resolveObjectiveCTypeName( ObjCTypeResolveResult* pOutResult, boolean8_t* pOutNewEntry, ObjCTypeDict* pTypeDict, const char* pTypeName, int32_t typeNameLength )
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

    const char* pFirstColon = findNextCharacterPositionInCString( pFunctionName, ':' );
    if( pFirstColon != NULL )
    {
        const char* pSecondColon = findNextCharacterPositionInCString( pFirstColon + 1, ':' );
        if( pSecondColon != NULL )
        {
            return 0;
        }
    }

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

ConvertResult convertParseResultToFunctionDefinition( StringAllocator* pStringAllocator, CFunctionDefinition* pOutFunctionDefintion, ObjCTypeDict* pDict, ParseResult* pParseResult, const char* pClassName, int32_t classNameLength )
{
    if( !isValidFunctionName( pParseResult->pFunctionName ) )
    {
        //FK: Add log here?
        return ConvertResult_InvalidFunctionName;
    }

    pOutFunctionDefintion->pOriginalReturnType          = allocateCStringCopyWithAllocator( pStringAllocator, pParseResult->pReturnType, pParseResult->returnTypeLength );
    pOutFunctionDefintion->pOriginalArgumentTypes       = allocateCStringCopyWithAllocator( pStringAllocator, pParseResult->pArguments, pParseResult->argumentLength );
    pOutFunctionDefintion->pOriginalFunctionName        = allocateCStringCopyWithAllocator( pStringAllocator, pParseResult->pFunctionName, pParseResult->functionNameLength );
    pOutFunctionDefintion->pResolvedFunctionName        = allocateCStringCopyWithAllocator( pStringAllocator, pParseResult->pFunctionName, pParseResult->functionNameLength );
    convertToCFunctionName( pOutFunctionDefintion->pResolvedFunctionName, pParseResult->functionNameLength );

    ObjCTypeResolveResult returnTypeResolveResult = {};
    if( !resolveObjectiveCTypeName( &returnTypeResolveResult, NULL, pDict, pParseResult->pReturnType, pParseResult->returnTypeLength ) )
    {
        printf_stderr( "[error] Couldn't resolve return type '%.*s' of function '%s'.\n", pParseResult->returnTypeLength, pParseResult->pReturnType, pOutFunctionDefintion->pResolvedFunctionName );
        return ConvertResult_UnknownReturnType;
    }

    pOutFunctionDefintion->pResolvedReturnType = returnTypeResolveResult.pResolvedType;
    pOutFunctionDefintion->pClassName          = allocateCStringCopyWithAllocator( pStringAllocator, pClassName, classNameLength );
    pOutFunctionDefintion->pLowerClassName     = allocateCStringCopyWithAllocator( pStringAllocator, pClassName, classNameLength );
    convertCStringToLowerInplace( pOutFunctionDefintion->pLowerClassName, classNameLength );

    const char* pArguments = pParseResult->pArguments;
    uint8_t argumentCount = 0u;
    const uint8_t maxArgumentCount = ArrayCount( pOutFunctionDefintion->pResolvedArgumentTypes );
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
            if( !resolveObjectiveCTypeName( &argumentResolveResult, NULL, pDict, pArgumentStart, argumentLength ) )
            {
                printf_stderr( "[error] Couldn't resolve %d. argument type name '%.*s' of function '%s'.\n", argumentCount, argumentLength, pArgumentStart, pOutFunctionDefintion->pResolvedFunctionName );
                return ConvertResult_UnknownArgumentType;
            }

            pOutFunctionDefintion->pResolvedArgumentTypes[ argumentIndex ] = argumentResolveResult.pResolvedType;
        }

        ++argumentCount;
        if( *pArgumentEnd == 0 )
        {
            break;
        }

        pArguments = pArgumentEnd + 1;
    }

    pOutFunctionDefintion->argumentCount  = argumentCount < argumentsToSkip ? 0 : argumentCount - argumentsToSkip;
    pOutFunctionDefintion->isVoidFunction = areCStringsEqual( pOutFunctionDefintion->pResolvedReturnType, "void" );
    pOutFunctionDefintion->isInitFunction = areCStringsEqual( pOutFunctionDefintion->pResolvedFunctionName, "init" ) 
        || areCStringsEqual( pOutFunctionDefintion->pResolvedFunctionName, "initWithCoder" );

    return ConvertResult_Success;
}

void writeCFunctionSignature( FILE* pResultFileHandle, const CFunctionDefinition* pFunctionDefinition )
{
    //FK: Function name
    fprintf( pResultFileHandle, "%s_%s(", pFunctionDefinition->pLowerClassName, pFunctionDefinition->pResolvedFunctionName );

    if( !pFunctionDefinition->isInitFunction )
    {
        //FK: add object as first parameter for non init functions
        fprintf( pResultFileHandle, " %s_t object", pFunctionDefinition->pLowerClassName );

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

void writeCFunctionDeclaration( FILE* pResultFileHandle, const CFunctionDefinition* pFunctionDefinition )
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

    if( pFunctionDefinition->isInitFunction )
    {
        //FK: little syntactic sugar, return correct type for init function(s)
        returnTypeLength = fprintf( pResultFileHandle, "%s_t ", pFunctionDefinition->pLowerClassName );
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
}

void writeCTypesHeaderSuffix( FILE* pTypesFileHandle )
{
    fprintf( pTypesFileHandle, "#endif" );
}

void writeCHeaderPrefix( FILE* pHeaderFileHandle, const char* pClassName, const int32_t classNameLength )
{
    char* pUpperClassName = (char*)alloca( classNameLength + 1u );
    convertCStringToUpper( pUpperClassName, pClassName, classNameLength );
    pUpperClassName[ classNameLength ] = 0;

    writeAutomicGeneratedComment( pHeaderFileHandle );

    //FK: Header guard
    fprintf( pHeaderFileHandle, "#ifndef SHIMMER_C_OCOA_%s_HEADER\n#define SHIMMER_C_OCOA_%s_HEADER\n\n", pUpperClassName, pUpperClassName );

    char* pLowerClassName = convertCStringToLowerInplace( pUpperClassName, classNameLength );
    fprintf( pHeaderFileHandle, "typedef void*\t%s_t;\n", pLowerClassName );
    fprintf( pHeaderFileHandle, "typedef void*\tnsobject_t;\n" );
    fprintf( pHeaderFileHandle, "typedef void*\tnsselector_t;\n" );
    fprintf( pHeaderFileHandle, "typedef void*\tnsclass_t;\n\n" );
}

void writeCHeaderSuffix( FILE* pHeaderFileHandle )
{
    //FK: End of header guard
    fprintf( pHeaderFileHandle, "#endif");
}

void writeCSourcePrefix( FILE* pSourceFileHandle, const char* pHeaderFileName) 
{
    writeAutomicGeneratedComment( pSourceFileHandle );
    fprintf( pSourceFileHandle, "#include \"%s\"\n\n", pHeaderFileName );
}

void writeCFunctionImplementation( FILE* pSourceFileHandle, const CFunctionDefinition* pFunctionDefinition, const char* pClassName, const int32_t classNameLength )
{   
    size_t returnTypeLength = 0u;
    if( pFunctionDefinition->isInitFunction )
    {
        //FK: little syntactic sugar, return correct type for init function(s)
        returnTypeLength = fprintf( pSourceFileHandle, "%s_t ", pFunctionDefinition->pLowerClassName );
    }
    else
    {
        returnTypeLength = fprintf( pSourceFileHandle, "%s ", pFunctionDefinition->pResolvedReturnType );
    }

    writeCFunctionSignature( pSourceFileHandle, pFunctionDefinition );

    fprintf( pSourceFileHandle, "\n{\n" );
    fprintf( pSourceFileHandle, "\tClass internalClassObject = objc_getClass( \"%.*s\" );\n", classNameLength, pClassName );
    fprintf( pSourceFileHandle, "\tSEL methodSelector = sel_registerName( \"%s\" );\n", pFunctionDefinition->pOriginalFunctionName );
    fprintf( pSourceFileHandle, "\tIMP methodImplementation = class_getMethodImplementation( internalClassObject, methodSelector );\n" );
    fprintf( pSourceFileHandle, "\ttypedef %s( *MethodFunctionPtr )( id, SEL", pFunctionDefinition->pResolvedReturnType );

    for( uint8_t argumentIndex = 0u; argumentIndex < pFunctionDefinition->argumentCount; ++argumentIndex )
    {
        const char* pArgumentType = pFunctionDefinition->pResolvedArgumentTypes[ argumentIndex ];
        fprintf( pSourceFileHandle, ", %s", pArgumentType );
    }

    fprintf( pSourceFileHandle, " );\n\n" );
    fprintf( pSourceFileHandle, "\tMethodFunctionPtr impl = ( MethodFunctionPtr )methodImplementation;\n" );
    fprintf( pSourceFileHandle, "\t" );

    const boolean8_t hasReturnValue = !areCStringsEqual( pFunctionDefinition->pResolvedReturnType, "void" );
    if( hasReturnValue )
    {
        fprintf( pSourceFileHandle, "return " );
    }

    if( pFunctionDefinition->isInitFunction )
    {
        fprintf( pSourceFileHandle, "impl( (id)internalClassObject, methodSelector");
    }
    else
    {
        fprintf( pSourceFileHandle, "impl( (id)object, methodSelector");
    }

    for( uint8_t argumentIndex = 0u; argumentIndex < pFunctionDefinition->argumentCount; ++argumentIndex )
    {
        fprintf( pSourceFileHandle, ", arg%hhu", argumentIndex );
    }
    fprintf( pSourceFileHandle, " );\n}\n\n");
    fflush( pSourceFileHandle );
}

void parseTestFile( ObjCConversionArguments* pParseArguments, const char* pFileContentBuffer, size_t fileContentBufferSizeInBytes )
{
    const size_t dictSizeInBytes = 1024*1024; //FK: 1 MiB
    ObjCTypeDict* pDict = createObjectiveCTypeDictionary( dictSizeInBytes );
    if( pDict == NULL )
    {
        return;
    }

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

    writeCHeaderPrefix( pParseArguments->pHeaderFileHandle, pClassNameStart, classNameLength );
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
                    CFunctionDefinition functionDefinition;
                    const ConvertResult convertResult = convertParseResultToFunctionDefinition( &stringAllocator, &functionDefinition, pDict, &parseResult, pClassNameStart, classNameLength );
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
                            writeCFunctionDeclaration( pParseArguments->pHeaderFileHandle, &functionDefinition );
                            writeCFunctionImplementation( pParseArguments->pSourceFileHandle, &functionDefinition, pClassNameStart, classNameLength );
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

void writeCTypeForwardDeclarations( FILE* pTypesFileHandle, ObjCTypeDict* pTypeDict )
{
    fprintf( pTypesFileHandle, "// Forward declarations:\n" );
    for( uint32_t typeEntryIndex = 0u; typeEntryIndex < pTypeDict->entryCapacity; ++typeEntryIndex )
    {
        const ObjCTypeDictEntry* pEntry = pTypeDict->pEntries + typeEntryIndex;
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
        fprintf( pTypesFileHandle, "typedef _%.*s_ %.*s;\n", resolvedTypeLength, pResolveResult->pResolvedType, resolvedTypeLength, pResolveResult->pResolvedType  );
    }

    fprintf( pTypesFileHandle, "\n" );
}

void writeSingleCTypeStructDefinition( FILE* pTypesFileHandle, ObjCTypeDict* pTypeDict, const ObjCTypeResolveResult* pObjCTypeDefinition )
{
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
        if( isObjectiveCStructType( pCurrentStructMember ) )
        {
            const char* pEndCurrentStructMember = findNextCharacterPositionInCString( pCurrentStructMember, '}' );
            structMemberTypeLength = castSizeToInt32( pEndCurrentStructMember - pCurrentStructMember ) + 1; //FK: +1 to include '}' as part of the name
        }

        boolean8_t isNewType = 0;
        ObjCTypeResolveResult resolveResult = {};
        if( !resolveObjectiveCTypeName( &resolveResult, &isNewType, pTypeDict, pCurrentStructMember, structMemberTypeLength ) )
        {
            printf_stderr( "[error] skipped struct generation of struct '%s' because struct member of type '%c' couldn't be resolved.\n", pObjCTypeDefinition->pResolvedType, *pCurrentStructMember );
            skipStruct = 1;
            break;
        }

        if( isNewType && !resolveResult.isBaseType && !resolveResult.isReference )
        {
            writeSingleCTypeStructDefinition( pTypesFileHandle, pTypeDict, &resolveResult );
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
    for( uint32_t typeEntryIndex = 0u; typeEntryIndex < pTypeDict->entryCapacity; ++typeEntryIndex )
    {
        const ObjCTypeDictEntry* pEntry = pTypeDict->pEntries + typeEntryIndex;
        if( pEntry->isNew )
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

        writeSingleCTypeStructDefinition( pTypesFileHandle, pTypeDict, pResolveResult );
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