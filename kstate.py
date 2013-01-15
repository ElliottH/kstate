#! /usr/bin/env python3

"""A simple Python binding for the Kstate library.

NB: Uses Python 3
"""

import sys
import os
import errno

import ctypes
from ctypes import c_char_p, c_int, c_uint32
from ctypes import Structure
from ctypes import POINTER, pointer, byref

THIS_DIR = os.path.split(__file__)[0]

KSTATE_READ = 1
KSTATE_WRITE = 2

class GiveUp(Exception):
    """
    Use this to indicate that something has gone wrong and we are giving up.

    By default, a return code of 1 is indicated by the 'retcode' value - this
    can be set by the caller to another value, which __main__.py should then
    use as its return code if the exception reaches it.
    """

    # We provide a single attribute, which is used to specify the exit code
    # to use when a command line handler gets back a GiveUp exception.
    retcode = 1

    def __init__(self, message=None, retcode=1):
        self.message = message
        self.retcode = retcode

    def __str__(self):
        if self.message is None:
            return ''
        else:
            return self.message

    def __repr__(self):
        parts = []
        if self.message is not None:
            parts.append(repr(self.message))
        if self.retcode != 1:
            parts.append('%d'%self.retcode)
        return 'GiveUp(%s)'%(', '.join(parts))

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

def permission_str(permissions):
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

class State(Structure):
    _fields_ = [('name', c_char_p),
                ('permissions', c_uint32)
               ]

    def __str__(self):
        return 'State {0} for {1}'.format(self.name, permission_str(self.permissions))

class Transaction(Structure):
    _fields_ = [('state', State),
               ]

    def __str__(self):
        return 'Transaction on {0}'.format(self.state)

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

    def subscribe(self, name, permissions):
        """Subscribe to state 'name' with the given 'permissions'

        The state name must be a byte string (b"xxx").
        """
        ptr_to_state = POINTER(State)()
        ret = self.fn_subscribe(name, permissions, byref(ptr_to_state))
        if ret:
            raise Error('Error subscribing to %s for %s'%(name,
                        permission_str(permissions)), -ret)
        state = ptr_to_state[0]
        return state

    def unsubscribe(self, state):
        """Unsubscribe from this state.
        """
        name = state.name
        permissions = state.permissions

        ptr_to_state = pointer(state)
        ret = self.fn_unsubscribe(byref(ptr_to_state))
        if ret:
            raise Error('Error unsubscribing from %s for %s'%(name,
                        permission_str(permissions)), -ret)

def expect_success(what, fn, *args, **kwargs):
    print('\n--- Test {0}'.format(what))
    val = fn(*args, **kwargs)
    print('... Call of {0} succeeded, returned {1}'.format(fn.__name__, val))
    return val

def expect_failure(what, expected_errno, fn, *args, **kwargs):
    print('\n--- Test {0}'.format(what))
    try:
        val = fn(*args, **kwargs)
        raise GiveUp('Expected failure, but call of {0} succeeded,'
                     ' returning {1}'.format(fn.__name__, val))
    except Error as e:
        print('Expected failure, and call of {0} failed'
              ' with:\n  {1}'.format(fn.__name__, e))
        if e.errno == expected_errno:
            print('...which was what we wanted')
        else:
            raise ValueError('xxx Got wrong errno value, expected {0}'.format(expected_errno))

def expect_state(state, name, permissions):
    if not state:
        raise GiveUp('Expected state {0}, but state is NULL'.format(state))
    if state.name != name:
        raise GiveUp('Expected state with name {0}, but got {1}'.format(name, state))
    if state.permissions != permissions:
        raise GiveUp('Expected state with permissions {} ({}), but got {}'.format(permissions,
            permission_str(permissions), state))

def main(args):

    lib = KstateLibrary()

    state = expect_success('Subscribing to a simple name',
                           lib.subscribe, b'Fred', KSTATE_READ|KSTATE_WRITE)
    expect_state(state, b'Fred', KSTATE_READ|KSTATE_WRITE)

    expect_success('Unsubscribing',
                   lib.unsubscribe, state)

    state = expect_success('Subscribing to a name with a "." in it',
                           lib.subscribe, b'Fred.Jim', KSTATE_READ|KSTATE_WRITE)
    expect_state(state, b'Fred.Jim', KSTATE_READ|KSTATE_WRITE)

    expect_success('Unsubscribing',
                   lib.unsubscribe, state)

    state = expect_success('Subscribing with just READ permission',
                           lib.subscribe, b'Fred.Jim', KSTATE_READ)
    expect_state(state, b'Fred.Jim', KSTATE_READ)

    expect_success('Unsubscribing',
                   lib.unsubscribe, state)

    state = expect_success('Subscribing with just WRITE permission',
                           lib.subscribe, b'Fred.Jim', KSTATE_WRITE)
    expect_state(state, b'Fred.Jim', KSTATE_WRITE)

    expect_success('Unsubscribing',
                   lib.unsubscribe, state)

    expect_failure('Failing to subscribe to a zero length name', errno.EINVAL,
                   lib.subscribe, b"", KSTATE_READ)

    expect_failure('Failing to subscribe to a name starting with "."', errno.EINVAL,
                   lib.subscribe, b".Fred", KSTATE_READ)

    expect_failure('Failing to subscribe to a name ending with "."', errno.EINVAL,
                   lib.subscribe, b"Fred.", KSTATE_READ)

    expect_failure('Failing to subscribe to a name with adjacent "."s', errno.EINVAL,
                   lib.subscribe, b"Fred..Bob", KSTATE_READ)

    expect_failure('Failing to subscribe to a name with non-alphanumerics', errno.EINVAL,
                   lib.subscribe, b"Fred&Bob", KSTATE_READ)

    expect_failure('Failing to subscribe with permission bits 0x0', errno.EINVAL,
                   lib.subscribe, b"Fred", 0x0)

    expect_failure('Failing to subscribe with permission bits 0xF', errno.EINVAL,
                   lib.subscribe, b"Fred", 0xF)


if __name__ == '__main__':
    main(sys.argv[1:])

# vim: set tabstop=8 softtabstop=4 shiftwidth=4 expandtab:
