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

class State(Structure):
    _fields_ = [('name', c_char_p),
                ('permissions', c_uint32)
               ]

class Transaction(Structure):
    _fields_ = [('state', State),
               ]

def load_kstate():
    """Load the Kstate library.

    Returns the handle for the library.
    """
    # At the moment, we're assuming the library is in the same directory
    lib = ctypes.CDLL(os.path.join(THIS_DIR,'libkstate.so'))
    return lib

def main(args):
    lib = load_kstate()
    print(lib)

    subscribe = lib.kstate_subscribe
    subscribe.argtypes = [c_char_p, c_int, POINTER(POINTER(State))]
    print(subscribe)

    ptr_to_state = POINTER(State)()
    ret = lib.kstate_subscribe(b"Fred", 1, byref(ptr_to_state))
    print('Returns {0}'.format(ret))
    print('State ptr is NULL: {0}'.format(not bool(ptr_to_state)))
    state = ptr_to_state[0]
    print(state.name)
    print(state.permissions)

    unsubscribe = lib.kstate_unsubscribe
    unsubscribe.argtypes = [POINTER(POINTER(State))]
    print(unsubscribe)
    ret = lib.kstate_unsubscribe(byref(ptr_to_state))
    print('Returns {0}'.format(ret))
    print('State ptr is NULL: {0}'.format(not bool(ptr_to_state)))

if __name__ == '__main__':
    main(sys.argv[1:])

# vim: set tabstop=8 softtabstop=4 shiftwidth=4 expandtab:
