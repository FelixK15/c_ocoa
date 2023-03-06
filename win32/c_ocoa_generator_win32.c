#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdint.h>
#include <malloc.h>
#include <intrin.h>

#define assert_runtime(x) if(!(x)){ __debugbreak();}
#define code_path_invalid() assert_runtime(0)

#define hook_breakpoint() __nop()

#define restrict_modifier __restrict__

#include "..\c_ocoa_generator.h"

int main( int argc, const char** argv )
{
	if( argc == 1 )
	{
		//FK: TODO: Print help
		printf("Too few arguments.\n");
		return -1;
	}

	c_ocoa_command_line_parse_result c_ocoa_parse_result = command_line_arguments_parse( argc - 1, argv + 1 );
	if( c_ocoa_parse_result.pTestFilePath == NULL )
	{
		printf("Real use only supported under macOS, use --test <testfile> to test implementation under win32.");
		return -1;
	}

	FILE* pTestFileHandle = fopen( c_ocoa_parse_result.pTestFilePath, "r" );
	if( pTestFileHandle == NULL )
	{
		printf("Couldn't open test file '%s' for reading.", c_ocoa_parse_result.pTestFilePath );
		return -1;
	}

	//FK: TODO: mmap file
	fseek(pTestFileHandle, 0, SEEK_END);
	const size_t fileSizeInBytes = ftell(pTestFileHandle);
	fseek(pTestFileHandle, 0, SEEK_SET);

	char* pFileBuffer = (char*)malloc( fileSizeInBytes );
	if( pFileBuffer == NULL )
	{
		printf("Couldn't allocate %.3fKiB as file buffer.", (float)fileSizeInBytes/1024.f);
		return -1;
	}

	fread( pFileBuffer, 1u, fileSizeInBytes, pTestFileHandle );
	fclose( pTestFileHandle );
	pTestFileHandle = NULL;

	c_ocoa_objc_conversion_arguments arguments;
	if( !conversion_arguments_create( &arguments, "test_out.h", "test_out.c" ) )
	{
		return -1;
	}

	parseTestFile( &arguments, pFileBuffer, fileSizeInBytes );
	return 0;
}