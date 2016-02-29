#!/usr/bin/python

import json
import os
import re

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
            print('<tr>'+cat+'<td></td><td><a href="manual/"'+filename+ext+'">"'+algo+'</a></td>')
            print('<td>'+desc+'</td>')

    return prevcat


# Scan all manual pages for algorithms.

dir='../core/algorithms/'
lst=set()
for file in os.listdir(dir):
    filename, ext = os.path.splitext(file)
    if ext=='.cnb':
        lst.add( filename )
cat=''
for nb in sorted(lst):
    cat=scan_file(cat, dir, nb, '.cnb')


