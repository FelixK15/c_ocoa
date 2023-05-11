## C-OCOA Code Generator ##
This project is a code generator that generates C code that can ultimately be used to program against the Apple Cocoa API without having to use Objective-C.
More details about this can be found on [this blog post](https://felixk15.github.io/posts/c_ocoa/).

## How to build ##
To use this project, you have to build it for yourself. This is because for the project to be able to extract information from the Cocoa API to generate to C API, it has to have to access to the Framework that you want to generate C code for. Eg: If you want to generate the C API for the `GLKit` framework, you have to build this project with the `-Framework GLKit` compiler option (or the equivalent XCode flag).

### OSX ###
You can either use the XCode project to build this project or the command line.
If you want to use the commandline, you can use the [`osx/build.sh`](osx/build.sh) script. The linked frameworks can be changed in line 33 of the script. To be able to build the program, XCode has to be installed and `clang` has to be installed (should come with XCode).

### iOS ###
To be able to use this project for iOS, you are bound to use the XCode project. You don't have to have an iOS device to be able to use this project for iOS - you can simply use a iOS simulator.

## How to use ##
Once the project is build, you can run the executable without any parameter. In that case it will generate the API for *all* classes that are contained within the framework that the executable has been build with. If you're only interested in a subset of the available classes you can specify one or more filters as arguments when running the executable (wildcards using `*` are also supported).

eg: If you're only interested in classes that start with `NS` you can start the executable with this argument:

`c_ocoa_generator NS*`

The project has additional support for these options:
```
    -o <output>     | Specifies where to write the generated C files. Default: Working Directory
    -p <prefix>     | Specifies a prefix that should be added to the name of the generated C files. Default: No prefix
```

eg: This will generate the C API for classes that start with `CG`. The generated C files will be written to the `~/c_ocoa/output/` directory and all generated files will be prefixed with `awesome_company_`.

`c_ocoa_generator -o ~/c_ocoa/output/ -p awesome_company_ CG*`

> *Note*: When running the executable on an iOS or iOS Simulator, you have to make sure that the code generator only writes to folders that it has write-access to

Once the code has been generated you can use it by including the generated `*.h` files and adding the generated `*.c` files to your project.