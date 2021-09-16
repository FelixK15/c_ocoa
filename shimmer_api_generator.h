typedef struct
{
    char* pOriginalType;
    const char* pResolvedType;

    uint8_t isNew;
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
    char* pResolvedArgumentTypes[32];

    int32_t returnTypeLength;
    int32_t functionNameLength;
    int32_t argumentLength;
    int32_t argumentCount;
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

    uint8_t argumentCount   : 6;
    uint8_t isVoidFunction  : 1;
    uint8_t isInitFunction  : 1;
} CFunctionDefinition;

typedef enum
{
    ConvertResult_Success = 0,
    ConvertResult_InvalidFunctionName,
    ConvertResult_UnknownReturnType,
    ConvertResult_UnknownArgumentType
} ConvertResult;

#define printf_stderr(x, ...) fprintf( stderr, x, __VA_ARGS__ )

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

uint8_t createConversionArguments( ObjCConversionArguments* pOutArguments, const char* pHeaderFileName, const char* pSourceFileName )
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

inline uint32_t castSizeToUint32( size_t val )
{
    RuntimeAssert( val < UINT32_MAX );
    return (uint32_t)val;
}

inline int32_t castSizeToInt32( size_t val )
{
    RuntimeAssert( val < INT32_MAX );
    return (int32_t)val;
}

inline void fillMemoryWithZeroes( void* pMemory, const size_t sizeInBytes )
{
    //FK: TODO: use STOSB - currently this is slower than memset (but saves the string.h include)
    uint8_t* pMemoryAsByteArray = (uint8_t*)pMemory;
    for( size_t byteIndex = 0u; byteIndex < sizeInBytes; ++byteIndex )
    {
        *pMemoryAsByteArray = 0;
    }
}

//FK: Use custom string.h alternatives since we're not concerned with locale 
inline uint8_t isWhiteSpaceCharacter( const char character )
{
    return character == ' '  ||
           character == '\n' ||
           character == '\t' ||
           character == '\v' ||
           character == '\f' ||
           character == '\r';
}

inline uint8_t isAlphabeticalCharacter( const char character )
{
    return ( character >= 'A' && character <= 'Z' ) || ( character >= 'a' && character <= 'z' );
}

inline char convertCharacterToLower( const char character )
{
    if( !isAlphabeticalCharacter( character ) )
    {
        return character;
    }

    return character <= 'Z' ? character + 32 : character;
}

inline char convertCharacterToUpper( const char character )
{
    if( !isAlphabeticalCharacter( character ) )
    {
        return character;
    }

    return character >= 'a' ? character - 32 : character;
}

inline uint8_t areCStringsEqual( const char* pStringA, const char* pStringB )
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

inline int32_t getCStringLengthInclNullTerminator( const char* pString )
{
    const char* pStringStart = pString;
    while( *pString++ );

    return castSizeToInt32( pString - pStringStart );
}

inline char* copyCStringAndAddNullTerminator( char* pDestination, const char* pSource, const int32_t stringLength )
{
    for( int32_t charIndex = 0; charIndex < stringLength; ++charIndex )
    {
        pDestination[ charIndex ] = pSource[ charIndex ];
    }

    pDestination[ stringLength ] = 0;
    return pDestination;
}

inline char* convertCStringToLowerInplace( char* pString, const int32_t stringLength )
{
    for( int32_t charIndex = 0u; charIndex < stringLength; ++charIndex )
    {
        pString[ charIndex ] = convertCharacterToLower( pString[ charIndex ] );
    }
    return pString;
}

inline char* convertCStringToLower( char* pDestination, const char* pSource, const int32_t sourceLength )
{
    for( int32_t charIndex = 0u; charIndex < sourceLength; ++charIndex )
    {
        pDestination[ charIndex ] = convertCharacterToLower( pSource[ charIndex ] );
    }

    return pDestination;
}

inline char* convertCStringToUpper( char* pDestination, const char* pSource, const int32_t sourceLength )
{
    for( int32_t charIndex = 0u; charIndex < sourceLength; ++charIndex )
    {
        pDestination[ charIndex ] = convertCharacterToUpper( pSource[ charIndex ] );
    }

    return pDestination;
}

inline char* allocateCStringCopy( const char* pString, const int32_t stringLength )
{
    char* pStringCopyMemory = (char*)malloc( stringLength + 1 );
    if( pStringCopyMemory == NULL )
    {
        //FK: TODO: Handle out of memory
        return NULL;
    }

    return copyCStringAndAddNullTerminator( pStringCopyMemory, pString, stringLength );
}

inline char* allocateLowerCStringCopy( const char* pString, const int32_t stringLength )
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

inline char* allocateCStringCopyWithAllocator( StringAllocator* pAllocator, const char* pString, const int32_t stringLength )
{
    char* pStringCopyMemory = getCurrentStringAllocatorBase( pAllocator );
    decrementStringAllocatorCapacity( pAllocator, stringLength + 1 );

    return copyCStringAndAddNullTerminator( pStringCopyMemory, pString, stringLength );
}

inline const char* findNextCharacterPositionInCString( const char* pString, char character )
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

inline uint8_t isStdStringType( const char* pTypeName )
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

uint8_t stringsAreEqual( const char* pStringA, const char* pStringB, const int32_t stringLength )
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

    while( *pString )
    {
        const char c = *pString++;
        hash = ((hash << 5) + hash) + c;
    }
        
    return hash;
}

ObjCTypeDictEntry* insertTypeDictionaryEntry( ObjCTypeDict* pDict, const char* restrict_modifier pTypeName, const int32_t typeNameLength, uint8_t* restrict_modifier pOutIsNew )
{
    //FK: TODO: Check hash distribution
    const uint32_t hashValue = djb2_hash( pTypeName, typeNameLength );
    const uint32_t entryIndex = hashValue % pDict->entryCapacity;
    ObjCTypeDictEntry* pEntry = pDict->pEntries + entryIndex;

    *pOutIsNew = pEntry->isNew;
    if( pEntry->isNew )
    {
        //FK: TODO: use custom linear allocator?
        pEntry->pOriginalType = (char*)malloc( typeNameLength + 1 ); 
        if( pEntry->pOriginalType == NULL )
        {
            //FK: out of memory.
            //FK: TODO: Print message, let the user know what's going on
            return NULL;
        }
        sprintf( pEntry->pOriginalType, "%.*s", typeNameLength, pTypeName );
        pEntry->isNew = 0;
    }

    //FK: Check for hash collision
    RuntimeAssert( stringsAreEqual( pEntry->pOriginalType, pTypeName, typeNameLength ) );

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

uint8_t isObjectiveCStructType( const char* pTypeName )
{
    return *pTypeName == '{';
}

const char* resolveBaseType( const char* pObjectiveCTypeName )
{
    switch( *pObjectiveCTypeName )
    {
        case '@':
            return "nsobject_t";

        case ':':
            return "nssignal_t";

        case '#':
            return "nsclass_t";

        case 'q':
        case 'Q':
            return "uint32_t";

        case 's':
        case 'S':
            return "uint16_t";

        case 'c':
        case 'C':
            return "uint8_t";

        case 'v':
        case 'V':
            return "void";
    }

    return NULL;
}

const char* resolveStructType( ObjCTypeDict* pTypeDict, const char* pTypeName, int32_t typeNameLength )
{
    const char* pResolvedTypeName = NULL;
    if( isStdStringType( pTypeName ) )
    {
        return "std::string";
    }
    
    //FK: Make copy to be able to manipulate string and leaving the original intact
    //    (since the original typename is used as a lookup into the type dictionary)
    char* pTypeNameCopy = (char*)alloca( typeNameLength );
    sprintf( pTypeNameCopy, "%.*s", typeNameLength, pTypeName );
    
    //FK: Eat first '{'
    ++pTypeNameCopy;

    //FK: Eat following '_'
    if( *pTypeNameCopy == '_' )
    {
        ++pTypeNameCopy;
    }

    //FK: pTypeNameCopy should now point to the beginning of the *actual* typename
    //    without any leading identifier like '{' or '_'.
    //    Search for end of type name to be able to extract it
    char* pTypeNameCopyEnd = ( char* )findNextCharacterPositionInCString( pTypeNameCopy, '=' );
    RuntimeAssert( pTypeNameCopyEnd != NULL );
    *pTypeNameCopyEnd = 0;

    typeNameLength = ( pTypeNameCopyEnd - pTypeNameCopy );

    uint8_t isNew = 0;
    ObjCTypeDictEntry* pTypeDictEntry = insertTypeDictionaryEntry( pTypeDict, pTypeNameCopy, typeNameLength, &isNew );
    if( isNew )
    {
        pTypeDictEntry->pResolvedType = allocateCStringCopy( pTypeNameCopy, typeNameLength );
    }

    return pTypeDictEntry->pResolvedType;
}

const char* resolveReferenceType( ObjCTypeDict* pTypeDict, const char* pTypeName, int32_t typeNameLength )
{
    //FK: Make copy to be able to manipulate string and leaving the original intact
    char* pTypeNameCopy = (char*)alloca( typeNameLength );
    sprintf( pTypeNameCopy, "%.*s", typeNameLength, pTypeName );

    //FK: Check if the typename is a reference and return the position of the reference identifier ('^' or '*')
    const int32_t referencePosition = getTypenameReferencePosition( pTypeNameCopy, typeNameLength );
    
    //FK: If we enter this block, we *should* be dealing with a reference
    //    TODO: Double-check
    RuntimeAssert( referencePosition != INT32_MAX );

    //FK: Reference indicator should always be at the end or beginning of the type name (objective-c runtime returns both)
    RuntimeAssert( referencePosition == 0u || referencePosition == ( typeNameLength - 1u ) );

    //FK: Remove reference modifier from typename to expose underlying base type
    --typeNameLength;
    if( referencePosition == 0u )
    {
        pTypeNameCopy = pTypeNameCopy + 1;
    }

    const char* pBaseTypeName = NULL;
    if( isObjectiveCStructType( pTypeNameCopy ) )
    {
        pBaseTypeName = resolveStructType( pTypeDict, pTypeNameCopy, typeNameLength );
    }
    else
    {
        pBaseTypeName = resolveBaseType( pTypeNameCopy );
    }

    const size_t baseTypeNameLength = getCStringLengthInclNullTerminator( pBaseTypeName );

    char* pReferenceType = (char*)alloca( baseTypeNameLength +1u ); //FK: +1 for reference indicator ('*')
    const int32_t referenceTypeLength = sprintf( pReferenceType, "%s*", pBaseTypeName );

    uint8_t isNew = 0;
    ObjCTypeDictEntry* pTypeDictEntry = insertTypeDictionaryEntry( pTypeDict, pReferenceType, referenceTypeLength, &isNew );
    if( isNew )
    {
        pTypeDictEntry->pResolvedType = allocateCStringCopy( pReferenceType, referenceTypeLength );
    }

    return pTypeDictEntry->pResolvedType;
}

const char* resolveObjectiveCTypeName( ObjCTypeDict* pTypeDict, const char* pTypeName, int32_t typeNameLength )
{
    if( typeNameLength == 1u )
    {
        return resolveBaseType( pTypeName );
    }
    else if( isObjectiveCStructType( pTypeName ) )
    {
        return resolveStructType( pTypeDict, pTypeName, typeNameLength );
    }
    
    return resolveReferenceType( pTypeDict, pTypeName, typeNameLength );
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

uint8_t isValidFunctionName( const char* pFunctionName )
{
    //FK: Filter (apparently) internal functions that we're not interested in
    return ( *pFunctionName != '_' && *pFunctionName != '.' );
}

const char* convertToCFunctionName( char* pObjectiveCFunctionName, int32_t functionNameLength )
{
    //FK: Some functions end with a colon (not sure yet what this indicates exactly)
    if( pObjectiveCFunctionName[ functionNameLength - 1 ] == ':' )
    {
        pObjectiveCFunctionName[ functionNameLength - 1 ] = 0;
    }

    return pObjectiveCFunctionName;
}

uint8_t createStringAllocator( StringAllocator* pOutStringAllocator, const uint32_t capacityInBytes )
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

    pOutFunctionDefintion->pOriginalFunctionName    = allocateCStringCopyWithAllocator( pStringAllocator, pParseResult->pFunctionName, pParseResult->functionNameLength );
    pOutFunctionDefintion->pResolvedFunctionName    = allocateCStringCopyWithAllocator( pStringAllocator, pParseResult->pFunctionName, pParseResult->functionNameLength );
    convertToCFunctionName( pOutFunctionDefintion->pResolvedFunctionName, pParseResult->functionNameLength );

    const char* pResolvedReturnType = resolveObjectiveCTypeName( pDict, pParseResult->pReturnType, pParseResult->returnTypeLength );
    if( pResolvedReturnType == NULL )
    {
        printf_stderr( "[error] Couldn't resolve return type '%.*s' of function '%s'.\n", pParseResult->returnTypeLength, pParseResult->pReturnType, pOutFunctionDefintion->pResolvedFunctionName );
        return ConvertResult_UnknownReturnType;
    }

    pOutFunctionDefintion->pResolvedReturnType      = pResolvedReturnType;


    pOutFunctionDefintion->pClassName       = allocateCStringCopyWithAllocator( pStringAllocator, pClassName, classNameLength );
    pOutFunctionDefintion->pLowerClassName  = allocateCStringCopyWithAllocator( pStringAllocator, pClassName, classNameLength );
    convertCStringToLowerInplace( pOutFunctionDefintion->pLowerClassName, classNameLength );

    const char* pArguments = pParseResult->pArguments;
    uint8_t argumentCount = 0u;
    const uint8_t maxArgumentCount = ArrayCount( pParseResult->pResolvedArgumentTypes );
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

            const char* pArgumentType = resolveObjectiveCTypeName( pDict, pArgumentStart, argumentLength );
            if( pArgumentType == NULL )
            {
                printf_stderr( "[error] Couldn't resolve %d. argument type name '%.*s' of function '%s'.\n", argumentCount, argumentLength, pArgumentStart, pOutFunctionDefintion->pResolvedFunctionName );
                return ConvertResult_UnknownArgumentType;
            }

            pOutFunctionDefintion->pResolvedArgumentTypes[ argumentIndex ] = pArgumentType;
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

void writeCFunctionDeclaration( FILE* pResultFileHandle, const CFunctionDefinition* pFunctionDefinition )
{   
    //FK: "guess" maximum tab count for nice formatting
    //    getting the correct tab count for the longest
    //    return type name would be too involved since
    //    we currently run on a function by function basis
    //    and thus don't know the maximum type name length
    //    beforehand
    const size_t maxTabCount = 5u;
    size_t returnTypeLength = 0u;

    if( pFunctionDefinition->isInitFunction )
    {
        //FK: little syntactic sugar, return correct type for init function(s)
        returnTypeLength = fprintf( pResultFileHandle, "%s_t ", pFunctionDefinition->pLowerClassName );
    }
    else
    {
        returnTypeLength = fprintf( pResultFileHandle, "%s ", pFunctionDefinition->pResolvedReturnType );
    }

    const size_t spacesForTab = 4u;
    const size_t tabCountForReturnType = getMax( 0u, maxTabCount - ( returnTypeLength / spacesForTab ) );
    for( size_t tabIndex = 0u; tabIndex < tabCountForReturnType; ++tabIndex )
    {
        fprintf( pResultFileHandle, "\t" );
    }

    fprintf( pResultFileHandle, "%s_%s( %s_t object", pFunctionDefinition->pLowerClassName, pFunctionDefinition->pResolvedFunctionName, pFunctionDefinition->pLowerClassName );

    for( uint8_t argumentIndex = 0u; argumentIndex < pFunctionDefinition->argumentCount; ++argumentIndex )
    {
        fprintf( pResultFileHandle, ", %s arg%u", pFunctionDefinition->pResolvedArgumentTypes[ argumentIndex ], argumentIndex );
    }

    fprintf( pResultFileHandle, " );\n" );
    fflush( pResultFileHandle );
}

void writeCHeaderPrefix( FILE* pHeaderFileHandle, const char* pClassName, const int32_t classNameLength )
{
    char* pUpperClassName = (char*)alloca( classNameLength + 1u );
    convertCStringToUpper( pUpperClassName, pClassName, classNameLength );
    pUpperClassName[ classNameLength ] = 0;

    fprintf( pHeaderFileHandle, "/*\n" );
    fprintf( pHeaderFileHandle, "\tThis file has been automatically generated by the shimmer industries c-ocoa API generator\n" );
    fprintf( pHeaderFileHandle, "\tThus, manual changes to this file will be lost if the file is re-generated.\n" );
    fprintf( pHeaderFileHandle, "*/\n\n" );

    //FK: Header guard
    fprintf( pHeaderFileHandle, "#ifndef SHIMMER_%s_HEADER\n#define SHIMMER_%s_HEADER\n\n", pUpperClassName, pUpperClassName );
}

void writeCHeaderSuffix( FILE* pHeaderFileHandle )
{
    //FK: End of header guard
    fprintf( pHeaderFileHandle, "#endif");
}

void writeCSourcePrefix( FILE* pSourceFileHandle, const char* pHeaderFileName, const char* pClassName, const int32_t classNameLength )
{
    char* pLowerClassName = (char*)alloca( classNameLength + 1u );
    convertCStringToLower( pLowerClassName, pClassName, classNameLength );
    pLowerClassName[ classNameLength ] = 0;

    fprintf( pSourceFileHandle, "/*\n" );
    fprintf( pSourceFileHandle, "\tThis file has been automatically generated by the shimmer industries c-ocoa API generator\n" );
    fprintf( pSourceFileHandle, "\tThus, manual changes to this file will be lost if the file is re-generated.\n" );
    fprintf( pSourceFileHandle, "*/\n\n" );

    fprintf( pSourceFileHandle, "#include \"%s\"\n\n", pHeaderFileName );
    fprintf( pSourceFileHandle, "static Class internalClassObject = objc_getClass( \"%.*s\" );\n\n", classNameLength, pClassName );
    fprintf( pSourceFileHandle, "typedef id\t%s_t;\n", pLowerClassName );
    fprintf( pSourceFileHandle, "typedef id\tnsobject_t;\n" );
    fprintf( pSourceFileHandle, "typedef SEL\tnsselector_t;\n\n" );
}

void writeCFunctionImplementation( FILE* pSourceFileHandle, const CFunctionDefinition* pFunctionDefinition )
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

    fprintf( pSourceFileHandle, "%s_%s( %s_t object", pFunctionDefinition->pLowerClassName, pFunctionDefinition->pResolvedFunctionName, pFunctionDefinition->pLowerClassName );

    for( uint8_t argumentIndex = 0u; argumentIndex < pFunctionDefinition->argumentCount; ++argumentIndex )
    {
        fprintf( pSourceFileHandle, ", %s arg%u", pFunctionDefinition->pResolvedArgumentTypes[ argumentIndex ], argumentIndex );
    }

    fprintf( pSourceFileHandle, " )\n{\n" );
    fprintf( pSourceFileHandle, "\tstatic SEL methodSelector = sel_registerName( \"%s\" );\n", pFunctionDefinition->pOriginalFunctionName );
    fprintf( pSourceFileHandle, "\tstatic IMP methodImplementation = class_getMethodImplementation( internalClassObject, methodSelector );\n" );
    fprintf( pSourceFileHandle, "\ttypedef %s( *MethodFunctionPtr )( id, SEL", pFunctionDefinition->pResolvedReturnType );

    for( uint8_t argumentIndex = 0u; argumentIndex < pFunctionDefinition->argumentCount; ++argumentIndex )
    {
        const char* pArgumentType = pFunctionDefinition->pResolvedArgumentTypes[ argumentIndex ];
        fprintf( pSourceFileHandle, ", %s", pArgumentType );
    }

    fprintf( pSourceFileHandle, " );\n\n" );
    fprintf( pSourceFileHandle, "\tMethodFunctionPtr impl = ( MethodFunctionPtr )methodImplementation;\n" );
    fprintf( pSourceFileHandle, "\t" );

    const uint8_t hasReturnValue = !areCStringsEqual( pFunctionDefinition->pResolvedReturnType, "void" );
    if( hasReturnValue )
    {
        fprintf( pSourceFileHandle, "return " );
    }

    fprintf( pSourceFileHandle, "impl( (id)object, methodSelector");
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
    writeCSourcePrefix( pParseArguments->pSourceFileHandle, pParseArguments->pHeaderFileName, pClassNameStart, classNameLength );

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
                            writeCFunctionImplementation( pParseArguments->pSourceFileHandle, &functionDefinition );
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