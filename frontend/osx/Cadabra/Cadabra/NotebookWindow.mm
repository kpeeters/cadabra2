//
//  NotebookWindow.cc
//  Cadabra
//
//  Created by Kasper Peeters on 05/01/2015.
//  Copyright (c) 2015 phi-sci. All rights reserved.
//

#include "NotebookWindow.hh"
#import <Cocoa/Cocoa.h>

using namespace cadabra;

NotebookWindow::NotebookWindow(NotebookController *nc)
: DocumentThread(this), controller(nc)
{
    canvasses.push_back(new NotebookCanvas());
    current_canvas=0;
    
    new_document();
}

void NotebookWindow::add_cell(const DTree &tr, DTree::iterator it, bool visible)
{
    NSLog(@"adding cell");
    NSSize contentSize = [controller.scrollbox contentSize];

    NSView *container = [[NSView alloc] initWithFrame:CGRectMake(0,0,contentSize.width,contentSize.height)];
    [container setAutoresizingMask:NSViewWidthSizable|NSViewHeightSizable];
    
    NSTextView *label = [[NSTextView alloc] initWithFrame:CGRectMake(0,0,contentSize.width,100)];
    [[label.textStorage mutableString] setString:@"hello world"];
    [label setAutoresizingMask:NSViewWidthSizable|NSViewHeightSizable];
    [[label textContainer] setContainerSize:NSMakeSize(contentSize.width, FLT_MAX)];
    [[label textContainer] setWidthTracksTextView:YES];
    
    NSTextView *label2 = [[NSTextView alloc] initWithFrame:CGRectMake(0,0,contentSize.width,100)];
    [[label2.textStorage mutableString] setString:@"this is fun"];
    [label2 setAutoresizingMask:NSViewWidthSizable|NSViewHeightSizable];
    [[label2 textContainer] setContainerSize:NSMakeSize(contentSize.width, FLT_MAX)];
    [[label2 textContainer] setWidthTracksTextView:YES];

    NSLayoutConstraint *c1=[NSLayoutConstraint constraintWithItem:label attribute:NSLayoutAttributeBottom
                                                          relatedBy:NSLayoutRelationEqual toItem:label2
                                                        attribute:NSLayoutAttributeTop multiplier:1.0 constant:0];
    
    [container addConstraint:c1];
    
    canvasses[current_canvas]->visualcells[&(*it)]=label;

    [container addSubview:label];
    [container addSubview:label2];
    [controller.scrollbox setHasVerticalScroller:YES];
    [controller.scrollbox setAutoresizingMask:NSViewWidthSizable|NSViewHeightSizable];
    [controller.scrollbox setDocumentView:container];
    [controller.scrollbox setNeedsDisplay:YES];
}

void NotebookWindow::remove_cell(const DTree&, DTree::iterator)
{
    
}

void NotebookWindow::update_cell(const DTree&, DTree::iterator)
{
    
}

void NotebookWindow::position_cursor(const DTree&, DTree::iterator)
{
    
}

void NotebookWindow::remove_all_cells()
{
    
}

void NotebookWindow::on_connect()
{
    [controller.status_label setStringValue:@"Connected"];
}

void NotebookWindow::on_disconnect()
{
    
}

void NotebookWindow::on_network_error()
{
    [controller.status_label setStringValue:@"Network error"];
}

void NotebookWindow::process_data()
{
    
}
