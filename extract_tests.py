#!/usr/bin/env python

"""extract_tests.py -- Find "check" tests and put them in a suite

Usage:
            ./extract_tests.py  [-h | -help | --help]
            ./extract_tests.py  <c_file>

Looks for tests declared as::

    START_TEST(<name>)

and replaces the text between::

  // TESTS START

and::

  // TESTS END

with lines of the form::

  tcase_add_test(tc_core, <name>);

It's terribly simple.
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

from difflib import ndiff

test_pattern = "START_TEST\((?P<test>.*)\)"

file_pattern = """\
(?P<start>
  .*
  \\n[\t\ ]*//\ START\ TESTS\\n
)
(?P<middle>
  .*
)
(?P<end>
  ^[\t\ ]*//\ END\ TESTS\\n
  .*
)
"""

DEBUG = True

class GiveUp(Exception):
    pass

def extract_tests(c_file):
    """Extract the test names
    """
    with open(c_file,'r') as file:
        data = file.read()
    matches = re.finditer(test_pattern, data)
    tests = []
    for m in matches:
        print '  Found ',m.group('test')
        tests.append(m.group('test'))
    return tests

def split_file(c_file):
    """Split the header file into its three parts and return them.

    We have:

    * the start of the file (written by the user)
    * the middle of the file (inserted by a previous use of this script)
    * the end of the file (written by the user)
    """
    with open(c_file, 'r') as original:
        data = original.read()

    match = re.match(file_pattern, data, re.VERBOSE|re.MULTILINE|re.DOTALL)
    if match is None:
        raise GiveUp('Could not match file to START TESTS...END TESTS pattern\n')

    start  = match.group('start')
    middle = match.group('middle')
    end    = match.group('end')

    return start, middle, end

def process_file(c_file):
    """Update the list of tests in our C file
    """
    temp_file = c_file + '.new'
    save_file = c_file + '.bak'

    print 'Finding tests in %s'%(c_file)

    tests = extract_tests(c_file)
    if len(tests) == 0:
        print 'No tests found'
        return

    new_middle = []
    for test in tests:
        new_middle.append('  tcase_add_test(tc_core, %s);'%test)
    new_middle = '\n'.join(new_middle) + '\n'

    try:
        start, middle, end = split_file(c_file)
    except GiveUp as e:
        print e
        return

    # Do we need to change anything?
    if DEBUG and middle != new_middle:
        print
        print 'List of tests for %s has changed'%c_file
        print '>>>>>>>>>>>>>>>>>>>>>>>>>>>>>'
        print '"%s"'%middle.splitlines()[0]
        print '>>>>>>>>>>>>>>>>>>>>>>>>>>>>>'
        diff = ndiff(middle.splitlines(), new_middle.splitlines())
        print '\n'.join(diff)
        print '>>>>>>>>>>>>>>>>>>>>>>>>>>>>>'

    if middle == new_middle:
        print 'Nothing changed'
    else:
        print 'Writing new %s'%c_file

        with open(temp_file, 'w') as output:
            output.write(start)
            output.write(new_middle)
            output.write(end)

        os.rename(c_file, save_file)
        os.rename(temp_file, c_file)

def do_stuff(args):

    if len(args) != 1 or args[0] in ('-help', '-h', '--help'):
        print __doc__
        return

    c_file = args[0]

    # Basic checks before we do *anything*
    if not os.path.exists(c_file):
        print 'C source file %s does not exist'%c_file
        return
    if os.path.splitext(c_file)[-1] != '.c':
        print 'C source file %s does not have extension .c'%c_file
        return

    process_file(c_file)

if __name__ == '__main__':
    do_stuff(sys.argv[1:])

# vim: set tabstop=8 softtabstop=4 shiftwidth=4 expandtab:
