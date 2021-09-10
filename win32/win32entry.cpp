#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdint.h>
#include <malloc.h>
#include <intrin.h>

#define getMax(a,b) (a)>(b)?(a):(b)
#define getMin(a,b) (a)>(b)?(b):(a)

#define ArrayCount(x) (sizeof(x)/sizeof((x)[1]))
#define RuntimeAssert(x) if(!(x)){ __debugbreak();}
#define InvalidCodePath() RuntimeAssert(false)
#define UnusedArgument(x) (void)x

#define BreakpointHook() __nop()

#define restrict_modifier __restrict__

#include "..\shimmer_api_generator.hpp"

int main( int argc, const char** argv )
{
	if( argc == 1 )
	{
		//FK: TODO: Print help
		printf("Too few arguments.\n");
		return -1;
	}

	CommandLineParseResult parseResult = parseCommandLineArguments( argc - 1, argv + 1 );
	if( parseResult.pTestFilePath == nullptr )
	{
		printf("Real use only supported under macOS, use --test <testfile> to test implementation under win32.");
		return -1;
	}

	FILE* pTestFileHandle = fopen( parseResult.pTestFilePath, "r" );
	if( pTestFileHandle == nullptr )
	{
		printf("Couldn't open test file '%s' for reading.", parseResult.pTestFilePath );
		return -1;
	}

	//FK: TODO: mmap file
	fseek(pTestFileHandle, 0, SEEK_END);
	const size_t fileSizeInBytes = ftell(pTestFileHandle);
	fseek(pTestFileHandle, 0, SEEK_SET);

	char* pFileBuffer = (char*)malloc( fileSizeInBytes );
	if( pFileBuffer == nullptr )
	{
		printf("Couldn't allocate %.3fKiB as file buffer.", (float)fileSizeInBytes/1024.f);
		return -1;
	}

	fread( pFileBuffer, 1u, fileSizeInBytes, pTestFileHandle );
	fclose( pTestFileHandle );
	pTestFileHandle = nullptr;

	FILE* pTestOutFileHandle = fopen( "test_out.hpp", "w" );
	if( pTestOutFileHandle == nullptr )
	{
		printf( "Could not open file 'test_out.hpp' for writing." );
		return -1;
	}

	parseTestFile( pTestOutFileHandle, pFileBuffer, fileSizeInBytes );
	return 0;
}