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
 * The Original Code is the BUS State library.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>    # for isalnum

#include "kstate.h"

/*
 * Given a message name, is it valid?
 *
 * We have nothing to say on maximum length.
 *
 * Returns the name length if it's OK, 0 if it's naughty
 */
static int kstate_check_message_name(const char *name)
{
  size_t ii;
  int dot_at = 1;

  if (name == NULL) {
    fprintf(stderr, "!!! kstate_subscribe: State name may not be NULL\n");
    return 0;
  }

  size_t name_len = strlen(name);

  if (name_len == 0) {
    fprintf(stderr, "!!! kstate_subscribe: State name may not be zero length\n");
    return 0;
  }

  if (name[0] == '.' || name[name_len-1] == '.') {
    fprintf(stderr, "!!! kstate_subscribe: State name '%s' may not start or"
            " end with '.'\n", name);
    return 0;
  }

  for (ii = 0; ii < name_len; ii++) {
    if (name[ii] == '.') {
      if (dot_at == ii - 1) {
        fprintf(stderr, "!!! kstate_subscribe: State name '%s' may not have"
                " adjacent '.'s\n", name);
        return 0;
      }
      dot_at = ii;
    } else if (!isalnum(name[ii])) {
      fprintf(stderr, "!!! kstate_subscribe: State name '%s' may not"
              " contain '%c' (not alphanumeric)\n", name, name[ii]);
      return 0;
    }
  }
  return name_len;
}

/*
 * Given state permissions, are they valid?
 *
 * Returns true if they're bad, false if we like them
 */
static bool kstate_permissions_are_bad(uint32_t permissions)
{
  if (!permissions) {
    fprintf(stderr, "!!! kstate_subscribe: Unset permissions bits (0x0) not allowed\n");
    return true;
  }
  else if (permissions & ~(KSTATE_READ | KSTATE_WRITE)) {
    fprintf(stderr, "!!! kstate_subscribe: Unexpected permission bits 0x%x in 0x%x\n",
            permissions & ~(KSTATE_READ | KSTATE_WRITE),
            permissions);
    return true;
  }
  return false;
}

/*
 * Subscribe to a state.
 *
 * - ``name`` is the name of the state to subscribe to.
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
extern int kstate_subscribe(const char               *name,
                            enum kstate_permissions   permissions,
                            struct kstate_state     **state)
{
  printf("Subscribing to '%s' for 0x%x\n", name, permissions);

  if (kstate_permissions_are_bad(permissions)) {
    return -EINVAL;
  }

  size_t name_len = kstate_check_message_name(name);
  if (name_len == 0) {
    return -EINVAL;
  }

  struct kstate_state *new = malloc(sizeof(*state));
  if (!new) return -ENOMEM;

  new->name = malloc(name_len + 1);
  if (!new->name) {
    free(new);
    return -ENOMEM;
  }
  strcpy(new->name, name);

  new->permissions = permissions;

  *state = new;
  return 0;
}

/*
 * Unsubscribe from a state.
 *
 * - ``state`` is the state from which to unsubscribe.
 *
 * After this, the content of the state datastructure will have been
 * unset/freed, and ``state`` itself will have been freed.
 *
 * Note that transactions using the state keep their own copy of the state
 * information, and are not affected by this function - i.e., the state can
 * still be accessed via any transactions that are still open on it.
 *
 * Returns 0 if the unsubscription succeeds, or a negative value if it fails.
 * The negative value will be ``-errno``, giving an indication of why the
 * function failed.
 */
extern int kstate_unsubscribe(struct kstate_state   **state)
{
  if (*state) {
    if ((*state)->name) {
      free((*state)->name);
      (*state)->name = NULL;
    }
    (*state)->permissions = 0;
    free(*state);
    *state = NULL;
  }
  return 0;
}

// vim: set tabstop=8 softtabstop=2 shiftwidth=2 expandtab:
//
// Local Variables:
// tab-width: 8
// indent-tabs-mode: nil
// c-basic-offset: 2
// End:
