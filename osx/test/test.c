#include <stdint.h>
#include <stdio.h>
#include <objc/objc.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include <objc/NSObjCRuntime.h>

#include "../generated_files/nsapplication.c"
#include "../generated_files/nswindow.c"
#include "../generated_files/nscolor.c"
#include "../generated_files/nsprocessinfo.c"
#include "../generated_files/nsmenu.c"
#include "../generated_files/nsmenuitem.c"
#include "../generated_files/nsscreen.c"
#include "../generated_files/nsarray.c"
#include "../generated_files/nsobject.c"
#include "../generated_files/nsview.c"
#include "../generated_files/nsopenglpixelformat.c"
#include "../generated_files/nsopenglcontext.c"
#include "../generated_files/nsstring.c"
#include "../generated_files/nsdate.c"
#include "../generated_files/nsevent.c"
#include "../generated_files/uifont.c"

extern id const NSDefaultRunLoopMode;

bool terminated = false;

NSUInteger applicationShouldTerminate(id self, SEL _sel, id sender)
{
	printf("requested to terminate\n");
	terminated = true;
	return 0;
}

int main(int argc, char** argv)
{
    nsobject_t nsapp = nsapplication_sharedApplication();
    nsapplication_setActivationPolicy( nsapp, 0u );

    SEL allocSel = sel_registerName("alloc");
    SEL initSel = sel_registerName("init");

	Class NSObjectClass = objc_getClass("NSObject");
	Class AppDelegateClass = objc_allocateClassPair(NSObjectClass, "AppDelegateOR", 0);
    Protocol* NSApplicationDelegateProtocol = objc_getProtocol("NSApplicationDelegate");
	bool resultAddProtoc = class_addProtocol(AppDelegateClass, NSApplicationDelegateProtocol);
//	assert(resultAddProtoc);
	SEL applicationShouldTerminateSel = sel_registerName("applicationShouldTerminate:");
	bool resultAddMethod = class_addMethod(AppDelegateClass, applicationShouldTerminateSel, (IMP)applicationShouldTerminate, "I@:@");
	
	id dgAlloc = ((id (*)(Class, SEL))objc_msgSend)(AppDelegateClass, allocSel);
	id dg = ((id (*)(id, SEL))objc_msgSend)(dgAlloc, initSel);

	SEL autoreleaseSel = sel_registerName("autorelease");
	((void (*)(id, SEL))objc_msgSend)(dg, autoreleaseSel);

	//[NSApp setDelegate:dg];
    nsapplication_setDelegate( nsapp, dg );

    nsprocessinfo_t processInfo = nsprocessinfo_processInfo();
    nsstring_t processName = nsprocessinfo_processName( processInfo );
    nsstring_t quitTitlePrefix = nsstring_stringWithUTF8String( "Quit " );
    nsstring_t quitTitle = nsstring_stringByAppendingString( quitTitlePrefix, processName );
    
    nsstring_t quitMenuItemKey = nsstring_stringWithUTF8String( "q" );
    nsmenuitem_t quitMenuItem = nsmenuitem_initWithTitle( nsmenuitem_alloc(), quitTitle, sel_registerName("terminate:"), quitMenuItemKey );

    nsmenu_t menu = nsmenu_initWithTitle( nsmenu_alloc(), nsstring_stringWithUTF8String("Hello from C-Ocoa") );
    nsmenu_addItem( menu, quitMenuItem );
    nsapplication_setMainMenu( nsapp, menu );

    nsapplication_finishLaunching( nsapp );

    nsscreen_t mainScreen = nsscreen_mainScreen();
    nsarray_t screens = nsscreen_screens();
    unsigned long long screenCount = nsarray_count( screens );
    nsscreen_t specificScreen = nsarray_objectAtIndex( screens, 0u );
    CGRect frameRect = nsscreen_frame( mainScreen );

    //betray_screen_screen_resolution_x = frameRect.member1.member0;
    //betray_screen_screen_resolution_y = frameRect.member1.member1;

    nswindow_t betrayWindow = nswindow_initWithContentRect( nswindow_alloc(), frameRect, 15, 2, NO );
    
    nsobject_autorelease( betrayWindow );

    nswindow_setReleasedWhenClosed( betrayWindow, NO );

    nsview_t betrayContentView = nswindow_contentView( betrayWindow );
    nsview_setWantsBestResolutionOpenGLSurface( betrayContentView, YES );

    CGPoint point = {0, 0};

//    if(size_x != 0)
//      point.member0 = (betray_screen_screen_resolution_x - size_x) / 2;
//    if(size_y != 0)
//       point.member1 = (betray_screen_screen_resolution_y + size_y) / 2;

    nswindow_cascadeTopLeftFromPoint( betrayWindow, point );

    nsopenglcontext_t openglContextDefault = NULL;
    for(uint32_t i = 0; i < 3; i++)
    {
        uint32_t glAttributes[] = {
		8, 24,
		11, 8,
		5,
		73,
		72,
		55, 1,
        56, 4,
        99, 0x3200, // or 0x1000 0x3200 0x4100
     // 99, 0x4100, // or 0x1000 0x3200 0x4100
		0};
        uint32_t version[] = {0x4100, 0x3200, 0x1000};
        glAttributes[12] = version[i];
        
        nsopenglpixelformat_t pixelFormat = nsopenglpixelformat_initWithAttributes( nsopenglpixelformat_alloc(), glAttributes );
        nsobject_autorelease( pixelFormat );
        
        openglContextDefault = nsopenglcontext_initWithFormat( nsopenglcontext_alloc(), pixelFormat, nil );
        if( openglContextDefault != NULL )
        {
            break;
        }
    }

    if( openglContextDefault == NULL )
    {
        return -1;
    }

    nsobject_autorelease( openglContextDefault );
    nsopenglcontext_setView( openglContextDefault, betrayContentView );

    nsstring_t caption = nsstring_stringWithUTF8String( "Hello from C-Ocoa" );
    nswindow_setTitle( betrayWindow, caption );

    nswindow_makeKeyAndOrderFront( betrayWindow, betrayWindow );
    nswindow_setAcceptsMouseMovedEvents( betrayWindow, YES );
    nswindow_setBackgroundColor( betrayWindow, nscolor_blackColor() );

    nsapplication_activateIgnoringOtherApps( nsapp, YES );

    nsopenglcontext_makeCurrentContext( openglContextDefault );
    nsapplication_setPresentationOptions( nsapp, (1<<10) );

    while( !terminated )
    {
        
        //id distantPast = ((id (*)(Class, SEL))objc_msgSend)(NSDateClass, distantPastSel);
        id distantPast = nsdate_distantPast();
        nsobject_t event;
        while( ( event = nsapplication_nextEventMatchingMask( nsapp, (~0), distantPast, NSDefaultRunLoopMode, YES ) ) != NULL )
        {
            unsigned int eventType = nsevent_type( event );

			switch(eventType)
			{
				//case NSMouseMoved:
				//case NSLeftMouseDragged:
				//case NSRightMouseDragged:
				//case NSOtherMouseDragged:
				case 5:
				case 6:
				case 7:
				case 27:
				{
					//NSRect adjustFrame = [[currentWindow contentView] frame];
                    nsview_t currentWindowContentView = nswindow_contentView( betrayWindow );
                    CGRect adjustFrame = nsview_frame( currentWindowContentView );
                    
                    CGPoint p = nswindow_mouseLocationOutsideOfEventStream( betrayWindow );

					// map input to content view rect
                #if 0
					if(p.x < 0)
                        p.x = 0;
					else if(p.x > adjustFrame.size.width)
                        p.x = adjustFrame.size.width;
					if(p.y < 0)
                        p.y = 0;
					else if(p.y > adjustFrame.size.height)
                        p.y = adjustFrame.size.height;
                    input->pointers[0].pointer_x = (float)p.x / (float)adjustFrame.size.width * 2.0 - 1.0;
                    input->pointers[0].pointer_y = (-1.0 + (float)p.y / (float)adjustFrame.size.height * 2.0) * (float)adjustFrame.size.height / (float)adjustFrame.size.width;

					// map input to pixels
					NSRect r = {p.x, p.y, 0, 0};
					//r = [currentWindowContentView convertRectToBacking:r];
					r = ((NSRect (*)(id, SEL, NSRect))abi_objc_msgSend_stret)(currentWindowContentView, convertRectToBackingSel, r);
					p = r.origin;
                #endif
					//printf("mouse moved to %f %f\n", p.member0, p.member1);
				break;
				}
                case 1:
					printf("mouse left key down\n");
				break;
				//case :
				case 2:
					printf("mouse left key up\n");
				break;
				//case NSRightMouseDown:
                case 3:
					printf("mouse right key down\n");
				break;
				//case NSRightMouseUp:
                case 4:
					printf("mouse right key up\n");
                break;
				//case NSScrollWheel:
				case 22:
				{
					float delta_x = nsevent_scrollingDeltaX( event );
					float delta_y = nsevent_scrollingDeltaY( event );
					char precision_scrolling = nsevent_hasPreciseScrollingDeltas( event );
					if(precision_scrolling)
					{
						delta_x *= 0.1f; // similar to glfw
						delta_y *= 0.1f;
                    }
                    #if 0
                    if(delta_x >= 0.001)
                        betray_plugin_button_set(0, BETRAY_BUTTON_SCROLL_UP, TRUE, -1);
                    if(delta_y <= -0.001)
                        betray_plugin_button_set(0, BETRAY_BUTTON_SCROLL_DOWN, TRUE, -1);
                    if(delta_y >= 0.001)
                        betray_plugin_button_set(0, BETRAY_BUTTON_SCROLL_LEFT, TRUE, -1);
                    if(delta_y <= -0.001)
                        betray_plugin_button_set(0, BETRAY_BUTTON_SCROLL_RIGHT, TRUE, -1);
                    betray_plugin_axis_set(betray_scroll_axis_id, (float)delta_x, (float)delta_y, 0);
                    #endif

                    printf("scrolling (x:%.3f, y:%.3f, prec:%d)\n", delta_x, delta_y, precision_scrolling);
				}
                break;
				//case NSKeyDown:
				case 10:
				{
                    uint pos = 0;
                    nsobject_t inputText = nsevent_characters( event );
                    const char * inputTextUTF8 = nsstring_UTF8String( inputText );
                    uint16_t keyCode = nsevent_keyCode( event );

                    #if 0
					if((last_modifyer_keys & 16) && (last_modifyer_keys & 2) && keyCode == BETRAY_BUTTON_Z)
					{
						betray_plugin_button_set(0, BETRAY_BUTTON_REDO, TRUE, -1);
					}else if((last_modifyer_keys & 16) && keyCode == BETRAY_BUTTON_Z)
					{
						betray_plugin_button_set(0, BETRAY_BUTTON_UNDO, TRUE, -1);
					}else if((last_modifyer_keys & 16) && keyCode == BETRAY_BUTTON_X)
					{
						betray_plugin_button_set(0, BETRAY_BUTTON_CUT, TRUE, -1);
					}else if((last_modifyer_keys & 16) && keyCode == BETRAY_BUTTON_C)
					{
						betray_plugin_button_set(0, BETRAY_BUTTON_COPY, TRUE, -1);
					}else if((last_modifyer_keys & 16) && keyCode == BETRAY_BUTTON_V)
					{
						betray_plugin_button_set(0, BETRAY_BUTTON_PASTE, TRUE, -1);
					}else
						betray_plugin_button_set(0, keyCode, TRUE, f_utf8_to_uint32(inputTextUTF8, &pos));
                    #endif
					printf("key down %u, text '%s'\n", keyCode, inputTextUTF8);
				}
                break;
				//case NSKeyUp:
				case 11:
				{
                    uint pos = 0;
                    nsobject_t inputText = nsevent_characters( event );
                    const char * inputTextUTF8 = nsstring_UTF8String( inputText );
                    uint16_t keyCode = nsevent_keyCode( event );
                    printf("key up %u\n", keyCode);
					break;
				}
				default:
					break;
            }

            nsapplication_sendEvent( nsapp, event );

        #if 0
			// if user closes the window we might need to terminate asap
			if(terminated)
				break;
        #endif

		}
    }

    return 0;
}