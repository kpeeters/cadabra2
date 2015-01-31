//
//  NotebookWindow.cc
//  Cadabra
//
//  Created by Kasper Peeters on 05/01/2015.
//  Copyright (c) 2015 phi-sci. All rights reserved.
//

#include "NotebookWindow.hh"

using namespace cadabra;

NotebookWindow::NotebookWindow(NotebookController *nc)
: DocumentThread(this), controller(nc)
{
}

void NotebookWindow::add_cell(const DTree &, DTree::iterator)
{
    
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

void NotebookWindow::on_connect()
{
    [controller setKernelStatus:@"Connected"];
}

void NotebookWindow::on_disconnect()
{
    
}

void NotebookWindow::on_network_error()
{
    [controller setKernelStatus:@"Network error"];
}

void NotebookWindow::process_data()
{
    
}
