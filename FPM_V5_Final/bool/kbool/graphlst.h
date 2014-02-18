#ifndef GRAPHLIST_H
#define GRAPHLIST_H

#include "booleng.h"

#include "_lnk_itr.h"

#include "graph.h"

namespace KBool {

class Debug_driver;

class A2DKBOOLDLLEXP GraphList: public DL_List<void*>
{
protected:
    Bool_Engine* _GC;
public:

    GraphList( Bool_Engine* GC );

    GraphList( GraphList* other );

    ~GraphList();

    void    MakeOneGraph( Graph *total );

    void    Prepare( Graph *total );
    void     MakeRings();
    void     Correction();

    void    Simplify( double marge );
    void     Smoothen( double marge );
    void     Merge();
    void     Boolean( BOOL_OP operation, int intersectionRunsMax );

    void           WriteGraphs();
    void           WriteGraphsKEY( Bool_Engine* GC );

protected:
    void    Renumber();
    void    UnMarkAll();

};

}

#endif

