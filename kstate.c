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
#include <ctype.h>    // for isalnum

// For shm_open and friends
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "kstate.h"

struct kstate_state {
  char        *name;
  uint32_t     permissions;

  int          shm_fd;    // The file id we get back from shm_open
};


struct kstate_transaction {
  struct kstate_state       state;

};

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
  if (name_len > KSTATE_MAX_NAME_LEN) {
    // Would it be more helpful to give all the characters?
    // Is anyone reading this?
    fprintf(stderr, "!!! kstate_subscribe: State name '%.5s..%s' is %u"
            " characters long, but the maximum length is %d characters\n",
            name, &name[name_len-5],
            (unsigned) name_len, KSTATE_MAX_NAME_LEN);
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
 * Return true if the given state is subscribed.
 */
extern bool kstate_state_is_subscribed(kstate_state_p state)
{
  // The detail of this is bound to change
  return state != NULL && state->name != NULL;
}

/*
 * Return true if the given transaction is active
 */
extern bool kstate_transaction_is_active(kstate_transaction_p transaction)
{
  // The detail of this is bound to change
  return (transaction != NULL) && (transaction->state.name != NULL);
}

/*
 * Return a state's name, or NULL if it is not subscribed.
 */
extern const char *kstate_get_state_name(kstate_state_p state)
{
  if (state && state->name) {
    // We ignore the leading '/' character, which the user did not specify
    return &state->name[1];
  } else {
    return NULL;
  }
}

/*
 * Return a transaction's state name, or NULL if it is not active.
 */
extern const char *kstate_get_transaction_state_name(kstate_transaction_p transaction)
{
  if (transaction && transaction->state.name) {
    // We ignore the leading '/' character, which the user did not specify
    return &transaction->state.name[1];
  } else {
    return NULL;
  }
}

/*
 * Return a state's permissions, or 0 if it is not subscribed.
 */
extern uint32_t kstate_get_state_permissions(kstate_state_p state)
{
  if (state && state->name) {
    // We ignore the leading '/' character, which the user did not specify
    return state->permissions;
  } else {
    return 0;
  }
}

/*
 * Return a transaction's state permissions, or 0 if it is not active.
 */
extern uint32_t kstate_get_transaction_state_permissions(kstate_transaction_p transaction)
{
  if (transaction && transaction->state.name) {
    return transaction->state.permissions;
  } else {
    return 0;
  }
}

// Note that if shm_fd is < -1, then it will not be mentioned
static void print_state(FILE       *stream,
                        const char *name,
                        uint32_t    permissions,
                        int         shm_fd)
{
  fprintf(stream, "State '%s' for ", name);
  if (permissions) {
    if (permissions & KSTATE_READ)
      fprintf(stream, "read");
    if ((permissions & KSTATE_READ) && (permissions & KSTATE_WRITE))
      fprintf(stream, "|");
    if (permissions & KSTATE_WRITE)
      fprintf(stream, "write");
  } else {
    fprintf(stream, "<no permissions>");
  }
  if (shm_fd == -1) {
    fprintf(stream, " <not open>");
  } else if (shm_fd > 0) {
    fprintf(stream, " fd %d", shm_fd);
  }
}

/*
 * Print a representation of 'state' on output 'stream'.
 *
 * Assumes the state is valid.
 *
 * If 'start' is non-NULL, print it before the state (with no added whitespace).
 * If 'eol' is true, then print a newline after the state.
 */
extern void kstate_print_state(FILE           *stream,
                               const char     *start,
                               kstate_state_p  state,
                               bool            eol)
{
  if (start)
    fprintf(stream, "%s", start);

  if (kstate_state_is_subscribed(state)) {
    print_state(stream,
                state->name+1,      // ignore the leading '/'
                state->permissions,
                state->shm_fd);
  } else {
    fprintf(stream, "State <unsubscribed>");
  }

  if (eol)
    fprintf(stream, "\n");
}

/*
 * Print a representation of 'transaction' on output 'stream'.
 *
 * Assumes the transaction is valid.
 *
 * If 'start' is non-NULL, print it before the transaction (with no added
 * whitespace).
 * If 'eol' is true, then print a newline after the transaction.
 */
extern void kstate_print_transaction(FILE                 *stream,
                                     const char           *start,
                                     kstate_transaction_p  transaction,
                                     bool                  eol)
{
  if (start)
    fprintf(stream, "%s", start);
  if (kstate_transaction_is_active(transaction)) {
    kstate_print_state(stream, "Transaction on ", &transaction->state, false);
  } else {
    fprintf(stream, "Transaction <not active>");
  }
  if (eol)
    fprintf(stream, "\n");
}

// XXX Also add "valid" functions for each of state_p and transaction_p
// XXX Also add get_ptr, get_name and get_permissions functions for state_p
// XXX Also add get_state function for transaction_p ???
// XXX Also add comparison function for state_p and transaction_p
// XXX Then move the definition of the two structures into this file,
// XXX so they are opaque in the header file.

/*
 * Create a new "empty" state.
 *
 * The normal usage is to create an empty state and then immediately
 * populate it::
 *
 *     kstate_state_p state = kstate_new_state();
 *     int ret = kstate_subscribe("State.Name", KSTATE_READ, state);
 *
 * and then eventually to destroy it::
 *
 *     int ret = kstate_unsubscribe(state);
 *     if (ret) {
 *       // deal with the error
 *     }
 *     kstate_free_state(&state);
 *
 * After which it can safely be reused, if you wish.
 *
 * Returns the new state, or NULL if there was insufficient memory.
 */
extern kstate_state_p kstate_new_state(void)
{
  kstate_state_p new = malloc(sizeof(*new));
  memset(new, 0, sizeof(*new));
  new->shm_fd = -1;
  return new;
}

/*
 * Destroy a state created with 'kstate_new_state'.
 *
 * If a NULL pointer is given, then it is ignored, otherwise the state is
 * freed and the pointer is set to NULL.
 */
extern void kstate_free_state(kstate_state_p *state)
{
  if (state && *state) {
    if (kstate_state_is_subscribed(*state)) {
      kstate_unsubscribe(*state);
    }
    struct kstate_state *s = (struct kstate_state *)(*state);
    free(s);
    *state = NULL;
  }
}

/*
 * Subscribe to a state.
 *
 * - ``name`` is the name of the state to subscribe to.
 * - ``permissions`` is constructed by OR'ing the permission flags
 *   KSTATE_READ and/or KSTATE_WRITE. At least one of those must be given.
 * - ``state`` is the actual state identifier, as amended by this function.
 *
 * A state name may contain A-Z, a-z, 0-9 and the dot (.) character. It may not
 * start or end with a dot, and may not contain adjacent dots. It must contain
 * at least one character. Note that the name will be copied into 'state'.
 *
 * If this is the first subscription to the named state, then the shared
 * data for the state will be created.
 *
 * Returns 0 if the subscription succeeds, or a negative value if it fails.
 * The negative value will be ``-errno``, giving an indication of why the
 * function failed.
 */
extern int kstate_subscribe(kstate_state_p         state,
                            const char            *name,
                            kstate_permissions_t   permissions)
{
  if (state == NULL) {
    fprintf(stderr, "!!! kstate_subscribe: state argument may not be NULL\n");
    return -EINVAL;
  }

  if (kstate_state_is_subscribed(state)) {
    fprintf(stderr, "!!! kstate_subscribe: state is still subscribeed\n");
    kstate_print_state(stderr, "!!! ", state, true);
    return -EINVAL;
  }

  printf("Subscribing to ");
  print_state(stdout, name, permissions, -99);
  printf("\n");

  size_t name_len = kstate_check_message_name(name);
  if (name_len == 0 || name_len > KSTATE_MAX_NAME_LEN) {
    return -EINVAL;
  }

  if (kstate_permissions_are_bad(permissions)) {
    return -EINVAL;
  }

  state->name = malloc(name_len + 1 + 1);
  if (!state->name) {
    return -ENOMEM;
  }
  state->name[0] = '/';
  strcpy(state->name + 1, name);
  state->permissions = permissions;

  int oflag = 0;
  mode_t mode = 0;
  if (permissions & KSTATE_WRITE) {
    oflag = O_RDWR | O_CREAT;
    // XXX Allow everyone any access, at least for the moment
    // XXX It is possible that we will want another version of this function
    // XXX which allows specifying the mode (the "normal" version of the
    // XXX function should always be the one that defaults to a "sensible"
    // XXX mode, whatever we decide that to be).
    mode = S_IRWXU | S_IRWXG | S_IRWXO;
  } else {
    // We always allow read
    oflag = O_RDONLY;
  }

  state->shm_fd = shm_open(state->name, oflag, mode);
  if (state->shm_fd < 0) {
    int rv = errno;
    free(state->name);
    state->name = NULL;
    state->permissions = 0;
    return -rv;
  }
  return 0;
}

/*
 * Unsubscribe from a state.
 *
 * - ``state`` is the state from which to unsubscribe.
 *
 * After this, the content of the state datastructure will have been
 * unset/freed. Unsubscribing from this same state value again will have no
 * effect.
 *
 * Note that transactions using the state keep their own copy of the state
 * information, and are not affected by this function - i.e., the state can
 * still be accessed via any transactions that are still open on it.
 *
 * Returns 0 if the unsubscription succeeds, or a negative value if it fails.
 * The negative value will be ``-errno``, giving an indication of why the
 * function failed.
 */
extern void kstate_unsubscribe(kstate_state_p  state)
{
  if (state == NULL)      // What did they expect us to do?
    return;

  kstate_print_state(stdout, "Unsubscribing from ", state, true);

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
 *     struct kstate_transaction *transaction = kstate_new_transaction();
 *     int ret = kstate_start_transaction(&transaction, state);
 *
 * and then eventually to destroy it::
 *
 *     int ret = kstate_unsubscribe(transaction);
 *     if (ret) {
 *       // deal with the error
 *     }
 *     kstate_free_transaction(&transaction);
 *
 * Returns the new transaction, or NULL if there was insufficient memory.
 */
extern struct kstate_transaction *kstate_new_transaction(void)
{
  struct kstate_transaction *new = malloc(sizeof(struct kstate_transaction));
  memset(new, 0, sizeof(*new));
  new->state.shm_fd = -1;
  return new;
}

/*
 * Destroy a transaction created with 'kstate_new_transaction'.
 *
 * If the transaction is still in progress, it will be aborted.
 *
 * If a NULL pointer is given, then it is ignored, otherwise the transaction is
 * freed and the pointer is set to NULL.
 */
extern void kstate_free_transaction(kstate_transaction_p *transaction)
{
  if (transaction && *transaction) {
    if (kstate_transaction_is_active(*transaction)) {
      kstate_abort_transaction(*transaction);
    }
    struct kstate_transaction *t = (struct kstate_transaction *)(*transaction);
    free(t);
    *transaction = NULL;
  }
}

/*
 * Start a new transaction on a state.
 *
 * If 'transaction' is still active, this will fail.
 *
 * * 'transaction' is the transaction to start.
 * * 'state' is the state on which to start the transaction.
 *
 * Note that a copy of the necessary information from 'state' will be held
 * in 'transaction'.
 *
 * Returns 0 if starting the transaction succeeds, or a negative value if it
 * fails. The negative value will be ``-errno``, giving an indication of why
 * the function failed.
 */
extern int kstate_start_transaction(kstate_transaction_p  transaction,
                                    kstate_state_p        state)
{
  if (transaction == NULL) {
    fprintf(stderr, "!!! kstate_start_transaction: transaction argument may"
            " not be NULL\n");
    return -EINVAL;
  }
  if (state == NULL) {
    fprintf(stderr, "!!! kstate_start_transaction: Cannot start a transaction"
            " on a NULL state\n");
    return -EINVAL;
  }
  if (kstate_transaction_is_active(transaction)) {
    fprintf(stderr, "!!! kstate_start_transaction: transaction is still active\n");
    kstate_print_transaction(stderr, "!!! ", transaction, true);
    return -EINVAL;
  }
  // Remember, unsubscribing from a state unsets its name
  if (state->name == NULL) {
    fprintf(stderr, "!!! kstate_start_transaction: Cannot start a transaction"
            " on an unset state\n");
    return -EINVAL;
  }

  kstate_print_state(stdout, "Starting Transaction on ", state, true);

  // Take a copy of the information we care about from the state

  size_t name_len = strlen(state->name);
  transaction->state.name = malloc(name_len + 1);
  if (!transaction->state.name) {
    return -ENOMEM;
  }
  strcpy(transaction->state.name, state->name);
  transaction->state.permissions = state->permissions;
  transaction->state.shm_fd = state->shm_fd;
  return 0;
}

/*
 * Abort a transaction.
 *
 * - ``transaction`` is the transaction to abort.
 *
 * After this, the content of the transaction datastructure will have been
 * unset/freed.
 *
 * It is not allowed to abort a transaction that has not been started.
 * In other words, you cannot abort a transaction before it has been started,
 * or after it has been aborted or committed.
 *
 * Returns 0 if the abort succeeds, or a negative value if it fails.
 * The negative value will be ``-errno``, giving an indication of why the
 * function failed.
 */
extern int kstate_abort_transaction(kstate_transaction_p  transaction)
{
  if (transaction == NULL) {     // What did they expect us to do?
    fprintf(stderr, "!!! kstate_abort_transaction: Cannot abort NULL transaction\n");
    return -EINVAL;
  }
  if (!kstate_transaction_is_active(transaction)) {
    fprintf(stderr, "!!! kstate_abort_transaction: transaction is not active\n");
    kstate_print_transaction(stderr, "!!! ", transaction, true);
    return -EINVAL;
  }

  kstate_print_transaction(stdout, "Aborting ", transaction, true);

  // Remember that we can always call this function on a previously aborted
  // transaction structure

  if (transaction->state.name) {
    free(transaction->state.name);
    transaction->state.name = NULL;
  }
  transaction->state.permissions = 0;
  return 0;
}

/*
 * Commit a transaction.
 *
 * - ``transaction`` is the transaction to commit.
 *
 * After this, the content of the transaction datastructure will have been
 * unset/freed.
 *
 * It is not allowed to commit a transaction that has not been started.
 * In other words, you cannot commit a transaction before it has been started,
 * or after it has been aborted or committed.
 *
 * Returns 0 if the commit succeeds, or a negative value if it fails.
 * The negative value will be ``-errno``, giving an indication of why the
 * function failed.
 */
extern int kstate_commit_transaction(struct kstate_transaction  *transaction)
{
  if (transaction == NULL) {    // What did they expect us to do?
    fprintf(stderr, "!!! kstate_commit_transaction: Cannot commit NULL transaction\n");
    return -EINVAL;
  }
  if (!kstate_transaction_is_active(transaction)) {
    fprintf(stderr, "!!! kstate_commit_transaction: transaction is not active\n");
    kstate_print_transaction(stderr, "!!! ", transaction, true);
    return -EINVAL;
  }

  kstate_print_transaction(stdout, "Committing ", transaction, true);

  if (transaction->state.name == NULL) {
    fprintf(stderr, "!!! kstate_commit_transaction: Cannot commit a transaction"
            " that has not been started\n");
    return -EINVAL;
  }

  if (transaction->state.name) {
    free(transaction->state.name);
    transaction->state.name = NULL;
  }
  transaction->state.permissions = 0;
  return 0;
}

// vim: set tabstop=8 softtabstop=2 shiftwidth=2 expandtab:
//
// Local Variables:
// tab-width: 8
// indent-tabs-mode: nil
// c-basic-offset: 2
// End:
