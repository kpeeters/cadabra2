#!/usr/bin/env python

import json
import os
import re
import sys

# Scan one Cadabra notebook file for an \algorithm{...}{...}
# or \property{...}{...} command and use this to output HTML
# code to produce a table-of-contents of all manual pages.

def scan_file(prevcat, dir, filename, ext):
    with open(dir+filename+ext) as jsonfile:
        data = json.load(jsonfile)
        jsonfile.close()

        src = data["cells"][0]["cells"][0]["source"]
        m = re.search('\\\\algorithm{(.*)}{(.*)}', src)
        if m:
            algo=m.group(1)
            desc=m.group(2)
            cat=algo[0].upper()
            if cat==prevcat:
                cat=''
            else:
                prevcat=cat
            print('<tr><td>'+cat+'<td></td><td><a href="manual/'+filename+'.html">'+algo+'</a></td>')
            print('<td>'+desc+'</td></tr>')
        m = re.search('\\\\property{(.*)}{(.*)}', src)
        if m:
            algo=m.group(1)
            desc=m.group(2)
            cat=algo[0].upper()
            if cat==prevcat:
                cat=''
            else:
                prevcat=cat
            print('<tr><td>'+cat+'<td></td><td><a href="manual/'+filename+'.html">'+algo+'</a></td>')
            print('<td>'+desc+'</td></tr>')

    return prevcat


# Scan all manual pages for algorithms.

#lst=set()
#for file in os.listdir(dir):
#    filename, ext = os.path.splitext(file)
#    if ext=='.cnb':
#        lst.add( filename )

algodir=sys.argv[1]+"/"
cat=''
for nb in sorted(sys.argv[2:]):
    cat=scan_file(cat, algodir, nb, '.cnb')


