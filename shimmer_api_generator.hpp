struct ObjCTypeDictEntry
{
    char* pOriginalType         = nullptr;
    const char* pResolvedType   = nullptr;

    uint8_t isNew               = 1u;
};

struct ObjCTypeDict
{
    ObjCTypeDictEntry* pEntries;
    uint32_t           entryCapacity;
};

struct CommandLineParseResult
{
    const char* pTestFilePath = nullptr;
};

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
                if( strcmp( argv[argIndex], "--test" ) == 0 )
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

uint8_t stringsAreEqual( const char* pStringA, const char* pStringB, size_t stringLength )
{
    for( size_t i = 0u; i < stringLength; ++i )
    {
        if( pStringA[ i ] != pStringB[ i ] )
        {
            return false;
        }
    }

    return true;
}

uint32_t djb2_hash( const char* pString, const size_t stringLength )
{
    uint32_t hash = 5381;

    while( *pString )
    {
        const char c = *pString++;
        hash = ((hash << 5) + hash) + c;
    }
        
    return hash;
}

ObjCTypeDictEntry* insertTypeDictionaryEntry( ObjCTypeDict* pDict, const char* restrict_modifier pTypeName, const size_t typeNameLength, uint8_t* restrict_modifier pOutIsNew )
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
        if( pEntry->pOriginalType == nullptr )
        {
            //FK: out of memory.
            //FK: TODO: Print message, let the user know what's going on
            return nullptr;
        }
        sprintf( pEntry->pOriginalType, "%.*s", (int)typeNameLength, pTypeName );
        pEntry->isNew = 0;
    }

    //FK: Check for hash collision
    RuntimeAssert( stringsAreEqual( pEntry->pOriginalType, pTypeName, typeNameLength ) );

    return pEntry;
}

const char* parseObjectiveCStruct( const char* pTypeName, const size_t typeNameLength )
{
    char* pTemp = (char*)malloc( typeNameLength + 1 );
    sprintf( pTemp, "%.*s", (int)typeNameLength, pTypeName );
    return pTemp;

    UnusedArgument(pTypeName);
    UnusedArgument(typeNameLength);

    //FK: Optimistically alloca 1KiB on the stack to have 
    //    a big enough scratchpad to resolve the typename
    char* pTempMemory = (char*)alloca(1024);
    memset(pTempMemory, 0, 1024);

    return pTypeName;
}

const char* parseObjectiveCType( const char* pTypeName, const size_t typeNameLength )
{
    char* pTemp = (char*)malloc( typeNameLength + 1 );
    sprintf( pTemp, "%.*s", (int)typeNameLength, pTypeName );
    return pTemp;
}

const char* resolveObjectiveCTypeName( ObjCTypeDict* pTypeDict, const char* pTypeName, size_t typeNameLength )
{
    if( typeNameLength == 1u )
    {
        switch( *pTypeName )
        {
            case '@':
                return "nsobject_t";

            case 'Q':
                return "uint32_t";

            case 'S':
                return "uint16_t";

            case 'c':
                return "uint8_t";

            case 'r':
            case '^':
                return "*"; //FK: Reference

            case '*':
                return "char*";

            case 'v':
                return "void";

            case ':':
                return "nssignal_t";
        }

        return nullptr;
    }
    else if( typeNameLength == 2u )
    {
        if( strcmp( pTypeName, "r*" ) == 0u )
        {
            return "const char*";
        }
    }

    if( *pTypeName == '{' )
    {
        //FK: Make copy to manipulate string for easier, human-readible lookup 
        char* pTypeNameCopy = (char*)alloca( typeNameLength );
        sprintf( pTypeNameCopy, "%.*s", (int)typeNameLength, pTypeName );
        
        //FK: struct
        if( *pTypeNameCopy == '{' )
        {
            ++pTypeNameCopy;
        }

        if( *pTypeNameCopy == '_' )
        {
            ++pTypeNameCopy;
        }

        char* pTypeNameCopyEnd = strstr( pTypeNameCopy, "=" );
        RuntimeAssert( pTypeNameCopyEnd != nullptr );
        *pTypeNameCopyEnd = 0;

        typeNameLength = ( pTypeNameCopyEnd - pTypeNameCopy );

        uint8_t isNew = 0;
        ObjCTypeDictEntry* pTypeDictEntry = insertTypeDictionaryEntry( pTypeDict, pTypeNameCopy, typeNameLength, &isNew );
        if( isNew )
        {
            pTypeDictEntry->pResolvedType = parseObjectiveCStruct( pTypeName, typeNameLength );
        }

        return pTypeDictEntry->pResolvedType;
    }

    uint8_t isNew = 0;
    ObjCTypeDictEntry* pTypeDictEntry = insertTypeDictionaryEntry( pTypeDict, pTypeName, typeNameLength, &isNew );
    if( isNew )
    {
        pTypeDictEntry->pResolvedType = parseObjectiveCType( pTypeName, typeNameLength );
    }

    return pTypeDictEntry->pResolvedType;
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

void convertToLower( char* pDestination, const char* pSource, size_t sourceLength )
{
    for( size_t i = 0u; i < sourceLength; ++i )
    {
        pDestination[ i ] = tolower( pSource[i] );
    }
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
    if( pObjectiveCFunctionName[ functionNameLength - 1 ] == ':' )
    {
        pObjectiveCFunctionName[ functionNameLength - 1 ] = 0;
    }

    return pObjectiveCFunctionName;
}

void writeCFunctionDeclaration( FILE* pResultFileHandle, const char* pLowerClassName, const char* pResolvedReturnType, const char* pFunctionName, const char** pResolvedArgumentTypes, int32_t argumentCount )
{
    fprintf( pResultFileHandle, "%s %s_%s( nsobject_t %s_object", pResolvedReturnType, pLowerClassName, pFunctionName, pLowerClassName );

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
    if( pResolvedReturnType == nullptr )
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
        const int32_t argumentLength = (int32_t)(pArgumentEnd - pArgumentStart);

        if( argumentCount >= argumentsToSkip )
        {
            const uint8_t argumentIndex = argumentCount - argumentsToSkip;
            RuntimeAssert( argumentIndex < ArrayCount( pResolvedArgumentTypes ) );

            pResolvedArgumentTypes[ argumentIndex ] = resolveObjectiveCTypeName( pDict, pArgumentStart, argumentLength );
        }

        if( *pArgumentEnd == 0 )
        {
            break;
        }

        ++argumentCount;
        pArguments = pArgumentEnd + 1;
    }

    writeCFunctionDeclaration( pResultFileHandle, pLowerClassName, pResolvedReturnType, pFunctionName, pResolvedArgumentTypes, ( argumentCount - argumentsToSkip ) );
    return 1u;
}

void parseTestFile( FILE* pResultFileHandle, const char* pFileContentBuffer, size_t fileContentBufferSizeInBytes )
{
    constexpr size_t dictSizeInBytes = 1024*1024; //FK: 1 MiB
    ObjCTypeDict* pDict = createObjectiveCTypeDictionary( dictSizeInBytes );
    if( pDict == nullptr )
    {
        return;
    }

    //FK: 
    const char* pClassNameEnd = strstr( pFileContentBuffer, ":" );
    const char* pClassNameStart = findPreviousWhitespace( pFileContentBuffer, pClassNameEnd );

    const size_t classNameLength = ( pClassNameEnd - pClassNameStart );
    char* pLowerClassName = (char*)malloc( classNameLength + 1 );
    convertToLower( pLowerClassName, pClassNameStart, classNameLength );

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
                    memset( pStringBuffer, 0u, stringBufferOffset );
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
                const char* pNextWhiteSpacePosition = strstr( pFunctionNameStart, " " );
                const char* pNextColonPosition      = strstr( pFunctionNameStart, ":" );

                //FK: Ignore function name extension naming optional arguments
                //    eg: nextEventMatchingMask:untilDate:inMode:dequeue:
                size_t functionNameLength = ( pNextWhiteSpacePosition - pFunctionNameStart );
                if( pNextColonPosition < pNextWhiteSpacePosition )
                {
                    functionNameLength = ( pNextColonPosition - pFunctionNameStart );
                }
                
                parseResult.pFunctionName = pStringBuffer + stringBufferOffset;

                const int32_t numCharactersWritten = sprintf( pStringBuffer + stringBufferOffset, "%.*s", (int)functionNameLength, pFunctionNameStart );
                parseResult.functionNameLength = numCharactersWritten;
                stringBufferOffset += numCharactersWritten + 1;
                pFileContentBuffer += functionNameLength;
                state = EatWhitespace;
                break;

#if 0
                //FK: Omit trailing ':' from function name;
                if( pFunctionNameStart[ functionNameLength ] == ':' )
                {
                    --functionNameLength;
                }

                //FK: Omit leading '.' (for internal functions...?)
                if( *pFunctionNameStart == '.' )
                {
                    ++pFunctionNameStart;
                    --functionNameLength;
                }
#endif
            }

            case ParseReturnType:
            {
                const char* pReturnTypeStart = pFileContentBuffer;
                const char* pReturnTypeEnd   = findNextWhitespace( pReturnTypeStart );
                const int32_t typeLength = ( int32_t )( pReturnTypeEnd - pReturnTypeStart );

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
                const int32_t argumentLength = ( int32_t )( pArgumentsEnd - pArgumentsStart );
               
                parseResult.pArguments = pStringBuffer + stringBufferOffset;

                const int32_t numCharactersWritten = sprintf( pStringBuffer + stringBufferOffset, "%.*s", argumentLength, pArgumentsStart );
                parseResult.argumentLength = numCharactersWritten;
                stringBufferOffset += numCharactersWritten + 1;
                state = EatUntilNewLine;

                pFileContentBuffer += argumentLength;

                break;

#if 0
                ++argumentCount;
                if( state == ParseArgumenType && argumentCount <= ArgumentsToSkip )
                {
                    state = EatWhitespace;
                    pFileContentBuffer += typeLength;
                    break;
                }

                const char* pResolvedType = resolveObjectiveCTypeName( pDict, pFileContentBuffer, typeLength );
                if( pResolvedType == nullptr )
                {
                    printf( "Could not resolve type '%.*s'\n", (int)typeLength, pFileContentBuffer );
                    state = EatUntilNewLine;
                    break;
                }

                if( state == ParseArgumenType )
                {
                    fprintf( pResultFileHandle, ", %s arg%d", pResolvedType, ( argumentCount - ArgumentsToSkip ) );
                }
                else
                {
                    fprintf( pResultFileHandle, "%s\t", pResolvedType );
                }
                
                pFileContentBuffer += typeLength;
                state = EatWhitespace;
                break;
#endif
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