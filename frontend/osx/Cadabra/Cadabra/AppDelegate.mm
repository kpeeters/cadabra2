//
//  AppDelegate.m
//  Cadabra
//
//  Created by Kasper Peeters on 01/11/2014.
//  Copyright (c) 2014 phi-sci. All rights reserved.
//

#include <thread>
#import "AppDelegate.h"
#include "ComputeThread.hh"
#include "NotebookWindow.hh"

@interface AppDelegate ()

@property (weak) IBOutlet NSWindow *window;
@end

@implementation AppDelegate {
    
    cadabra::NotebookWindow *nw;
    cadabra::ComputeThread  *compute;
    std::thread             *compute_thread;
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {

    nw = new cadabra::NotebookWindow();
    
    compute        = new cadabra::ComputeThread(nw, *nw);
    compute_thread = new std::thread([&] { compute->run(); });

    nw->set_compute_thread(compute);
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
    // Insert code here to tear down your application
}

@end
