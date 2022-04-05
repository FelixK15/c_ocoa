#!/bin/bash

ScriptDirectory="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
BuildConfig=$1

if [ -z ${BuildConfig} ] 
then 	
		echo "Missing argument, assuming release build"
		BuildConfig=release
fi

InvalidBuildConfig=1
if [ "${BuildConfig}" == "release" -o "${BuildConfig}" == "debug" ]
then
	InvalidBuildConfig=0
fi

if [ ${InvalidBuildConfig} == 1 ] 
then
	echo "Invalid build config '${BuildConfig}'. Assuming release build"
	BuildConfig=release
fi

BuildFolder="${ScriptDirectory}/build_${BuildConfig}"
if [ ! -d "${BuildFolder}" ] 
then
	mkdir "${BuildFolder}"
fi

OutputFile="${BuildFolder}/osx_c_coca_api_generator"
InputCFile="${ScriptDirectory}/osx_entry.c"
Libraries="-lobjc"
Frameworks="-framework CoreFoundation -framework AppKit"

CompilerOptions="-ferror-limit=900 -fstrict-aliasing --output ${OutputFile} ${Libraries} ${Frameworks}"
if [ "${BuildConfig}" == "release" ] 
then
	echo "Build config = release"
	CompilerOptions="${CompilerOptions} -O3"
fi

if [ "${BuildConfig}" == "debug" ] 
then
	echo "Build config = debug"
	CompilerOptions="${CompilerOptions} --debug"
fi

BuildCommand="clang ${InputCFile} ${CompilerOptions}"
${BuildCommand}
