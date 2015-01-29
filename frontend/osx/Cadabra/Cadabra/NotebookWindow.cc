//
//  NotebookWindow.cc
//  Cadabra
//
//  Created by Kasper Peeters on 05/01/2015.
//  Copyright (c) 2015 phi-sci. All rights reserved.
//

#include "NotebookWindow.hh"

using namespace cadabra;

NotebookWindow::NotebookWindow()
: DocumentThread(this)
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
    
}

void NotebookWindow::on_disconnect()
{
    
}

void NotebookWindow::on_network_error()
{
    std::cout << "Network error" << std::endl;
}

void NotebookWindow::process_data()
{
    
}
