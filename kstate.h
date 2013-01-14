/*
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the KBUS State library.
 *
 * The Initial Developer of the Original Code is Kynesim, Cambridge UK.
 * Portions created by the Initial Developer are Copyright (C) 2013
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Kynesim, Cambridge UK
 *   Tony Ibbs <tibs@tonyibbs.co.uk>
 *
 * ***** END LICENSE BLOCK *****
 */

#ifndef _KSTATE_H_INCLUDED_
#define _KSTATE_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

enum kstate_permissions {
  KSTATE_READ=1,          // The state may be read
  KSTATE_WRITE=2,         // The state may be written
};
typedef enum kstate_permissions kstate_permissions_t;

struct kstate_state {
  char        *name;
  uint32_t     permissions;
};
typedef struct kstate_state kstate_state_t;

struct kstate_transaction {
  struct kstate_state       state;

};
typedef struct kstate_transaction kstate_transaction_t;

// -------- TEXT AFTER THIS AUTOGENERATED - DO NOT EDIT --------
// Autogenerated by extract_hdrs.py on 2013-01-14 (Mon 14 Jan 2013) at 11:20
/*
 * Subscribe to a state.
 *
 * - ``state_name`` is the name of the state to subscribe to.
 * - ``permissions`` is constructed by OR'ing the permission flags
 *   KSTATE_READ and/or KSTATE_WRITE. At least one of those must be given.
 * - ``state`` is the actual state identifier, as newly constructed by this
 *   function.
 *
 * A state name may contain A-Z, a-z, 0-9 and the dot (.) character. It may not
 * start or end with a dot, and may not contain adjacent dots. It must contain
 * at least one character.
 *
 * If this is the first subscription to the named state, then the state will
 * be created.
 *
 * Returns 0 if the subscription succeeds, or a negative value if it fails.
 * The negative value will be ``-errno``, giving an indication of why the
 * function failed.
 */
extern int kstate_subscribe(const char               *state_name,
                            enum kstate_permissions   permissions,
                            struct kstate_state     **state);

/*
 * Unsubscribe from a state.
 *
 * - ``state`` is the state from which to unsubscribe.
 *
 * After this, the content of the state datastructure will have been
 * unset - it will no longer be a valid state.
 *
 * Also, any attempt to use pointers into the state data will fail.
 *
 * Note that transactions using the state keep their own copy of the state
 * information, and are not affected by this function - i.e., the state can
 * still be accessed via any transactions that are still open on it.
 *
 * Returns 0 if the unsubscription succeeds, or a negative value if it fails.
 * The negative value will be ``-errno``, giving an indication of why the
 * function failed.
 */
extern int kstate_unsubscribe(struct kstate_state   **state);

// -------- TEXT BEFORE THIS AUTOGENERATED - DO NOT EDIT --------

#ifdef __cplusplus
}
#endif

#endif /* _KSTATE_H_INCLUDED_ */

// vim: set tabstop=8 softtabstop=2 shiftwidth=2 expandtab:
//
// Local Variables:
// tab-width: 8
// indent-tabs-mode: nil
// c-basic-offset: 2
// End:
