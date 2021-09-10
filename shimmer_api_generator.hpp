struct ObjCTypeDictEntry
{
    char* pOriginalType         = NULL;
    const char* pResolvedType   = NULL;

    uint8_t isNew               = 1u;
};

struct ObjCTypeDict
{
    ObjCTypeDictEntry* pEntries;
    uint32_t           entryCapacity;
};

struct CommandLineParseResult
{
    const char* pTestFilePath = NULL;
};

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
    memset( pMemory, 0u, sizeInBytes );
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

    return ( *pStringA == *pStringB && *pStringA == 0 );
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

inline char* convertCStringToLower( char* pString, const int32_t stringLength )
{
    for( int32_t charIndex = 0u; charIndex < stringLength; ++charIndex )
    {
        pString[ charIndex ] = tolower( pString[ charIndex ] );
    }
    return pString;
}

inline char* convertCStringToLower( char* pDestination, const char* pSource, const int32_t sourceLength )
{
    for( int32_t charIndex = 0u; charIndex < sourceLength; ++charIndex )
    {
        pDestination[ charIndex ] = tolower( pSource[ charIndex ] );
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
    return convertCStringToLower( pStringCopyMemory, stringLength );
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

CommandLineParseResult parseCommandLineArguments( const int argc, const char** argv )
{
    enum ParseState
    {
        NextArgument,
        ParseTestArg
    };

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
    memset( pTypeDict->pEntries, 1, sizeInBytes );
    return pTypeDict;
}

uint8_t stringsAreEqual( const char* pStringA, const char* pStringB, const int32_t stringLength )
{
    for( int32_t charIndex = 0u; charIndex < stringLength; ++charIndex )
    {
        if( pStringA[ charIndex ] != pStringB[ charIndex ] )
        {
            return false;
        }
    }

    return true;
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
    memset(pTempMemory, 0, 1024);

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

    printf( "Unknown base type identifier '%c'\n", *pObjectiveCTypeName );
    return NULL;
}

const char* resolveStructType( ObjCTypeDict* pTypeDict, const char* pTypeName, int32_t typeNameLength )
{
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
        if( isspace( *pText ) )
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
        if( isspace( *pText ) )
        {
            ++pText;
            break;
        }

        --pText;
    }

    return pText;
}

struct ParseResult
{
    char* pReturnType;
    char* pFunctionName;
    char* pArguments;

    int32_t returnTypeLength;
    int32_t functionNameLength;
    int32_t argumentLength;
};

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

void writeCFunctionDeclaration( FILE* pResultFileHandle, const char* pLowerClassName, const char* pResolvedReturnType, const char* pFunctionName, const char** pResolvedArgumentTypes, int32_t argumentCount )
{   
    //FK: "guess" maximum tab count for nice formatting
    //    getting the correct tab count for the longest
    //    return type name would be too involved since
    //    we currently run on a function by function basis
    //    and thus don't know the maximum type name length
    //    beforehand
    const size_t maxTabCount = 5u;
    size_t returnTypeLength = 0u;

    const uint8_t isInitFunction = ( areCStringsEqual( pFunctionName, "init" ) || areCStringsEqual( pFunctionName, "initWithCoder" ) );
    if( isInitFunction )
    {
        //FK: little syntactic sugar, return correct type for init function(s)
        returnTypeLength = fprintf( pResultFileHandle, "%s_t ", pLowerClassName );
    }
    else
    {
        returnTypeLength = fprintf( pResultFileHandle, "%s ", pResolvedReturnType );
    }

    const size_t spacesForTab = 4u;
    const size_t tabCountForReturnType = getMax( 0u, maxTabCount - ( returnTypeLength / spacesForTab ) );
    for( size_t tabIndex = 0u; tabIndex < tabCountForReturnType; ++tabIndex )
    {
        fprintf( pResultFileHandle, "\t" );
    }

    fprintf( pResultFileHandle, "%s_%s( %s_t object", pLowerClassName, pFunctionName, pLowerClassName );

    for( int32_t argumentIndex = 0u; argumentIndex < argumentCount; ++argumentIndex )
    {
        fprintf( pResultFileHandle, ", %s arg%u", pResolvedArgumentTypes[ argumentIndex ], argumentIndex );
    }

    fprintf( pResultFileHandle, " );\n" );
    fflush( pResultFileHandle );
}

uint8_t convertParseResultToCCode( FILE* pResultFileHandle, ObjCTypeDict* pDict, ParseResult* pParseResult, const char* pLowerClassName )
{
    if( !isValidFunctionName( pParseResult->pFunctionName ) )
    {
        return 0u;
    }

    const char* pResolvedReturnType = resolveObjectiveCTypeName( pDict, pParseResult->pReturnType, pParseResult->returnTypeLength );
    if( pResolvedReturnType == NULL )
    {
        return 0u;
    }

    const char* pFunctionName = convertToCFunctionName( pParseResult->pFunctionName, pParseResult->functionNameLength );

    const char* pArguments = pParseResult->pArguments;
    const char* pResolvedArgumentTypes[32] = {};
    
    uint8_t argumentCount = 0u;
    constexpr uint8_t argumentsToSkip = 2; //FK: Skip first two arguments since these are always the object + the selector.
    while( *pArguments )
    {
        const char* pArgumentStart = pArguments;
        const char* pArgumentEnd = findNextWhitespace( pArgumentStart );
        const int32_t argumentLength = castSizeToInt32( pArgumentEnd - pArgumentStart );

        if( argumentCount >= argumentsToSkip )
        {
            const uint8_t argumentIndex = argumentCount - argumentsToSkip;
            RuntimeAssert( argumentIndex < ArrayCount( pResolvedArgumentTypes ) );

            const char* pArgumentType = resolveObjectiveCTypeName( pDict, pArgumentStart, argumentLength );
            RuntimeAssert( pArgumentType != NULL );

            pResolvedArgumentTypes[ argumentIndex ] = pArgumentType;
        }

        ++argumentCount;

        if( *pArgumentEnd == 0 )
        {
            break;
        }

        pArguments = pArgumentEnd + 1;
    }
    
    writeCFunctionDeclaration( pResultFileHandle, pLowerClassName, pResolvedReturnType, pFunctionName, pResolvedArgumentTypes, ( argumentCount - argumentsToSkip ) );
    return 1u;
}

void parseTestFile( FILE* pResultFileHandle, const char* pFileContentBuffer, size_t fileContentBufferSizeInBytes )
{
    constexpr size_t dictSizeInBytes = 1024*1024; //FK: 1 MiB
    ObjCTypeDict* pDict = createObjectiveCTypeDictionary( dictSizeInBytes );
    if( pDict == NULL )
    {
        return;
    }

    const char* pClassNameEnd = findNextCharacterPositionInCString( pFileContentBuffer, ':' );
    const char* pClassNameStart = findPreviousWhitespace( pFileContentBuffer, pClassNameEnd );

    const int32_t classNameLength = castSizeToInt32( pClassNameEnd - pClassNameStart );
    char* pLowerClassName = (char*)malloc( classNameLength + 1 );
    convertCStringToLower( pLowerClassName, pClassNameStart, classNameLength );

    pFileContentBuffer = findNextNewline( pFileContentBuffer ) + 1;

    enum State
    {
        EatWhitespace = 0,
        EatUntilNewLine,
        ParseReturnType,
        ParseArgumenType,
        ParseFunctionName
    };

    constexpr size_t stringBufferSizeInBytes = 1024u*32u; //FK: 32KiB
    char* pStringBuffer = (char*)calloc( 1u, stringBufferSizeInBytes );
    size_t stringBufferOffset = 0u;
    //FK: Temporary parse struct to store strings for easier parsing later
    ParseResult parseResult = {};

    constexpr uint8_t ArgumentsToSkip = 2;

    uint8_t argumentCount = 0u;
    State state = ParseReturnType;
    State previousState = ParseReturnType;
    while( *pFileContentBuffer != 0 )
    {
        switch( state )
        {
            case EatWhitespace:
            {
                if( !isspace( *pFileContentBuffer ) )
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
                    convertParseResultToCCode( pResultFileHandle, pDict, &parseResult, pLowerClassName );
                    fillMemoryWithZeroes( pStringBuffer, stringBufferOffset );
                    stringBufferOffset = 0u;
                    
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
            
                parseResult.pFunctionName = pStringBuffer + stringBufferOffset;

                const int32_t numCharactersWritten = sprintf( pStringBuffer + stringBufferOffset, "%.*s", functionNameLength, pFunctionNameStart );
                parseResult.functionNameLength = numCharactersWritten;
                stringBufferOffset += numCharactersWritten + 1;
                pFileContentBuffer += ( pNextWhiteSpacePosition - pFunctionNameStart );
                state = EatWhitespace;
                break;
            }

            case ParseReturnType:
            {
                const char* pReturnTypeStart = pFileContentBuffer;
                const char* pReturnTypeEnd   = findNextWhitespace( pReturnTypeStart );
                const int32_t typeLength = castSizeToInt32( pReturnTypeEnd - pReturnTypeStart );

                parseResult.pReturnType = pStringBuffer + stringBufferOffset;

                const int32_t numCharactersWritten = sprintf( pStringBuffer + stringBufferOffset, "%.*s", typeLength, pReturnTypeStart );
                parseResult.returnTypeLength = numCharactersWritten;
                stringBufferOffset += numCharactersWritten + 1;
                state = EatWhitespace;

                pFileContentBuffer += typeLength;

                break;
            }
            case ParseArgumenType:
            {
                const char* pArgumentsStart = pFileContentBuffer;
                const char* pArgumentsEnd = findNextNewline( pArgumentsStart );
                const int32_t argumentLength = castSizeToInt32( pArgumentsEnd - pArgumentsStart );
               
                parseResult.pArguments = pStringBuffer + stringBufferOffset;

                const int32_t numCharactersWritten = sprintf( pStringBuffer + stringBufferOffset, "%.*s", argumentLength, pArgumentsStart );
                parseResult.argumentLength = numCharactersWritten;
                stringBufferOffset += numCharactersWritten + 1;
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
}