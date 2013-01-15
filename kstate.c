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
 * Print a representation of 'state' on output 'stream'.
 *
 * Assumes the state is valid.
 *
 * If 'start' is non-NULL, print it before the state (with no added whitespace).
 * If 'eol' is true, then print a newline after the state.
 */
extern void print_state(FILE *stream, char *start, struct kstate_state *state, bool eol)
{
  if (start)
    fprintf(stream, "%s", start);
  fprintf(stream, "State '%s' for ", state->name);
  if (state->permissions & KSTATE_READ)
    fprintf(stream, "read");
  if ((state->permissions & KSTATE_READ) && (state->permissions & KSTATE_WRITE))
    fprintf(stream, "|");
  if (state->permissions & KSTATE_WRITE)
    fprintf(stream, "write");
  if (eol)
    fprintf(stream, "\n");
}

/*
 * Create a new "empty" state.
 *
 * The normal usage is to create an empty state and then immediately
 * populate it::
 *
 *     struct kstate_state *state = kstate_create_state();
 *     int ret = kstate_subscribe("State.Name", KSTATE_READ, state);
 *
 * and then eventually to destroy it:
 *
 *     int ret = kstate_unsubscribe(state);
 *     if (ret) {
 *       // deal with the error
 *     }
 *     kstate_destroy(&state);
 *
 * Returns the new state, or NULL if there was insufficient memory.
 */
extern struct kstate_state *kstate_create_state()
{
  struct kstate_state *new = malloc(sizeof(struct kstate_state));
  memset(new, 0, sizeof(*new));
  return new;
}

/*
 * Destroy a state created with 'kstate_create_state'.
 *
 * If a NULL pointer is given, then it is ignored, otherwise the state is
 * freed and the pointer 'state' is set to NULL.
 */
extern void kstate_free_state(struct kstate_state **state)
{
  if (*state) {
    kstate_unsubscribe(*state);
    free(*state);
    *state = NULL;
  }
}

/*
 * Subscribe to a state.
 *
 * Any data that was previously in 'state' will be removed using
 * 'kstate_unsubscribe()'.
 *
 * - ``name`` is the name of the state to subscribe to.
 * - ``permissions`` is constructed by OR'ing the permission flags
 *   KSTATE_READ and/or KSTATE_WRITE. At least one of those must be given.
 * - ``state`` is the actual state identifier, as amended by this function.
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
extern int kstate_subscribe(struct kstate_state     *state,
                            const char              *name,
                            enum kstate_permissions  permissions)
{
  printf("Subscribing to '%s' for 0x%x\n", name, permissions);

  if (kstate_permissions_are_bad(permissions)) {
    return -EINVAL;
  }

  size_t name_len = kstate_check_message_name(name);
  if (name_len == 0) {
    return -EINVAL;
  }

  kstate_unsubscribe(state);

  state->name = malloc(name_len + 1);
  if (!state->name) {
    return -ENOMEM;
  }
  strcpy(state->name, name);
  state->permissions = permissions;
  return 0;
}

/*
 * Unsubscribe from a state.
 *
 * - ``state`` is the state from which to unsubscribe.
 *
 * After this, the content of the state datastructure will have been
 * unset/freed. Unsubscribing from the same state again will have no effect.
 *
 * Note that transactions using the state keep their own copy of the state
 * information, and are not affected by this function - i.e., the state can
 * still be accessed via any transactions that are still open on it.
 *
 * Returns 0 if the unsubscription succeeds, or a negative value if it fails.
 * The negative value will be ``-errno``, giving an indication of why the
 * function failed.
 */
extern void kstate_unsubscribe(struct kstate_state   *state)
{
  if (state == NULL)      // What did they expect us to do?
    return;

  print_state(stdout, "Unsubscribing from ", state, true);

  if (state->name) {
    free(state->name);
    state->name = NULL;
  }
  state->permissions = 0;
}

/*
 * Create a new "empty" transaction.
 *
 * The normal usage is to create an empty transaction and then immediately
 * populate it::
 *
 *     struct kstate_transaction *transaction = kstate_create_transaction();
 *     int ret = kstate_start_transaction(&transaction, state);
 *
 * and then eventually to destroy it:
 *
 *     int ret = kstate_unsubscribe(transaction);
 *     if (ret) {
 *       // deal with the error
 *     }
 *     kstate_destroy(&transaction);
 *
 * Returns the new transaction, or NULL if there was insufficient memory.
 */
extern struct kstate_transaction *kstate_create_transaction()
{
  struct kstate_transaction *new = malloc(sizeof(struct kstate_transaction));
  memset(new, 0, sizeof(*new));
  return new;
}

/*
 * Destroy a transaction created with 'kstate_create_transaction'.
 *
 * If a NULL pointer is given, then it is ignored, otherwise the transaction is
 * freed and the pointer 'transaction' is set to NULL.
 */
extern void kstate_free_transaction(struct kstate_transaction **transaction)
{
  if (*transaction) {
    //kstate_abort_transaction(*transaction);
    free(*transaction);
    *transaction = NULL;
  }
}

/*
 * Start a new transaction on a state.
 *
 * If 'transaction' was still active, it will first be aborted with
 * 'kstate_abort_transaction()'.
 *
 * * 'transaction' is the transaction to start.
 * * 'state' is the state on which to start the transaction.
 *
 * Returns 0 if starting the transaction succeeds, or a negative value if it
 * fails. The negative value will be ``-errno``, giving an indication of why
 * the function failed.
 */
extern int kstate_start_transaction(struct kstate_transaction *transaction,
                                    struct kstate_state       *state)
{
  if (state == NULL) {
    fprintf(stderr, "!!! kstate_start_transaction: Cannot start a transaction"
            " on a NULL state\n");
    return -EINVAL;
  }
  // Remember, unsubscribing from a state unsets its name
  if (state->name == NULL) {
    fprintf(stderr, "!!! kstate_start_transaction: Cannot start a transaction"
            " on an unset state\n");
    return -EINVAL;
  }
  print_state(stdout, "Starting transaction on ", state, true);

  //kstate_abort_transaction(*transaction);

  // Take a copy of the information we care about from the state

  size_t name_len = strlen(state->name);
  transaction->state.name = malloc(name_len + 1);
  if (!transaction->state.name) {
    return -ENOMEM;
  }
  strcpy(transaction->state.name, state->name);
  transaction->state.permissions = state->permissions;
  return 0;
}

// vim: set tabstop=8 softtabstop=2 shiftwidth=2 expandtab:
//
// Local Variables:
// tab-width: 8
// indent-tabs-mode: nil
// c-basic-offset: 2
// End:
