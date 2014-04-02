//
//  ServerWrapper.m
//  Cadabra
//
//  Created by Kasper Peeters on 02/04/2014.
//  Copyright (c) 2014 Kasper Peeters. All rights reserved.
//

#import "ServerWrapper.h"
#include "Server.hh"

@interface ServerWrapper () {
    Server server;
}
@end

@implementation ServerWrapper
-(void)run {
    server.run();
}
@end
