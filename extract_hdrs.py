#!/usr/bin/env python

"""extract_hdrs.py -- Extract "extern" function headers from .c files to .h files.

Usage:
            ./extract_hdrs.py  [-h | -help | --help]
            ./extract_hdrs.py  -kstate
            ./extract_hdrs.py  [<c_file> <h_file> ...]

If run with the '-kstate' switch, then it looks for the file 'kstate.c',
and writes header information to 'kstate.h'. This is for convenience in the
management of libkstate.

If run with no filenames, it looks for all the C files in the current directory,
and for each <name>.c file expects to extract headers to a corresponding
<name>_fns.h file.

Otherwise, it expects pairs of source .c file and target .h file.

In all cases, the target .h file must (a) exist and (b) contain a pair of
start/end delimiters - as, for instance::

    Any text before the first delimiter is not altered
    // -------- TEXT AFTER THIS AUTOGENERATED - DO NOT EDIT --------
    Any text in here is replaced with the (new) header text
    // -------- TEXT BEFORE THIS AUTOGENERATED - DO NOT EDIT --------
    Any text after the second delimiter is not altered

A very simple regular expression is used to detect headers - basically, the
header is from the opening ``/*`` to the ``)`` before the ``{`` in something
like::

    /*
     * Header comment text
     */
    extern datatype other words function_name( any arguments )
    {

(and in general whitespace should be flexible in the actual function signature
itself).

Beware that the script does not check to see if the output header file will be
given different content (apart from the timestamp) and will thus proceed
regardless.
"""

# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is the Kynesim state management library.
#
# The Initial Developer of the Original Code is Kynesim, Cambridge UK.
# Portions created by the Initial Developer are Copyright (C) 2012
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Kynesim, Cambridge UK
#   Tony Ibbs <tibs@tonyibbs.co.uk>
#
# ***** END LICENSE BLOCK *****

import os
import re
import sys

from datetime import datetime
from difflib import ndiff

pattern = r"""\
(?P<header>                     # start of header group
     \s* / \* .* \n             # start of header comment
    (\s*   \* .* \n)*           # 0 or more comment lines
     \s*   \* /  \n             # end of header comment
     \s* extern \s+
        (?P<type>               # crudely allow for type info
          #(\w+ \s+) |                  # e.g., "fred "
          (\w+ \s+ \** \s*) |           # e.g., "fred *"
          (\w+ \s+ \w+ \s+ \** \s*)     # e.g., "struct fred " or "struct fred *"
        )
        (?P<name>
            \w+                 # name of function
        )
     \(  ([^)]|\n)* \)          # crudely match arguments
)                               # end of header group
\s* \n
\s* {
"""

start_delimiter = "// -------- TEXT AFTER THIS AUTOGENERATED - DO NOT EDIT --------\n"
start_timestamp = "// Autogenerated by extract_hdrs.py on %s\n"
end_delimiter = "// -------- TEXT BEFORE THIS AUTOGENERATED - DO NOT EDIT --------\n"

DEBUG = False

class GiveUp(Exception):
    pass

def extract_headers(c_file):
    """Extract the function headers from this C file.

    Returns text suitable for inclusion into the header file.
    """
    with open(c_file,'r') as file:
        data = file.read()
    headers = re.finditer(pattern, data, re.VERBOSE)
    new = []
    # Turn our function header into prototypes
    for hdr in headers:
        print '  Found ',hdr.group('name')
        new.append(hdr.group('header')+';\n')
    return ''.join(new)

def split_header_file(h_file):
    """Split the header file into its three parts and return them.

    We have:

    * the start of the file (written by the user)
    * the middle of the file (inserted by a previous use of this script)
    * the end of the file (written by the user)
    """
    with open(h_file, 'r') as original:
        data = original.read()

    delimiter1_posn = data.find(start_delimiter)
    delimiter2_posn = data.find(end_delimiter)

    if delimiter1_posn == -1:
        raise GiveUp("Couldn't find start 'AUTOGENERATED' line in file %s"%h_file)

    if delimiter2_posn == -1:
        raise GiveUp("Couldn't find end 'AUTOGENERATED' line in file %s"%h_file)

    start = data[:delimiter1_posn+len(start_delimiter)]
    end   = data[delimiter2_posn:]

    middle = data[delimiter1_posn+len(start_delimiter):delimiter2_posn]
    # Drop the previous timestamp line from the beginning
    lines = middle.splitlines()
    lines = lines[1:]
    middle = '\n'.join(lines) + '\n'

    return start, middle, end

def process_file(c_file, h_file):
    """Extract header comments/prototypes from 'c_file' into 'h_file'.
    """
    temp_file = h_file + '.new'
    save_file = h_file + '.bak'

    print 'Extracting headers from %s to %s'%(c_file, h_file)

    # Determine what we want the headers to look like
    new_middle = extract_headers(c_file)

    # Find out what they were already
    try:
        start, middle, end = split_header_file(h_file)
    except GiveUp as e:
        print e
        return

    # Do we need to change anything?
    if DEBUG and middle != new_middle:
        print
        print 'Function definitions for %s have changed'%c_file
        print '>>>>>>>>>>>>>>>>>>>>>>>>>>>>>'
        diff = ndiff(middle.splitlines(), new_middle.splitlines())
        print '\n'.join(diff)
        print '>>>>>>>>>>>>>>>>>>>>>>>>>>>>>'

    if middle == new_middle:
        print 'Nothing changed'
    else:
        print 'Writing new %s'%h_file

        now = datetime.now()
        timestamp_str = now.strftime('%Y-%m-%d (%a %d %b %Y) at %H:%M')
        timestamp_line = start_timestamp%timestamp_str

        with open(temp_file, 'w') as output:
            output.write(start)
            output.write(timestamp_line)
            output.write(new_middle)
            output.write(end)

        os.rename(h_file, save_file)
        os.rename(temp_file, h_file)

def do_stuff(args):

    c_to_h = {}

    if not args:
        filenames = os.listdir('.')
        for name in filenames:
            base, ext = os.path.splitext(name)
            if ext == '.c':
                c_to_h[name] = '%s_fns.h'%base
    else:
        while len(args) > 0:
            word = args[0]
            args = args[1:]
            if word in ('-help', '-h', '--help'):
                print __doc__
                return
            elif word == '-kstate':
                c_to_h = {'kstate.c' : 'kstate.h'}
                if len(args[1:]) != 0:
                    print 'Unexpected arguments %s after -kstate'%args[1:]
                    return
            else:
                c_file = word
                try:
                    h_file = args[0]
                except:
                    print 'C file (%s) not matched to a .h file'%c_file
                    return
                c_to_h[c_file] = h_file
                args = args[1:]
                if len(args) %2 != 0:
                    print 'Unbalanced arguments: not pairs of c_file h_file'

    # Basic checks before we do *anything*
    for c_file, h_file in c_to_h.items():
        if not os.path.exists(c_file):
            print 'C source file %s does not exist'%c_file
            return
        if not os.path.exists(h_file):
            print 'C header file %s does not exist'%h_file
            return
        if os.path.splitext(c_file)[-1] != '.c':
            print 'C source file %s does not have extension .c'%c_file
            return
        if os.path.splitext(h_file)[-1] != '.h':
            print 'C header file %s does not have extension .c'%h_file
            return

    for c_file, h_file in c_to_h.items():
        process_file(c_file, h_file)

if __name__ == '__main__':
    do_stuff(sys.argv[1:])

# vim: set tabstop=8 softtabstop=4 shiftwidth=4 expandtab:
