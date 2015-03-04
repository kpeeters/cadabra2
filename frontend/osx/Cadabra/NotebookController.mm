//
//  NotebookController.m
//  Cadabra
//
//  Created by Kasper Peeters on 31/01/2015.
//  Copyright (c) 2015 phi-sci. All rights reserved.
//

#import "NotebookController.h"
#include <thread>
#include "ComputeThread.hh"
#include "NotebookWindow.hh"

@interface NotebookController ()

@end

@implementation NotebookController {
    
    cadabra::NotebookWindow *nw;
    cadabra::ComputeThread  *compute;
    std::thread             *compute_thread;
}

- (void)loadView {
    BOOL ownImp = ![NSViewController instancesRespondToSelector:@selector(readySetGo)];
    
    [super loadView];
    
    if(ownImp) {
        [self readySetGo];
    }
}

-(void)readySetGo
{
   
    [_status_label setStringValue:@"Ready"];
    
    nw = new cadabra::NotebookWindow(self);
    
    compute        = new cadabra::ComputeThread(nw, *nw);
    compute_thread = new std::thread(&cadabra::ComputeThread::run, compute);
    
    nw->set_compute_thread(compute);
}


@end
