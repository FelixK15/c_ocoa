#!/bin/bash

iosSimulationSysRoot="$( xcrun --sdk iphonesimulator --show-sdk-path )"
iosSysRoot="$( xcrun --sdk iphoneos --show-sdk-path )"

scriptPath="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
buildMode=$1

projectName="C-Ocoa_Generator"
executableName="c_ocoa_generator_ios"
bundleId="com.shimmer.c_ocoa_generator"

sourceFileDirectory="$scriptPath/"
sourceFiles=("c_ocoa_generator_ios.c")

iconName="shimmer"
version="1"
subVersion="0"

function echo_error()
{
    echo -e "\033[0;31\x1b[1mERROR:\x1b[0m $1"
}

function echo_success()
{
    echo -e "\033[0;32m\x1b[1mSUCCESS:\x1b[0m $1"
}

function echo_warning()
{
    echo -e "\033[0;33m\x1b[1mWARNING:\x1b[0m $1" 
}

function getBuildPath()
{
    echo "$scriptPath/build/$buildMode"
}

function getBuildOutputPath()
{
    local buildPath=$(getBuildPath $buildMode)
    echo "$buildPath/$projectName.app"
}

function getBuildArtifactsPath()
{
    local buildPath=$(getBuildPath $buildMode)
    echo "$buildPath/artifacts"
}

function createBuildDirectories()
{
    local buildFolder=$(getBuildPath)
    if ! [ -d $buildFolder ]; then
        mkdir -p $buildFolder
    fi

    local buildArtifactsPath=$(getBuildArtifactsPath)
    if ! [ -d $buildArtifactsPath ]; then
        mkdir -p $buildArtifactsPath
    fi

    local buildOutputPath=$(getBuildOutputPath)
       if ! [ -d $buildOutputPath ]; then
        mkdir -p $buildOutputPath
    fi
}

function generateInfoPlist()
{
    local buildPath=$(getBuildOutputPath)
    local infoPlistPath="$buildPath/Info.plist"

    echo
    echo "Generating Info.plist..."

    #FK: Remove old manifest
    if [ -f $infoPlistPath ]; then
        rm $infoPlistPath
    fi

    #FK: Create new manifest from template
    if ! cp "$scriptPath/Info.plist.template" $infoPlistPath; then
        echo_error "Couldn't create Info.plist"
        return $errorValue
    fi

    local versionString="$version.$subVersion"

    #FK: Replace template arguments in manifest
    if ! sed -i.bk 's/%MAJORVERSION%/'$version'/' $infoPlistPath; then
        echo_warning "Couldn't find '%MAJORVERSION% template argument in Info.plist file $infoPlistPath"
    fi

    if ! sed -i.bk 's/%MINORVERSION%/'$subVersion'/' $infoPlistPath; then
        echo_warning "Couldn't find '%MINORVERSION% template argument in Info.plist file $infoPlistPath"
    fi

    if ! sed -i.bk 's/%EXECUTABLENAME%/'$executableName'/' $infoPlistPath; then
        echo_warning "Couldn't find '%EXECUTABLENAME% template argument in Info.plist file $manifestPath"
    fi
    
    if ! sed -i.bk 's/%VERSIONSTRING%/'$versionString'/' $infoPlistPath; then
        echo_warning "Couldn't find '%VERSIONSTRING% template argument in Info.plist file $manifestPath"
    fi

    if ! sed -i.bk 's/%BUNDLEID%/'$bundleId'/' $infoPlistPath; then
        echo_warning "Couldn't find '%BUNDLEID% template argument in Info.plist file $manifestPath"
    fi

    if ! sed -i.bk 's/%PROJECTNAME%/'$projectName'/' $infoPlistPath; then
        echo_warning "Couldn't find '%PROJECTNAME% template argument in Info.plist file $manifestPath"
    fi

    #FK: on OSX we have to supply the -i command with a backup name...We don't need a backup so we just delete the backup afterwards..
    rm "$infoPlistPath.bk"

    echo_success "Generated Info.plist!"

    return $successValue
}

function compileUsingMakeFile()
{
    local makeFilePath="$(getBuildArtifactsPath)"
    local makeCommand="make"
    
    pushd $makeFilePath
    eval $makeCommand
    popd

    return $?
}

function generateMakeFile()
{
    local makeFilePath="$(getBuildArtifactsPath)/makefile"
    local objectOutputPath="$(getBuildArtifactsPath)/obj"
    local outputPath="$(getBuildOutputPath)/$executableName"
    local libraries="-lobjc"
    local sysroot="-isysroot $iosSimulationSysRoot"
    local frameworks="-framework UiKit -framework CoreFoundation -framework OpenGLES"
    local compilerArgs="-ferror-limit=900 -Wno-deprecated-declarations -working-directory=${sourceFileDirectory} -fstrict-aliasing ${sysroot}"
    local linkerArgs="${sysroot} ${frameworks} ${libraries}"
    local objectFiles=()

    mkdir -p $objectOutputPath

    if [ "${buildMode}" == "release" ] 
    then
	    echo "Build config = release"
	    compilerArgs="$compilerArgs -O3"
    fi

    if [ "${buildMode}" == "debug" ] 
    then
	    echo "Build config = debug"
	    compilerArgs="$compilerArgs --debug"
    fi

    for sourceFile in ${sourceFiles[@]}; do
        local sourceFileLength=${#sourceFile}
        objectFile="${sourceFile:0:${sourceFileLength} - 2}.o"
        objectFiles+="$objectOutputPath/$objectFile "
    done

    echo "CC=clang"                                                 >  $makeFilePath
    echo "CFLAGS=$compilerArgs"                                     >> $makeFilePath
    echo "LDFLAGS=$linkerArgs"                                      >> $makeFilePath
    echo "SRCDIR=$sourceFileDirectory"                              >> $makeFilePath
    echo "OBJDIR=$objectOutputPath"                                 >> $makeFilePath
    echo "EXECUTABLE=$outputPath"                                   >> $makeFilePath
    echo "OBJECTS:=${objectFiles[@]}"                               >> $makeFilePath
    echo "\$(EXECUTABLE): \$(OBJECTS)"                              >> $makeFilePath
    echo $'\t'"\$(CC) \$(LDFLAGS) \$(OBJECTS) -o \$@ "              >> $makeFilePath
    echo "\$(OBJDIR)/%.o: \$(SRCDIR)/%.c"                           >> $makeFilePath
    echo $'\t'"\$(CC) \$(CFLAGS) -c $< -o \$@"                      >> $makeFilePath
    echo "clear:"                                                   >> $makeFilePath
    echo $'\t'"rm -rf $objectOutputPath"                            >> $makeFilePath
}

if [ -z ${buildMode} ] 
then 	
		echo "Missing argument, assuming release build"
		buildMode=release
fi

invalidBuildMode=1
if [ "${buildMode}" == "release" -o "${buildMode}" == "debug" ]
then
	invalidBuildMode=0
fi

if [ ${invalidBuildMode} == 1 ] 
then
	echo "Invalid build config '${buildMode}'. Assuming release build"
	buildMode=release
fi

echo "Building using build mode '$buildMode'"

createBuildDirectories
generateInfoPlist
generateMakeFile

if ! compileUsingMakeFile; then
    echo
    echo_error "Compilation failed, aborting apk building process"
    exit
fi

echo_success "Successfully build app '${projectName}'"