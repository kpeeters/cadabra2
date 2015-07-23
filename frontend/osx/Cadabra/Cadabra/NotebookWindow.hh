//
//  NotebookWindow.h
//  Cadabra
//
//  Created by Kasper Peeters on 05/01/2015.
//  Copyright (c) 2015 phi-sci. All rights reserved.
//

#ifndef __Cadabra__NotebookWindow__
#define __Cadabra__NotebookWindow__

#include <stdio.h>
#include "DocumentThread.hh"
#include "GUIBase.hh"
#include "NotebookController.h"
#include "NotebookCanvas.hh"

namespace cadabra_osx {

/// \ingroup osx
///
/// Objective-C++ class implementing DocumentThread and providing an OS-X
/// notebook interface.

class NotebookWindow : public DocumentThread, public GUIBase {
    public:
        NotebookWindow(NotebookController *);
    
        virtual void add_cell(const DTree&, DTree::iterator, bool visible) override;
        virtual void remove_cell(const DTree&, DTree::iterator) override;
        virtual void update_cell(const DTree&, DTree::iterator) override;
        virtual void position_cursor(const DTree&, DTree::iterator) override;
        virtual void remove_all_cells() override;
    
        virtual void on_connect() override;
        virtual void on_disconnect() override;
        virtual void on_network_error() override;
    
        virtual void process_data() override;
    
    private:
        NotebookController *controller;

        std::vector<NotebookCanvas *> canvasses;
        int                           current_canvas;
};
    
};

#endif /* defined(__Cadabra__NotebookWindow__) */
