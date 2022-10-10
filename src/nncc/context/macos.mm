#import <Cocoa/Cocoa.h>

void ToggleFullscreenCocoa(NSWindow* window) {
    dispatch_async(dispatch_get_main_queue(),
        ^{
            [window toggleFullScreen:nil];
        }
    );
}

void ToggleFullscreenCocoa(void* window) {
    ToggleFullscreenCocoa((NSWindow*) window);
}