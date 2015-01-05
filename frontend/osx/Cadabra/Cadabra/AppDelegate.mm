//
//  AppDelegate.m
//  Cadabra
//
//  Created by Kasper Peeters on 01/11/2014.
//  Copyright (c) 2014 phi-sci. All rights reserved.
//

#import "AppDelegate.h"
#include "ComputeThread.hh"
#include "NotebookWindow.hh"

@interface AppDelegate ()

@property (weak) IBOutlet NSWindow *window;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    // Insert code here to initialize your application
    cadabra::NotebookWindow nw;
    
    cadabra::ComputeThread compute(&nw, nw);
    std::thread            compute_thread(&cadabra::ComputeThread::run, std::ref(compute));
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
    // Insert code here to tear down your application
}

@end
