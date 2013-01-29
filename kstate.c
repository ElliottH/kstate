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
#include <time.h>     // for strftime
#include <sys/time.h> // for gettimeofday

#include <sys/types.h>
#include <unistd.h>

// For shm_open and friends
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "kstate.h"

struct kstate_state {
  char      *name;        // The name of our shared memory object
  uint32_t   permissions; // Our idea of its permissions

  int        shm_fd;      // The file id we get back from shm_open
  void      *map_addr;    // The shared memory associated with it
  size_t     map_length;  // and how much shared memory there is
};


struct kstate_transaction {
  char      *name;        // The name of our shared memory object

  uint32_t   id;          // A simple id for this transaction
  uint32_t   permissions; // The permissions for this transaction

  void      *map_addr;    // The shared memory associated with it
  size_t     map_length;  // and how much shared memory there is
};

/*
 * Given a message name, is it valid?
 *
 * We have nothing to say on maximum length.
 *
 * Returns the name length if it's OK, 0 if it's naughty
 */
static int kstate_check_name(const char *name)
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

static int num_digits(int value)
{
  int count = 0;
  do
  {
    ++count;
    value /= 10;
  }
  while (value);
  return count;
}

/*
 * Return a unique valid state name starting with prefix.
 *
 * The name is composed of:
 *
 * * the prefix string
 * * the number of microseconds since the epoch
 * * our process id
 * * a statically increasing integer
 *
 * separated by dots. Thus it is only as "unique" as afforded by the
 * accuracy of gettimeofday - i.e., it relies on the apparent time
 * thus reported having changed.
 *
 * For most purposes, this should be sufficient.
 *
 * The caller is responsible for freeing the returned string.
 *
 * Returns NULL if it is not possible to make such a name with the given
 * prefix.
 */
extern char *kstate_get_unique_name(const char *prefix)
{
  static uint32_t extra = 0;

  if (prefix == NULL) {
    fprintf(stderr, "!!! kstate_get_unique_name: Prefix may not be NULL\n");
    return NULL;
  }

  size_t prefix_len = strlen(prefix);

  struct timeval tv;
  int rv = gettimeofday(&tv, NULL);
  if (rv) {
    fprintf(stderr, "!!! kstate_get_unique_name: Error getting time-of-day: %d %s\n",
            errno, strerror(errno));
    return NULL;
  }

  pid_t pid = getpid();

  char *name = malloc(prefix_len + 1 +
                      num_digits(tv.tv_sec) + 6 + 1 +
                      num_digits(pid) + 1 + num_digits(extra) + 1);
  if (name == NULL) return NULL;
  sprintf(name, "%s.%ld%06ld.%u.%u", prefix, tv.tv_sec, tv.tv_usec, (uint32_t) pid, extra);

  ++extra;

  if (!kstate_check_name(name)) {
    return NULL;
  }

  return name;
}

static bool state_permissions_are_bad(uint32_t permissions)
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

static bool transaction_permissions_are_bad(uint32_t permissions)
{
  if (!permissions) {
    fprintf(stderr, "!!! kstate_start_transaction: Unset permissions bits (0x0) not allowed\n");
    return true;
  }
  else if (permissions & ~(KSTATE_READ | KSTATE_WRITE)) {
    fprintf(stderr, "!!! kstate_start_transaction: Unexpected permission bits 0x%x in 0x%x\n",
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
  return state != NULL && state->name;
}

/*
 * Return true if the given transaction is active
 */
extern bool kstate_transaction_is_active(kstate_transaction_p transaction)
{
  // The detail of this is bound to change
  return transaction != NULL && transaction->name;
}

/*
 * Return a state's name, or NULL if it is not subscribed.
 */
extern const char *kstate_get_state_name(kstate_state_p state)
{
  if (kstate_state_is_subscribed(state)) {
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
  if (kstate_transaction_is_active(transaction)) {
    // We ignore the leading '/' character, which the user did not specify
    return &transaction->name[1];
  } else {
    return NULL;
  }
}

/*
 * Return a state's permissions, or 0 if it is not subscribed.
 */
extern uint32_t kstate_get_state_permissions(kstate_state_p state)
{
  if (kstate_state_is_subscribed(state)) {
    return state->permissions;
  } else {
    return 0;
  }
}

/*
 * Return a transaction's permissions, or 0 if it is not active.
 */
extern uint32_t kstate_get_transaction_permissions(kstate_transaction_p transaction)
{
  if (kstate_transaction_is_active(transaction)) {
    return transaction->permissions;
  } else {
    return 0;
  }
}

/*
 * Return a transaction's id, or 0 if it is not active.
 *
 * We do not say anything about the value of the id, except that 0 means the
 * transaction is not active, the same transaction always has the same id, and
 * two separate transactions have distinct ids.
 */
extern uint32_t kstate_get_transaction_id(kstate_transaction_p transaction)
{
  if (kstate_transaction_is_active(transaction)) {
    return transaction->id;
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
    fprintf(stream, " on <no fd>");
  } else if (shm_fd > 0) {
    fprintf(stream, " on fd %d", shm_fd);
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

static void print_transaction(FILE       *stream,
                              uint32_t    id,
                              const char *name,
                              uint32_t    permissions)
{
  fprintf(stream, "Transaction %u for ", id);
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
  fprintf(stream, " on '%s'", name);
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
    print_transaction(stream,
                      transaction->id,
                      transaction->name+1,      // ignore the leading '/'
                      transaction->permissions);
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

/*
 * Create a new "empty" state.
 *
 * The normal usage is to create an empty state and then immediately
 * populate it::
 *
 *     kstate_state_p state = kstate_new_state();
 *     int ret = kstate_subscribe("State.Name", KSTATE_READ|KSTATE_WRITE, state);
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
 * Free a state created with 'kstate_new_state'.
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
 *   KSTATE_WRITE by itself is regarded as equivalent to KSTATE_WRITE|KSTATE_READ.
 * - ``state`` is the actual state identifier, as amended by this function.
 *
 * A state name may contain A-Z, a-z, 0-9 and the dot (.) character. It may not
 * start or end with a dot, and may not contain adjacent dots. It must contain
 * at least one character. Note that the name will be copied into 'state'.
 *
 * If this is the first subscription to the named state, then the shared
 * data for the state will be created.
 *
 * Note that the first subscription to a state cannot be read-only, as there is
 * nothing to read -i.e., the first subscription to a state must be for
 * KSTATE_WRITE|KSTATE_READ.
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

  size_t name_len = kstate_check_name(name);
  if (name_len == 0) {
    return -EINVAL;
  }

  if (state_permissions_are_bad(permissions)) {
    return -EINVAL;
  }

  state->name = malloc(name_len + 1 + 1);
  if (!state->name) return -ENOMEM;

  state->name[0] = '/';
  strcpy(state->name + 1, name);

  // If we had a legitimate permissions set that doesn't include READ,
  // add READ back in
  if (!(permissions & KSTATE_READ)) {
    permissions |= KSTATE_READ;
  }

  state->permissions = permissions;

  int oflag = 0;
  mode_t mode = 0;
  bool creating = false;
  if (permissions & KSTATE_WRITE) {
    oflag = O_RDWR | O_CREAT;
    // XXX Allow everyone any access, at least for the moment
    // XXX It is possible that we will want another version of this function
    // XXX which allows specifying the mode (the "normal" version of the
    // XXX function should always be the one that defaults to a "sensible"
    // XXX mode, whatever we decide that to be).
    mode = S_IRWXU | S_IRWXG | S_IRWXO;
    creating = true;
  } else {
    // We always allow read
    oflag = O_RDONLY;
  }

  state->shm_fd = shm_open(state->name, oflag, mode);
  if (state->shm_fd < 0) {
    int rv = errno;
    fprintf(stderr, "!!! kstate_subscribe: Error in shm_open(\"%s\", 0x%x, 0x%x): %d %s\n",
            state->name, oflag, mode, rv, strerror(rv));
    free(state->name);
    state->name = NULL;
    state->permissions = 0;
    return -rv;
  }

  long page_size = sysconf(_SC_PAGESIZE);

  // If we're creating the shared memory object, we need to set a size,
  // or it will be zero sized.
  // For the moment, we always set the same size, one page.
  if (creating) {
    // Caveat emptor - from the man page:
    //
    //    If the file previously was larger than this size, the extra data is
    //    lost. If  the file  previously was  shorter, it is extended, and the
    //    extended part reads as null bytes ('\0').
    //
    int rv = ftruncate(state->shm_fd, page_size);
    if (rv) {
      int rv = errno;
      kstate_print_state(stderr, "!!! kstate_subscribe:"
                         " Error in setting shared memory size for ", state, false);
      fprintf(stderr, " to 0x%x: %d %s\n", (uint32_t)page_size, rv, strerror(rv));
      free(state->name);
      state->name = NULL;
      state->permissions = 0;
      // NB: we're not doing shm_unlink...
      return -rv;
    }
  }

  // Some defaults just for now...
  int prot = PROT_READ;
  int flags = MAP_SHARED;

  // Again, by default map the whole available area, starting at the
  // start of the "file".
  state->map_length = page_size;
  state->map_addr = mmap(NULL, state->map_length, prot, flags, state->shm_fd, 0);
  if (state->map_addr == MAP_FAILED) {
    int rv = errno;
    kstate_print_state(stderr, "!!! kstate_subscribe:"
                       " Error in mapping shared memory for ", state, false);
    fprintf(stderr, ": %d %s\n", rv, strerror(rv));
    free(state->name);
    state->name = NULL;
    state->permissions = 0;
    state->map_addr = 0;
    state->map_length = 0;
    // NB: we're not doing shm_unlink...
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

  if (state->map_addr != 0 && state->map_addr != MAP_FAILED) {
    int rv = munmap(state->map_addr, state->map_length);
    if (rv) {
      rv = errno;
      kstate_print_state(stderr, "!!! kstate_unsubscribe:"
                         " Error in freeing shared memory for ", state, false);
      fprintf(stderr, ": %d %s\n", rv, strerror(rv));
      // But there's not much we can do about it...
    }
    state->map_addr = 0;
    state->map_length = 0;
  }

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
 *     int ret = kstate_start_transaction(&transaction, state, KSTATE_WRITE);
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
  static uint32_t next_transaction_id = 1;    // because 0 is reserved

  struct kstate_transaction *new = malloc(sizeof(struct kstate_transaction));
  memset(new, 0, sizeof(*new));
  new->id = next_transaction_id++;
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
 * - 'permissions' is constructed by OR'ing the permission flags
 *   KSTATE_READ and/or KSTATE_WRITE. At least one of those must be given.
 *   KSTATE_WRITE by itself is regarded as equivalent to KSTATE_WRITE|KSTATE_READ.
 *
 * Note that copy of the state will be taken, so that the transaction
 * can continue to access the state's shared memory even if the particular
 * 'state' is unsubscribed. However, this is not enough information to
 * reconstruct/return the entirety of the original 'state', as we do not
 * (for instance) remember the shared memory object used internally as an
 * intermediary when creating a state.
 *
 * Returns 0 if starting the transaction succeeds, or a negative value if it
 * fails. The negative value will be ``-errno``, giving an indication of why
 * the function failed.
 */
extern int kstate_start_transaction(kstate_transaction_p  transaction,
                                    kstate_state_p        state,
                                    uint32_t              permissions)
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
  if (!kstate_state_is_subscribed(state)) {
    fprintf(stderr, "!!! kstate_start_transaction: Cannot start a transaction"
            " on an unsubscribed state\n");
    return -EINVAL;
  }

  kstate_print_state(stdout, "Starting Transaction on ", state, true);

  if (transaction_permissions_are_bad(permissions)) {
    return -EINVAL;
  }

  // If we had a legitimate permissions set that doesn't include READ,
  // add READ back in
  if (!(permissions & KSTATE_READ)) {
    permissions |= KSTATE_READ;
  }

  if ((permissions & KSTATE_WRITE) && !(state->permissions & KSTATE_WRITE)) {
    fprintf(stderr, "!!! kstate_start_transaction: Cannot start a write"
            " transaction on a read-only state\n");
    kstate_print_state(stderr, "!!! ", state, true);
    return -EINVAL;
  }

  transaction->permissions = permissions;

  size_t name_len = strlen(state->name);
  transaction->name = malloc(name_len + 1);
  if (transaction->name == NULL) return -ENOMEM;

  strcpy(transaction->name, state->name);
  transaction->map_length = state->map_length;

  // Some defaults just for now...
  int prot = PROT_READ | PROT_WRITE;
  int flags = MAP_SHARED;

  transaction->map_addr = mmap(NULL, transaction->map_length, prot, flags, state->shm_fd, 0);
  if (transaction->map_addr == MAP_FAILED) {
    int rv = errno;
    kstate_print_state(stderr, "!!! kstate_start_transaction:"
                       " Error in mapping shared memory for Transaction on ", state, false);
    fprintf(stderr, ": %d %s\n", rv, strerror(rv));
    free(transaction->name);
    transaction->name = NULL;
    transaction->permissions = 0;
    transaction->map_addr = 0;
    transaction->map_length = 0;
    return -rv;
  }

  kstate_print_transaction(stdout, "Started ", transaction, true);

  return 0;
}

static int finish_transaction(char *caller,
                              kstate_transaction_p  transaction)
{
  if (transaction->map_addr != 0 && transaction->map_addr != MAP_FAILED) {
    int rv = munmap(transaction->map_addr, transaction->map_length);
    if (rv) {
      rv = errno;
      fprintf(stderr, "!!! %s: ", caller);
      kstate_print_transaction(stderr, " Error in freeing shared memory for ", transaction, false);
      fprintf(stderr, ": %d %s\n", rv, strerror(rv));
      return -rv;
    }
    transaction->map_addr = 0;
    transaction->map_length = 0;
  }

  if (transaction->name) {
    free(transaction->name);
    transaction->name = NULL;
  }

  transaction->permissions = 0;
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

  int rv = finish_transaction("kstate_abort_transaction", transaction);
  return rv;
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

  int rv = finish_transaction("kstate_commit_transaction", transaction);
  return rv;
}

// vim: set tabstop=8 softtabstop=2 shiftwidth=2 expandtab:
//
// Local Variables:
// tab-width: 8
// indent-tabs-mode: nil
// c-basic-offset: 2
// End:
