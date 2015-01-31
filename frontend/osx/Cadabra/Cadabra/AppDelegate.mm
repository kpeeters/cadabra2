//
//  AppDelegate.m
//  Cadabra
//
//  Created by Kasper Peeters on 01/11/2014.
//  Copyright (c) 2014 phi-sci. All rights reserved.
//

#import "AppDelegate.h"

@interface  AppDelegate()

@property (weak) IBOutlet NSWindow *window;
@property (nonatomic,strong) IBOutlet NotebookController *notebookController;

@end


@implementation AppDelegate {
    
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    self.notebookController = [[NotebookController alloc] initWithNibName:@"Notebook" bundle:nil];
    
    // 2. Add the view controller to the Window's content view
    [self.window.contentView addSubview:self.notebookController.view];
    self.notebookController.view.frame = ((NSView*)self.window.contentView).bounds;
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
    // Insert code here to tear down your application
}
- (IBAction)menuAboutCadabra:(id)sender {
    NSLog(@"About");
}

@end
