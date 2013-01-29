/*
 * Unit tests for kstate.
 *
 * Written using check - see http://check.sourceforge.net/
 *
 * Other unit tests are in kstate.py
 *
 * For the moment, this is where tests that can only really be done from C
 * live, and other tests may be found in kstate.py.
 */

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

#include <check.h>
#include <errno.h>
#include <stdlib.h>

#include "kstate.h"

START_TEST(new_and_free_state)
{
  kstate_state_p state = kstate_new_state();
  fail_if(state == NULL);

  kstate_free_state(&state);
  fail_unless(state == NULL);
}
END_TEST

START_TEST(free_NULL_state)
{
  kstate_state_p state = NULL;
  kstate_free_state(&state);
  fail_unless(state == NULL);
}
END_TEST

START_TEST(subscribe_with_NULL_name_fails)
{
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, NULL, KSTATE_READ|KSTATE_WRITE);
  ck_assert_int_eq(rv, -EINVAL);
  kstate_free_state(&state);
}
END_TEST

START_TEST(subscribe_with_zero_permissions_fails)
{
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, "Fred", 0);
  ck_assert_int_eq(rv, -EINVAL);
  kstate_free_state(&state);
}
END_TEST

START_TEST(subscribe_with_too_many_permissions_fails)
{
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, "Fred", 0xF);
  ck_assert_int_eq(rv, -EINVAL);
  kstate_free_state(&state);
}
END_TEST

START_TEST(subscribe_with_NULL_name_and_zero_permissions_fails)
{
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, NULL, 0);
  ck_assert_int_eq(rv, -EINVAL);
  kstate_free_state(&state);
}
END_TEST

START_TEST(subscribe_with_zero_length_name_fails)
{
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, "", KSTATE_READ|KSTATE_WRITE);
  ck_assert_int_eq(rv, -EINVAL);
  kstate_free_state(&state);
}
END_TEST

// 255 characters is too long
START_TEST(subscribe_with_too_long_name_fails)
{
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state,
                            "1234567890" "1234567890" "1234567890" "1234567890" "1234567890"
                            "1234567890" "1234567890" "1234567890" "1234567890" "1234567890"
                            "1234567890" "1234567890" "1234567890" "1234567890" "1234567890"
                            "1234567890" "1234567890" "1234567890" "1234567890" "1234567890"
                            "1234567890" "1234567890" "1234567890" "1234567890" "1234567890"
                            "12345",
                            KSTATE_READ|KSTATE_WRITE);
  ck_assert_int_eq(rv, -EINVAL);
  kstate_free_state(&state);
}
END_TEST

// But we expect 254 to be OK
START_TEST(subscribe_with_max_length_name_and_unsubscribe)
{
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state,
                            "1234567890" "1234567890" "1234567890" "1234567890" "1234567890"
                            "1234567890" "1234567890" "1234567890" "1234567890" "1234567890"
                            "1234567890" "1234567890" "1234567890" "1234567890" "1234567890"
                            "1234567890" "1234567890" "1234567890" "1234567890" "1234567890"
                            "1234567890" "1234567890" "1234567890" "1234567890" "123456",
                            KSTATE_READ|KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  kstate_unsubscribe_state(state);
  fail_if(kstate_state_is_subscribed(state));
  kstate_free_state(&state);
  fail_unless(state == NULL);
}
END_TEST

START_TEST(subscribe_with_dot_at_start_of_name_fails)
{
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, ".Fred", KSTATE_READ|KSTATE_WRITE);
  ck_assert_int_eq(rv, -EINVAL);
  kstate_free_state(&state);
}
END_TEST

START_TEST(subscribe_with_dot_at_end_of_name_fails)
{
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, "Fred.", KSTATE_READ|KSTATE_WRITE);
  ck_assert_int_eq(rv, -EINVAL);
  kstate_free_state(&state);
}
END_TEST

START_TEST(subscribe_with_adjacent_dots_in_name_fails)
{
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, "Fred..Jim", KSTATE_READ|KSTATE_WRITE);
  ck_assert_int_eq(rv, -EINVAL);
  kstate_free_state(&state);
}
END_TEST

// This is a very basic test of this, but there's not really any point in
// trying to be exhaustive.
START_TEST(subscribe_with_non_alphanumeric_in_name_fails)
{
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, "Fred&Jim", KSTATE_READ|KSTATE_WRITE);
  ck_assert_int_eq(rv, -EINVAL);
  kstate_free_state(&state);
}
END_TEST

START_TEST(subscribe_for_read_alone_fails)
{
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, "Fred.Read.Only", KSTATE_READ);
  ck_assert_int_eq(rv, -ENOENT);
  kstate_free_state(&state);
}
END_TEST

START_TEST(subscribe_for_write_is_actually_for_readwrite)
{
  char *state_name = kstate_get_unique_name("Fred");
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, state_name, KSTATE_WRITE);
  free(state_name);
  ck_assert_int_eq(rv, 0);

  uint32_t permissions = kstate_get_state_permissions(state);
  ck_assert_int_eq(permissions, KSTATE_WRITE|KSTATE_READ);

  kstate_free_state(&state);
}
END_TEST

START_TEST(subscribe_for_readwrite_and_unsubscribe_and_free)
{
  char *state_name = kstate_get_unique_name("Fred");
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, state_name, KSTATE_READ|KSTATE_WRITE);
  free(state_name);
  ck_assert_int_eq(rv, 0);

  kstate_unsubscribe_state(state);
  fail_if(kstate_state_is_subscribed(state));
  kstate_free_state(&state);
  fail_unless(state == NULL);
}
END_TEST

START_TEST(subscribe_for_readwrite_and_free)
{
  char *state_name = kstate_get_unique_name("Fred");
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, state_name, KSTATE_READ|KSTATE_WRITE);
  free(state_name);
  ck_assert_int_eq(rv, 0);

  kstate_free_state(&state);
  fail_unless(state == NULL);
}
END_TEST

START_TEST(query_state_name)
{
  char *state_name = kstate_get_unique_name("Fred");
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, state_name, KSTATE_READ|KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  const char *name = kstate_get_state_name(state);
  ck_assert_str_eq(state_name, name);

  free(state_name);

  kstate_unsubscribe_state(state);
  fail_if(kstate_state_is_subscribed(state));

  name = kstate_get_state_name(state);
  fail_unless(name == NULL);

  kstate_free_state(&state);

  name = kstate_get_state_name(state);
  fail_unless(name == NULL);
}
END_TEST

START_TEST(query_state_permissions)
{
  char *state_name = kstate_get_unique_name("Fred");
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, state_name, KSTATE_READ|KSTATE_WRITE);
  free(state_name);
  ck_assert_int_eq(rv, 0);

  uint32_t permissions = kstate_get_state_permissions(state);
  ck_assert_int_eq(permissions, KSTATE_READ|KSTATE_WRITE);

  kstate_unsubscribe_state(state);

  permissions = kstate_get_state_permissions(state);
  fail_unless(permissions == 0);

  kstate_free_state(&state);

  permissions = kstate_get_state_permissions(state);
  fail_unless(permissions == 0);
}
END_TEST

START_TEST(query_state_pointer)
{
  char *state_name = kstate_get_unique_name("Fred");
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, state_name, KSTATE_READ|KSTATE_WRITE);
  free(state_name);
  ck_assert_int_eq(rv, 0);

  void *ptr = kstate_get_state_ptr(state);
  fail_if(ptr == NULL);

  kstate_unsubscribe_state(state);

  ptr = kstate_get_state_ptr(state);
  fail_unless(ptr == NULL);

  kstate_free_state(&state);

  ptr = kstate_get_state_ptr(state);
  fail_unless(ptr == NULL);
}
END_TEST

// XXX At the moment, it is allowed to subscribe for WRITE, although
// XXX this is shorthand for READ|WRITE. Some decision needs to be made
// XXX about whether this laziness is good or bad.
START_TEST(subscribe_for_write_and_unsubscribe)
{
  char *state_name = kstate_get_unique_name("Fred");
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, state_name, KSTATE_WRITE);
  free(state_name);
  ck_assert_int_eq(rv, 0);

  kstate_unsubscribe_state(state);
  kstate_free_state(&state);
}
END_TEST

START_TEST(subscribe_for_write_then_for_read)
{
  char *state_name = kstate_get_unique_name("Fred");

  kstate_state_p state_w = kstate_new_state();
  int rv = kstate_subscribe_state(state_w, state_name, KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  kstate_state_p state_r = kstate_new_state();
  rv = kstate_subscribe_state(state_r, state_name, KSTATE_READ);
  ck_assert_int_eq(rv, 0);

  free(state_name);

  kstate_unsubscribe_state(state_w);
  kstate_free_state(&state_w);

  kstate_unsubscribe_state(state_r);
  kstate_free_state(&state_r);
}
END_TEST

START_TEST(subscribe_for_write_then_for_write)
{
  char *state_name = kstate_get_unique_name("Fred");

  kstate_state_p state_w1 = kstate_new_state();
  int rv = kstate_subscribe_state(state_w1, state_name, KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  kstate_state_p state_w2 = kstate_new_state();
  rv = kstate_subscribe_state(state_w2, state_name, KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  free(state_name);

  kstate_unsubscribe_state(state_w1);
  kstate_free_state(&state_w1);

  kstate_unsubscribe_state(state_w2);
  kstate_free_state(&state_w2);
}
END_TEST

START_TEST(subscribe_for_write_then_for_read_unsubscribe_other_order)
{
  char *state_name = kstate_get_unique_name("Fred");

  kstate_state_p state_w = kstate_new_state();
  int rv = kstate_subscribe_state(state_w, state_name, KSTATE_READ|KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  kstate_state_p state_r = kstate_new_state();
  rv = kstate_subscribe_state(state_r, state_name, KSTATE_READ);
  ck_assert_int_eq(rv, 0);

  free(state_name);

  kstate_unsubscribe_state(state_r);
  kstate_free_state(&state_r);

  kstate_unsubscribe_state(state_w);
  kstate_free_state(&state_w);
}
END_TEST

START_TEST(subscribe_with_NULL_state_fails)
{
  char *state_name = kstate_get_unique_name("Fred");
  int rv = kstate_subscribe_state(NULL, state_name, KSTATE_READ|KSTATE_WRITE);
  free(state_name);
  ck_assert_int_eq(rv, -EINVAL);
}
END_TEST

START_TEST(create_and_free_transaction)
{
  kstate_transaction_p transaction = kstate_new_transaction();
  fail_if(transaction == NULL);

  kstate_free_transaction(&transaction);
  fail_unless(transaction == NULL);
}
END_TEST

START_TEST(free_NULL_transaction)
{
  kstate_transaction_p transaction = NULL;

  kstate_free_transaction(&transaction);
  fail_unless(transaction == NULL);
}
END_TEST

START_TEST(start_transaction_with_NULL_transaction_fails)
{
  kstate_state_p state = kstate_new_state();
  fail_if(state == NULL);

  int rv = kstate_start_transaction(NULL, state, KSTATE_READ);
  ck_assert_int_eq(rv, -EINVAL);
}
END_TEST

START_TEST(start_transaction_with_NULL_state_fails)
{
  kstate_state_p state = NULL;

  kstate_transaction_p transaction = kstate_new_transaction();
  fail_if(transaction == NULL);

  int rv = kstate_start_transaction(transaction, state, KSTATE_READ);
  ck_assert_int_eq(rv, -EINVAL);
}
END_TEST

START_TEST(start_transaction_with_unset_state_fails)
{
  kstate_state_p state = kstate_new_state();

  kstate_transaction_p transaction = kstate_new_transaction();
  fail_if(transaction == NULL);

  int rv = kstate_start_transaction(transaction, state, KSTATE_READ);
  ck_assert_int_eq(rv, -EINVAL);
}
END_TEST

START_TEST(start_transaction_with_zero_permissions_fails)
{
  char *state_name = kstate_get_unique_name("Fred");
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, state_name, KSTATE_READ|KSTATE_WRITE);
  free(state_name);
  ck_assert_int_eq(rv, 0);

  kstate_transaction_p transaction = kstate_new_transaction();
  rv = kstate_start_transaction(transaction, state, 0);
  ck_assert_int_eq(rv, -EINVAL);
  kstate_free_state(&state);
}
END_TEST

START_TEST(start_transaction_with_too_many_permissions_fails)
{
  char *state_name = kstate_get_unique_name("Fred");
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, state_name, KSTATE_READ|KSTATE_WRITE);
  free(state_name);
  ck_assert_int_eq(rv, 0);

  kstate_transaction_p transaction = kstate_new_transaction();
  rv = kstate_start_transaction(transaction, state, 0xF);
  ck_assert_int_eq(rv, -EINVAL);
  kstate_free_state(&state);
}
END_TEST

START_TEST(start_write_transaction_on_readonly_state_fails)
{
  char *state_name = kstate_get_unique_name("Fred");

  // First, create a writeable state (we can't create a read-only state
  // from nothing)
  kstate_state_p state_w = kstate_new_state();
  int rv = kstate_subscribe_state(state_w, state_name, KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  // Now let's have a read--only "view" of that state
  kstate_state_p state_r = kstate_new_state();
  rv = kstate_subscribe_state(state_r, state_name, KSTATE_READ);
  ck_assert_int_eq(rv, 0);

  kstate_free_state(&state_w);
  free(state_name);

  kstate_transaction_p transaction = kstate_new_transaction();
  rv = kstate_start_transaction(transaction, state_r, KSTATE_WRITE);
  ck_assert_int_eq(rv, -EINVAL);
  kstate_free_state(&state_r);
}
END_TEST

START_TEST(start_write_transaction_on_writable_state)
{
  char *state_name = kstate_get_unique_name("Fred");
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, state_name, KSTATE_READ|KSTATE_WRITE);
  free(state_name);
  ck_assert_int_eq(rv, 0);

  kstate_transaction_p transaction = kstate_new_transaction();
  rv = kstate_start_transaction(transaction, state, KSTATE_READ|KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  kstate_free_transaction(&transaction);
  kstate_free_state(&state);
}
END_TEST

START_TEST(start_read_transaction_on_writable_state)
{
  char *state_name = kstate_get_unique_name("Fred");
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, state_name, KSTATE_READ|KSTATE_WRITE);
  free(state_name);
  ck_assert_int_eq(rv, 0);

  kstate_transaction_p transaction = kstate_new_transaction();
  rv = kstate_start_transaction(transaction, state, KSTATE_READ);
  ck_assert_int_eq(rv, 0);

  kstate_free_transaction(&transaction);
  kstate_free_state(&state);
}
END_TEST

START_TEST(start_write_only_transaction_is_actually_readwrite)
{
  char *state_name = kstate_get_unique_name("Fred");
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, state_name, KSTATE_READ|KSTATE_WRITE);
  free(state_name);
  ck_assert_int_eq(rv, 0);

  kstate_transaction_p transaction = kstate_new_transaction();
  rv = kstate_start_transaction(transaction, state, KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  kstate_free_transaction(&transaction);
  kstate_free_state(&state);
}
END_TEST

START_TEST(sensible_transaction_aborted)
{
  char *state_name = kstate_get_unique_name("Fred");
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, state_name, KSTATE_READ|KSTATE_WRITE);
  free(state_name);
  ck_assert_int_eq(rv, 0);

  kstate_transaction_p transaction = kstate_new_transaction();
  rv = kstate_start_transaction(transaction, state, KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  rv = kstate_abort_transaction(transaction);
  ck_assert_int_eq(rv, 0);
  fail_if(kstate_transaction_is_active(transaction));

  kstate_free_transaction(&transaction);
  fail_unless(transaction == NULL);

  kstate_unsubscribe_state(state);
  kstate_free_state(&state);
}
END_TEST

START_TEST(sensible_transaction_committed)
{
  char *state_name = kstate_get_unique_name("Fred");
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, state_name, KSTATE_READ|KSTATE_WRITE);
  free(state_name);
  ck_assert_int_eq(rv, 0);

  kstate_transaction_p transaction = kstate_new_transaction();
  rv = kstate_start_transaction(transaction, state, KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  rv = kstate_commit_transaction(transaction);
  ck_assert_int_eq(rv, 0);
  fail_if(kstate_transaction_is_active(transaction));

  kstate_free_transaction(&transaction);
  fail_unless(transaction == NULL);

  kstate_unsubscribe_state(state);
  kstate_free_state(&state);
}
END_TEST

START_TEST(commit_readonly_transaction_fails)
{
  char *state_name = kstate_get_unique_name("Fred");
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, state_name, KSTATE_READ|KSTATE_WRITE);
  free(state_name);
  ck_assert_int_eq(rv, 0);

  kstate_transaction_p transaction = kstate_new_transaction();
  rv = kstate_start_transaction(transaction, state, KSTATE_READ);
  ck_assert_int_eq(rv, 0);

  // Commit fails
  rv = kstate_commit_transaction(transaction);
  ck_assert_int_eq(rv, -EPERM);
  fail_unless(kstate_transaction_is_active(transaction));

  // But we can always abort
  rv = kstate_abort_transaction(transaction);
  ck_assert_int_eq(rv, 0);
  fail_if(kstate_transaction_is_active(transaction));

  kstate_free_transaction(&transaction);
  fail_unless(transaction == NULL);

  kstate_unsubscribe_state(state);
  kstate_free_state(&state);
}
END_TEST

START_TEST(free_transaction_also_aborts)  // or, at least, doesn't fall over
{
  char *state_name = kstate_get_unique_name("Fred");
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, state_name, KSTATE_READ|KSTATE_WRITE);
  free(state_name);
  ck_assert_int_eq(rv, 0);

  kstate_transaction_p transaction = kstate_new_transaction();
  rv = kstate_start_transaction(transaction, state, KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  kstate_free_transaction(&transaction);
  fail_unless(transaction == NULL);

  kstate_unsubscribe_state(state);
  kstate_free_state(&state);
}
END_TEST

START_TEST(query_transaction_name)
{
  char *state_name = kstate_get_unique_name("Fred");

  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, state_name, KSTATE_READ|KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  kstate_transaction_p transaction = kstate_new_transaction();
  rv = kstate_start_transaction(transaction, state, KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  const char *name = kstate_get_transaction_name(transaction);
  ck_assert_str_eq(name, state_name);

  free(state_name);

  rv = kstate_abort_transaction(transaction);
  ck_assert_int_eq(rv, 0);
  fail_if(kstate_transaction_is_active(transaction));

  name = kstate_get_transaction_name(transaction);
  fail_unless(name == NULL);

  kstate_free_transaction(&transaction);

  name = kstate_get_transaction_name(transaction);
  fail_unless(name == NULL);

  kstate_unsubscribe_state(state);
  kstate_free_state(&state);
}
END_TEST

START_TEST(query_transaction_state_permissions)
{
  char *state_name = kstate_get_unique_name("Fred");
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, state_name, KSTATE_READ|KSTATE_WRITE);
  free(state_name);
  ck_assert_int_eq(rv, 0);

  kstate_transaction_p transaction = kstate_new_transaction();
  rv = kstate_start_transaction(transaction, state, KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  uint32_t permissions = kstate_get_transaction_permissions(transaction);
  ck_assert_int_eq(permissions, KSTATE_READ|KSTATE_WRITE);

  rv = kstate_abort_transaction(transaction);
  ck_assert_int_eq(rv, 0);

  permissions = kstate_get_transaction_permissions(transaction);
  fail_unless(permissions == 0);

  kstate_free_transaction(&transaction);

  permissions = kstate_get_transaction_permissions(transaction);
  fail_unless(permissions == 0);

  kstate_unsubscribe_state(state);
  kstate_free_state(&state);
}
END_TEST

START_TEST(query_transaction_state_pointer)
{
  char *state_name = kstate_get_unique_name("Fred");
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, state_name, KSTATE_READ|KSTATE_WRITE);
  free(state_name);
  ck_assert_int_eq(rv, 0);

  kstate_transaction_p transaction = kstate_new_transaction();
  rv = kstate_start_transaction(transaction, state, KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  void *ptr = kstate_get_transaction_ptr(transaction);
  fail_if(ptr == NULL);

  rv = kstate_abort_transaction(transaction);
  ck_assert_int_eq(rv, 0);

  ptr = kstate_get_transaction_ptr(transaction);
  fail_unless(ptr == NULL);

  kstate_free_transaction(&transaction);

  ptr = kstate_get_transaction_ptr(transaction);
  fail_unless(ptr == NULL);

  kstate_unsubscribe_state(state);
  kstate_free_state(&state);
}
END_TEST

START_TEST(abort_transaction_twice_fails)
{
  char *state_name = kstate_get_unique_name("Fred");
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, state_name, KSTATE_READ|KSTATE_WRITE);
  free(state_name);
  ck_assert_int_eq(rv, 0);

  kstate_transaction_p transaction = kstate_new_transaction();
  rv = kstate_start_transaction(transaction, state, KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  rv = kstate_abort_transaction(transaction);
  ck_assert_int_eq(rv, 0);
  fail_if(kstate_transaction_is_active(transaction));

  rv = kstate_abort_transaction(transaction);
  ck_assert_int_eq(rv, -EINVAL);

  kstate_free_transaction(&transaction);

  kstate_unsubscribe_state(state);
  kstate_free_state(&state);
}
END_TEST

START_TEST(commit_transaction_twice_fails)
{
  char *state_name = kstate_get_unique_name("Fred");
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, state_name, KSTATE_READ|KSTATE_WRITE);
  free(state_name);
  ck_assert_int_eq(rv, 0);

  kstate_transaction_p transaction = kstate_new_transaction();
  rv = kstate_start_transaction(transaction, state, KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  rv = kstate_commit_transaction(transaction);
  ck_assert_int_eq(rv, 0);
  fail_if(kstate_transaction_is_active(transaction));

  rv = kstate_commit_transaction(transaction);
  ck_assert_int_eq(rv, -EINVAL);

  kstate_free_transaction(&transaction);

  kstate_unsubscribe_state(state);
  kstate_free_state(&state);
}
END_TEST

START_TEST(abort_NULL_fails)
{
  int rv = kstate_abort_transaction(NULL);
  ck_assert_int_eq(rv, -EINVAL);
}
END_TEST

START_TEST(commit_NULL_fails)
{
  int rv = kstate_commit_transaction(NULL);
  ck_assert_int_eq(rv, -EINVAL);
}
END_TEST

START_TEST(abort_unstarted_transaction_fails)
{
  kstate_transaction_p transaction = kstate_new_transaction();

  int rv = kstate_abort_transaction(transaction);
  ck_assert_int_eq(rv, -EINVAL);

  kstate_free_transaction(&transaction);
}
END_TEST

START_TEST(commit_unstarted_transaction_fails)
{
  kstate_transaction_p transaction = kstate_new_transaction();

  int rv = kstate_commit_transaction(transaction);
  ck_assert_int_eq(rv, -EINVAL);

  kstate_free_transaction(&transaction);
}
END_TEST

START_TEST(abort_freed_transaction_fails)
{
  char *state_name = kstate_get_unique_name("Fred");
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, state_name, KSTATE_READ|KSTATE_WRITE);
  free(state_name);
  ck_assert_int_eq(rv, 0);

  kstate_transaction_p transaction = kstate_new_transaction();
  rv = kstate_start_transaction(transaction, state, KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  kstate_free_transaction(&transaction);

  rv = kstate_abort_transaction(transaction);
  ck_assert_int_eq(rv, -EINVAL);

  kstate_unsubscribe_state(state);
  kstate_free_state(&state);
}
END_TEST

START_TEST(commit_freed_transaction_fails)
{
  char *state_name = kstate_get_unique_name("Fred");
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, state_name, KSTATE_READ|KSTATE_WRITE);
  free(state_name);
  ck_assert_int_eq(rv, 0);

  kstate_transaction_p transaction = kstate_new_transaction();
  rv = kstate_start_transaction(transaction, state, KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  kstate_free_transaction(&transaction);

  rv = kstate_commit_transaction(transaction);
  ck_assert_int_eq(rv, -EINVAL);

  kstate_free_transaction(&transaction);

  kstate_unsubscribe_state(state);
  kstate_free_state(&state);
}
END_TEST

// A transaction takes a copy of the state
START_TEST(transaction_aborted_after_state_freed)
{
  char *state_name = kstate_get_unique_name("Fred");
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, state_name, KSTATE_READ|KSTATE_WRITE);
  free(state_name);
  ck_assert_int_eq(rv, 0);

  kstate_transaction_p transaction = kstate_new_transaction();
  rv = kstate_start_transaction(transaction, state, KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  kstate_free_state(&state);
  fail_unless(state == NULL);

  rv = kstate_abort_transaction(transaction);
  ck_assert_int_eq(rv, 0);
  fail_if(kstate_transaction_is_active(transaction));

  kstate_free_transaction(&transaction);
}
END_TEST

// A transaction takes a copy of the state
START_TEST(transaction_committed_after_state_freed)
{
  char *state_name = kstate_get_unique_name("Fred");
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, state_name, KSTATE_READ|KSTATE_WRITE);
  free(state_name);
  ck_assert_int_eq(rv, 0);

  kstate_transaction_p transaction = kstate_new_transaction();
  rv = kstate_start_transaction(transaction, state, KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  kstate_free_state(&state);
  fail_unless(state == NULL);

  rv = kstate_commit_transaction(transaction);
  ck_assert_int_eq(rv, 0);
  fail_if(kstate_transaction_is_active(transaction));

  kstate_free_transaction(&transaction);
}
END_TEST

START_TEST(states_can_be_distinguished)
{
  char *state_name = kstate_get_unique_name("Fred");

  kstate_state_p state1 = kstate_new_state();
  int rv = kstate_subscribe_state(state1, state_name, KSTATE_READ|KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  kstate_state_p state2 = kstate_new_state();
  rv = kstate_subscribe_state(state2, state_name, KSTATE_READ|KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  free(state_name);

  uint32_t id1 = kstate_get_state_id(state1);
  ck_assert_int_ne(id1, 0);
  uint32_t id2 = kstate_get_state_id(state2);
  ck_assert_int_ne(id2, 0);

  ck_assert_int_ne(id1, id2);

  ck_assert_int_eq(id1, kstate_get_state_id(state1));
  ck_assert_int_eq(id2, kstate_get_state_id(state2));

  kstate_free_state(&state1);
  kstate_free_state(&state2);

  id1 = kstate_get_state_id(state1);
  ck_assert_int_eq(id1, 0);
  id2 = kstate_get_state_id(state2);
  ck_assert_int_eq(id2, 0);
}
END_TEST

START_TEST(transactions_can_be_distinguished)
{
  char *state_name = kstate_get_unique_name("Fred");
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, state_name, KSTATE_READ|KSTATE_WRITE);
  free(state_name);
  ck_assert_int_eq(rv, 0);

  kstate_transaction_p transaction1 = kstate_new_transaction();
  rv = kstate_start_transaction(transaction1, state, KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  kstate_transaction_p transaction2 = kstate_new_transaction();
  rv = kstate_start_transaction(transaction2, state, KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  uint32_t id1 = kstate_get_transaction_id(transaction1);
  ck_assert_int_ne(id1, 0);
  uint32_t id2 = kstate_get_transaction_id(transaction2);
  ck_assert_int_ne(id2, 0);

  ck_assert_int_ne(id1, id2);

  ck_assert_int_eq(id1, kstate_get_transaction_id(transaction1));
  ck_assert_int_eq(id2, kstate_get_transaction_id(transaction2));

  kstate_free_transaction(&transaction1);
  kstate_free_transaction(&transaction2);

  id1 = kstate_get_transaction_id(transaction1);
  ck_assert_int_eq(id1, 0);
  id2 = kstate_get_transaction_id(transaction2);
  ck_assert_int_eq(id2, 0);

  kstate_free_state(&state);
}
END_TEST

START_TEST(nested_transactions_same_state_commit_commit)
{
  char *state_name = kstate_get_unique_name("Fred");
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, state_name, KSTATE_READ|KSTATE_WRITE);
  free(state_name);
  ck_assert_int_eq(rv, 0);

  kstate_transaction_p transaction1 = kstate_new_transaction();
  rv = kstate_start_transaction(transaction1, state, KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  kstate_transaction_p transaction2 = kstate_new_transaction();
  rv = kstate_start_transaction(transaction2, state, KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  rv = kstate_commit_transaction(transaction2);
  ck_assert_int_eq(rv, 0);
  fail_if(kstate_transaction_is_active(transaction2));

  rv = kstate_commit_transaction(transaction1);
  ck_assert_int_eq(rv, 0);
  fail_if(kstate_transaction_is_active(transaction1));

  kstate_free_transaction(&transaction1);
  kstate_free_transaction(&transaction2);

  kstate_free_state(&state);
  fail_unless(state == NULL);
}
END_TEST

START_TEST(nested_transactions_same_state_commit_abort)
{
  char *state_name = kstate_get_unique_name("Fred");
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, state_name, KSTATE_READ|KSTATE_WRITE);
  free(state_name);
  ck_assert_int_eq(rv, 0);

  kstate_transaction_p transaction1 = kstate_new_transaction();
  rv = kstate_start_transaction(transaction1, state, KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  kstate_transaction_p transaction2 = kstate_new_transaction();
  rv = kstate_start_transaction(transaction2, state, KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  rv = kstate_commit_transaction(transaction2);
  ck_assert_int_eq(rv, 0);
  fail_if(kstate_transaction_is_active(transaction2));

  rv = kstate_abort_transaction(transaction1);
  ck_assert_int_eq(rv, 0);
  fail_if(kstate_transaction_is_active(transaction1));

  kstate_free_transaction(&transaction1);
  kstate_free_transaction(&transaction2);

  kstate_free_state(&state);
  fail_unless(state == NULL);
}
END_TEST

START_TEST(nested_transactions_same_state_abort_commit)
{
  char *state_name = kstate_get_unique_name("Fred");
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, state_name, KSTATE_READ|KSTATE_WRITE);
  free(state_name);
  ck_assert_int_eq(rv, 0);

  kstate_transaction_p transaction1 = kstate_new_transaction();
  rv = kstate_start_transaction(transaction1, state, KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  kstate_transaction_p transaction2 = kstate_new_transaction();
  rv = kstate_start_transaction(transaction2, state, KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  rv = kstate_abort_transaction(transaction2);
  ck_assert_int_eq(rv, 0);
  fail_if(kstate_transaction_is_active(transaction2));

  rv = kstate_commit_transaction(transaction1);
  ck_assert_int_eq(rv, 0);
  fail_if(kstate_transaction_is_active(transaction1));

  kstate_free_transaction(&transaction1);
  kstate_free_transaction(&transaction2);

  kstate_free_state(&state);
  fail_unless(state == NULL);
}
END_TEST

START_TEST(interleaved_transactions_same_state_commit_commit)
{
  char *state_name = kstate_get_unique_name("Fred");
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, state_name, KSTATE_READ|KSTATE_WRITE);
  free(state_name);
  ck_assert_int_eq(rv, 0);

  kstate_transaction_p transaction1 = kstate_new_transaction();
  rv = kstate_start_transaction(transaction1, state, KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  kstate_transaction_p transaction2 = kstate_new_transaction();
  rv = kstate_start_transaction(transaction2, state, KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  rv = kstate_commit_transaction(transaction1);
  ck_assert_int_eq(rv, 0);
  fail_if(kstate_transaction_is_active(transaction1));

  rv = kstate_commit_transaction(transaction2);
  ck_assert_int_eq(rv, 0);
  fail_if(kstate_transaction_is_active(transaction2));

  kstate_free_transaction(&transaction1);
  kstate_free_transaction(&transaction2);

  kstate_free_state(&state);
  fail_unless(state == NULL);
}
END_TEST

START_TEST(interleaved_transactions_same_state_commit_abort)
{
  char *state_name = kstate_get_unique_name("Fred");
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, state_name, KSTATE_READ|KSTATE_WRITE);
  free(state_name);
  ck_assert_int_eq(rv, 0);

  kstate_transaction_p transaction1 = kstate_new_transaction();
  rv = kstate_start_transaction(transaction1, state, KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  kstate_transaction_p transaction2 = kstate_new_transaction();
  rv = kstate_start_transaction(transaction2, state, KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  rv = kstate_commit_transaction(transaction1);
  ck_assert_int_eq(rv, 0);
  fail_if(kstate_transaction_is_active(transaction1));

  rv = kstate_abort_transaction(transaction2);
  ck_assert_int_eq(rv, 0);
  fail_if(kstate_transaction_is_active(transaction2));

  kstate_free_transaction(&transaction1);
  kstate_free_transaction(&transaction2);

  kstate_free_state(&state);
  fail_unless(state == NULL);
}
END_TEST

START_TEST(interleaved_transactions_same_state_abort_commit)
{
  char *state_name = kstate_get_unique_name("Fred");
  kstate_state_p state = kstate_new_state();
  int rv = kstate_subscribe_state(state, state_name, KSTATE_READ|KSTATE_WRITE);
  free(state_name);
  ck_assert_int_eq(rv, 0);

  kstate_transaction_p transaction1 = kstate_new_transaction();
  rv = kstate_start_transaction(transaction1, state, KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  kstate_transaction_p transaction2 = kstate_new_transaction();
  rv = kstate_start_transaction(transaction2, state, KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  rv = kstate_abort_transaction(transaction1);
  ck_assert_int_eq(rv, 0);
  fail_if(kstate_transaction_is_active(transaction1));

  rv = kstate_commit_transaction(transaction2);
  ck_assert_int_eq(rv, 0);
  fail_if(kstate_transaction_is_active(transaction2));

  kstate_free_transaction(&transaction1);
  kstate_free_transaction(&transaction2);

  kstate_free_state(&state);
  fail_unless(state == NULL);
}
END_TEST

Suite *test_kstate_suite(void)
{
  Suite *s = suite_create("Kstate");

  TCase *tc_core = tcase_create("core");
  // The code between the "TESTS" delimiters is auto-generated by the
  // 'extract_tests.py' script. This helps avoid having to fix compiler
  // warnings when I forget to include a new test in the list.
  // START TESTS
  tcase_add_test(tc_core, new_and_free_state);
  tcase_add_test(tc_core, free_NULL_state);
  tcase_add_test(tc_core, subscribe_with_NULL_name_fails);
  tcase_add_test(tc_core, subscribe_with_zero_permissions_fails);
  tcase_add_test(tc_core, subscribe_with_too_many_permissions_fails);
  tcase_add_test(tc_core, subscribe_with_NULL_name_and_zero_permissions_fails);
  tcase_add_test(tc_core, subscribe_with_zero_length_name_fails);
  tcase_add_test(tc_core, subscribe_with_too_long_name_fails);
  tcase_add_test(tc_core, subscribe_with_max_length_name_and_unsubscribe);
  tcase_add_test(tc_core, subscribe_with_dot_at_start_of_name_fails);
  tcase_add_test(tc_core, subscribe_with_dot_at_end_of_name_fails);
  tcase_add_test(tc_core, subscribe_with_adjacent_dots_in_name_fails);
  tcase_add_test(tc_core, subscribe_with_non_alphanumeric_in_name_fails);
  tcase_add_test(tc_core, subscribe_for_read_alone_fails);
  tcase_add_test(tc_core, subscribe_for_write_is_actually_for_readwrite);
  tcase_add_test(tc_core, subscribe_for_readwrite_and_unsubscribe_and_free);
  tcase_add_test(tc_core, subscribe_for_readwrite_and_free);
  tcase_add_test(tc_core, query_state_name);
  tcase_add_test(tc_core, query_state_permissions);
  tcase_add_test(tc_core, query_state_pointer);
  tcase_add_test(tc_core, subscribe_for_write_and_unsubscribe);
  tcase_add_test(tc_core, subscribe_for_write_then_for_read);
  tcase_add_test(tc_core, subscribe_for_write_then_for_write);
  tcase_add_test(tc_core, subscribe_for_write_then_for_read_unsubscribe_other_order);
  tcase_add_test(tc_core, subscribe_with_NULL_state_fails);
  tcase_add_test(tc_core, create_and_free_transaction);
  tcase_add_test(tc_core, free_NULL_transaction);
  tcase_add_test(tc_core, start_transaction_with_NULL_transaction_fails);
  tcase_add_test(tc_core, start_transaction_with_NULL_state_fails);
  tcase_add_test(tc_core, start_transaction_with_unset_state_fails);
  tcase_add_test(tc_core, start_transaction_with_zero_permissions_fails);
  tcase_add_test(tc_core, start_transaction_with_too_many_permissions_fails);
  tcase_add_test(tc_core, start_write_transaction_on_readonly_state_fails);
  tcase_add_test(tc_core, start_write_transaction_on_writable_state);
  tcase_add_test(tc_core, start_read_transaction_on_writable_state);
  tcase_add_test(tc_core, start_write_only_transaction_is_actually_readwrite);
  tcase_add_test(tc_core, sensible_transaction_aborted);
  tcase_add_test(tc_core, sensible_transaction_committed);
  tcase_add_test(tc_core, commit_readonly_transaction_fails);
  tcase_add_test(tc_core, free_transaction_also_aborts);
  tcase_add_test(tc_core, query_transaction_name);
  tcase_add_test(tc_core, query_transaction_state_permissions);
  tcase_add_test(tc_core, query_transaction_state_pointer);
  tcase_add_test(tc_core, abort_transaction_twice_fails);
  tcase_add_test(tc_core, commit_transaction_twice_fails);
  tcase_add_test(tc_core, abort_NULL_fails);
  tcase_add_test(tc_core, commit_NULL_fails);
  tcase_add_test(tc_core, abort_unstarted_transaction_fails);
  tcase_add_test(tc_core, commit_unstarted_transaction_fails);
  tcase_add_test(tc_core, abort_freed_transaction_fails);
  tcase_add_test(tc_core, commit_freed_transaction_fails);
  tcase_add_test(tc_core, transaction_aborted_after_state_freed);
  tcase_add_test(tc_core, transaction_committed_after_state_freed);
  tcase_add_test(tc_core, states_can_be_distinguished);
  tcase_add_test(tc_core, transactions_can_be_distinguished);
  tcase_add_test(tc_core, nested_transactions_same_state_commit_commit);
  tcase_add_test(tc_core, nested_transactions_same_state_commit_abort);
  tcase_add_test(tc_core, nested_transactions_same_state_abort_commit);
  tcase_add_test(tc_core, interleaved_transactions_same_state_commit_commit);
  tcase_add_test(tc_core, interleaved_transactions_same_state_commit_abort);
  tcase_add_test(tc_core, interleaved_transactions_same_state_abort_commit);
  // END TESTS
  suite_add_tcase(s, tc_core);

  return s;
}

int main (void)
{
 int number_failed;
 Suite *s = test_kstate_suite();
 SRunner *sr = srunner_create(s);
 srunner_run_all(sr, CK_NORMAL);
 number_failed = srunner_ntests_failed(sr);
 srunner_free(sr);
 if (number_failed == 0) {
   printf("\nThe light is GREEN\n");
 } else {
   printf("\nThe light is RED\n");
 }
 return number_failed;
}

// vim: set tabstop=8 softtabstop=2 shiftwidth=2 expandtab:
