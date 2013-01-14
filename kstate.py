#! /usr/bin/env python3

"""A simple Python binding for the Kstate library.

NB: Uses Python 3
"""

import sys
import os

import ctypes
from ctypes import c_char_p, c_int, c_uint32
from ctypes import Structure
from ctypes import POINTER, byref

THIS_DIR = os.path.split(__file__)[0]

KSTATE_READ = 1
KSTATE_WRITE = 2

class Error(Exception):
    """ Use this to report an 'errno' error.
    """

    def __init__(self, message=None, errno=0):
        self.message = message
        self.errno = errno

    def _errno_str(self):
        return '%d: %s %s'%(self.errno,
                            errno.errorcode[self.errno],
                            os.strerror(self.errno))

    def __str__(self):
        if self.message is None:
            return self._errno_str()
        else:
            return '%s %s'%(self.message, self._errno_str())

    def __repr__(self):
        parts = []
        if self.message is not None:
            parts.append(repr(self.message))
        if self.retcode != 1:
            parts.append('%d'%self.errno)
        return 'Error(%s)'%(', '.join(parts))

class State(Structure):
    _fields_ = [('name', c_char_p),
                ('permissions', c_uint32)
               ]

class Transaction(Structure):
    _fields_ = [('state', State),
               ]

class KstateLibrary(object):

    def __init__(self, location=None):
        if location is None:
            # At the moment, we're assuming the library is in the same directory
            self.location = os.path.join(THIS_DIR,'libkstate.so')
        else:
            self.location = location
        self.lib = ctypes.CDLL(self.location)

        self.fn_subscribe = self.lib.kstate_subscribe
        self.fn_subscribe.argtypes = [c_char_p, c_int, POINTER(POINTER(State))]
        print('subscribe', self.fn_subscribe)

        self.fn_unsubscribe = self.lib.kstate_unsubscribe
        self.fn_unsubscribe.argtypes = [POINTER(POINTER(State))]
        print('unsubscribe', self.fn_unsubscribe)

    def _perm_str(self, permissions):
        parts = []
        if permissions & KSTATE_READ:
            parts.append('read')
        if permissions & KSTATE_WRITE:
            parts.append('write')
        if permissions & ~(KSTATE_READ|KSTATE_WRITE):
            parts.append('0x%x'%(permissions & ~(KSTATE_READ|KSTATE_WRITE)))
        if parts:
            return '|'.join(parts)
        else:
            return '0x0'

    def subscribe(self, name, permissions):
        print('Subscribing to %s for %s'%(name, self._perm_str(permissions)))
        ptr_to_state = POINTER(State)()
        ret = self.fn_subscribe(b"Fred", 1, byref(ptr_to_state))
        if ret:
            print('Returns {0}'.format(ret))
            raise Error('Error subscribing to %s for %s'%(name,
                self._perm_str(permissions)), -ret)
        print('State ptr is NULL: {0}'.format(not bool(ptr_to_state)))
        state = ptr_to_state[0]
        print(state.name)
        print(state.permissions)
        return state

    def unsubscribe(self, state):
        name = state.name
        permissions = state.permissions
        print('Unsubscribing from %s for %s'%(name, self._perm_str(permissions)))

        ptr_to_state = POINTER(State)()
        ret = self.fn_unsubscribe(byref(ptr_to_state))
        if ret:
            print('Returns {0}'.format(ret))
            raise Error('Error unsubscribing from %s for %s'%(name,
                self._perm_str(permissions)), -ret)
        print('Returns {0}'.format(ret))
        print('State ptr is NULL: {0}'.format(not bool(ptr_to_state)))

def main(args):

    lib = KstateLibrary()

    state = lib.subscribe(b"Fred", KSTATE_READ|KSTATE_WRITE|0x8)

    lib.unsubscribe(state)


if __name__ == '__main__':
    main(sys.argv[1:])

# vim: set tabstop=8 softtabstop=4 shiftwidth=4 expandtab:
