struct ObjCTypeDictEntry
{
    char* pOriginalType         = nullptr;
    const char* pResolvedType   = nullptr;

    size_t originalTypeLength   = 0u;
    uint8_t isNew               = 1u;
};

struct ObjCTypeDict
{
    ObjCTypeDictEntry* pEntries;
    uint32_t           entryCapacity;
};

static uint32_t djb2_hash( const char* pString )
{
    uint32_t hash = 5381;

    for(;;)
    {
        if( pString == 0 )
        {
            break;
        }

        const char c = *pString++;
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}



ObjCTypeDictEntry* insertTypeDictionaryEntry( ObjCTypeDict* pDict, const char* restrict_modifier pTypeName, uint8_t* restrict_modifier pOutIsNew )
{
    //FK: TODO: Check hash distribution
    const uint32_t hashValue = djb2_hash( pTypeName );
    const uint32_t entryIndex = hashValue % pDict->entryCapacity;
    ObjCTypeDictEntry* pEntry = pDict->pEntries + entryIndex;

    *pOutIsNew = pEntry->isNew;
    if( pEntry->isNew )
    {
        const size_t typeNameLength = strlen( pTypeName ); 
        pEntry->originalTypeLength = typeNameLength;

        //FK: TODO: use stringpool?
        pEntry->pOriginalType = (char*)malloc( typeNameLength + 1 ); 
        if( pEntry->pOriginalType == nullptr )
        {
            //FK: out of memory.
            //FK: TODO: Print message, let the user know what's going on
            return nullptr;
        }
        strcpy( pEntry->pOriginalType, pTypeName );
        pEntry->isNew = 0;

        //FK:
    }

    return pEntry;
}

const char* parseObjectiveCStruct( const char* pTypeName, const size_t typeNameLength )
{
    UnusedArgument(pTypeName);
    UnusedArgument(typeNameLength);

    //FK: Optimistically alloca 1KiB on the stack to have 
    //    a big enough scratchpad to resolve the typename
    char* pTempMemory = (char*)alloca(1024);
    memset(pTempMemory, 0, 1024);

    return nullptr;
}

const char* parseObjectiveCType( const char* pTypeName, const size_t typeNameLength )
{
    UnusedArgument(pTypeName);
    UnusedArgument(typeNameLength);
    return nullptr;
}

const char* resolveObjectiveCTypeName( ObjCTypeDict* pTypeDict, const char* pTypeName, const size_t typeNameLength )
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
        }

        InvalidCodePath();
        return "";
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
        sprintf( pTypeNameCopy, "%s", pTypeName );
        
        //FK: struct
        pTypeNameCopy += 2; //FK: Skip '{_'
        char* pTypeNameCopyEnd = strstr( pTypeNameCopy, "=" );
        *pTypeNameCopyEnd = 0;

        uint8_t isNew = 0;
        ObjCTypeDictEntry* pTypeDictEntry = insertTypeDictionaryEntry( pTypeDict, pTypeNameCopy, &isNew );
        if( isNew )
        {
            pTypeDictEntry->pResolvedType = parseObjectiveCStruct( pTypeName, typeNameLength );
        }

        return pTypeDictEntry->pResolvedType;
    }

    uint8_t isNew = 0;
    ObjCTypeDictEntry* pTypeDictEntry = insertTypeDictionaryEntry( pTypeDict, pTypeName, &isNew );
    if( pTypeDictEntry->isNew )
    {
        pTypeDictEntry->pResolvedType = parseObjectiveCType( pTypeName, typeNameLength );
    }

    return pTypeDictEntry->pResolvedType;
}