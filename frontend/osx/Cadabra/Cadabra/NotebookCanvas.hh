//
//  NotebookCanvas.hh
//  Cadabra
//
//  Created by Kasper Peeters on 04/03/2015.
//  Copyright (c) 2015 phi-sci. All rights reserved.
//

#ifndef Cadabra_NotebookCanvas_hh
#define Cadabra_NotebookCanvas_hh

namespace cadabra {
    
    class NotebookCanvas {
    public:
        std::map<DataCell *, NSTextView *> visualcells;
    };
    
}


#endif
