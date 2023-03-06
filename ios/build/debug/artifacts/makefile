CC=clang
CFLAGS=-ferror-limit=900 -Wno-deprecated-declarations -working-directory=/Users/felix/Documents/repos/C-Ocoa-CodeGenerator/ios/ -fstrict-aliasing -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator15.2.sdk --debug
LDFLAGS=-isysroot /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator15.2.sdk -framework UiKit -framework CoreFoundation -framework OpenGLES -lobjc
SRCDIR=/Users/felix/Documents/repos/C-Ocoa-CodeGenerator/ios/
OBJDIR=/Users/felix/Documents/repos/C-Ocoa-CodeGenerator/ios/build/debug/artifacts/obj
EXECUTABLE=/Users/felix/Documents/repos/C-Ocoa-CodeGenerator/ios/build/debug/C-Ocoa_Generator.app/c_ocoa_generator_ios
OBJECTS:=/Users/felix/Documents/repos/C-Ocoa-CodeGenerator/ios/build/debug/artifacts/obj/c_ocoa_generator_ios.o 
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ 
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@
clear:
	rm -rf /Users/felix/Documents/repos/C-Ocoa-CodeGenerator/ios/build/debug/artifacts/obj
