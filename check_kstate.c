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

#include "kstate.h"

START_TEST(subscribe_with_null_state_name_fails)
{
  struct kstate_state state;
  int rv = kstate_subscribe(&state, NULL, KSTATE_READ);
  ck_assert_int_eq(rv, -EINVAL);
}
END_TEST

START_TEST(subscribe_with_zero_permissions_fails)
{
  struct kstate_state state;
  int rv = kstate_subscribe(&state, "Fred", 0);
  ck_assert_int_eq(rv, -EINVAL);
}
END_TEST

START_TEST(subscribe_with_null_state_name_and_zero_permissions_fails)
{
  struct kstate_state state;
  int rv = kstate_subscribe(&state, NULL, 0);
  ck_assert_int_eq(rv, -EINVAL);
}
END_TEST

START_TEST(subscribe_and_unsubscribe)
{
  struct kstate_state state;
  int rv = kstate_subscribe(&state, "Fred", KSTATE_READ);
  ck_assert_int_eq(rv, 0);

  kstate_unsubscribe(&state);
  fail_unless(state.name == NULL);
}
END_TEST

START_TEST(create_and_free_state)
{
  struct kstate_state *state = kstate_create_state();
  fail_if(state == NULL);

  kstate_free_state(&state);
  fail_unless(state == NULL);
}
END_TEST

START_TEST(free_NULL_state_fails)
{
  struct kstate_state *state = NULL;

  kstate_free_state(&state);
  fail_unless(state == NULL);
}
END_TEST

START_TEST(subscribe_with_NULL_state_fails)
{
  int rv = kstate_subscribe(NULL, "Fred", KSTATE_READ);
  ck_assert_int_eq(rv, -EINVAL);
}
END_TEST

START_TEST(create_and_free_transaction)
{
  struct kstate_transaction *transaction = kstate_create_transaction();
  fail_if(transaction == NULL);

  kstate_free_transaction(&transaction);
  fail_unless(transaction == NULL);
}
END_TEST

START_TEST(free_NULL_transaction_fails)
{
  struct kstate_transaction *transaction = NULL;

  kstate_free_transaction(&transaction);
  fail_unless(transaction == NULL);
}
END_TEST

START_TEST(start_transaction_with_NULL_transaction_fails)
{
  struct kstate_state *state = kstate_create_state();
  fail_if(state == NULL);

  int rv = kstate_start_transaction(NULL, state);
  ck_assert_int_eq(rv, -EINVAL);
}
END_TEST

START_TEST(start_transaction_with_NULL_state_fails)
{
  struct kstate_state *state = NULL;

  struct kstate_transaction *transaction = kstate_create_transaction();
  fail_if(transaction == NULL);

  int rv = kstate_start_transaction(transaction, state);
  ck_assert_int_eq(rv, -EINVAL);
}
END_TEST

START_TEST(sensible_transaction_aborted)
{
  struct kstate_state state;
  int rv = kstate_subscribe(&state, "Fred", KSTATE_READ|KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  struct kstate_transaction transaction;
  rv = kstate_start_transaction(&transaction, &state);
  ck_assert_int_eq(rv, 0);

  kstate_unsubscribe(&state);
  fail_unless(state.name == NULL);

  rv = kstate_abort_transaction(&transaction);
  ck_assert_int_eq(rv, 0);
  fail_unless(transaction.state.name == NULL);
}
END_TEST

START_TEST(sensible_transaction_committed)
{
  struct kstate_state state;
  int rv = kstate_subscribe(&state, "Fred", KSTATE_READ|KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  struct kstate_transaction transaction;
  rv = kstate_start_transaction(&transaction, &state);
  ck_assert_int_eq(rv, 0);

  kstate_unsubscribe(&state);
  fail_unless(state.name == NULL);

  rv = kstate_commit_transaction(&transaction);
  ck_assert_int_eq(rv, 0);
  fail_unless(transaction.state.name == NULL);
}
END_TEST

START_TEST(abort_transaction_twice_succeeds)
{
  struct kstate_state state;
  int rv = kstate_subscribe(&state, "Fred", KSTATE_READ|KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  struct kstate_transaction transaction;
  rv = kstate_start_transaction(&transaction, &state);
  ck_assert_int_eq(rv, 0);

  kstate_unsubscribe(&state);
  fail_unless(state.name == NULL);

  rv = kstate_abort_transaction(&transaction);
  ck_assert_int_eq(rv, 0);
  fail_unless(transaction.state.name == NULL);

  rv = kstate_abort_transaction(&transaction);
  ck_assert_int_eq(rv, 0);
}
END_TEST

START_TEST(commit_transaction_twice_fails)
{
  struct kstate_state state;
  int rv = kstate_subscribe(&state, "Fred", KSTATE_READ|KSTATE_WRITE);
  ck_assert_int_eq(rv, 0);

  struct kstate_transaction transaction;
  rv = kstate_start_transaction(&transaction, &state);
  ck_assert_int_eq(rv, 0);

  kstate_unsubscribe(&state);
  fail_unless(state.name == NULL);

  rv = kstate_commit_transaction(&transaction);
  ck_assert_int_eq(rv, 0);
  fail_unless(transaction.state.name == NULL);

  rv = kstate_commit_transaction(&transaction);
  ck_assert_int_eq(rv, -EINVAL);
}
END_TEST

Suite *test_kstate_suite(void)
{
  Suite *s = suite_create("Kstate");

  TCase *tc_core = tcase_create("core");
  // START TESTS
  tcase_add_test(tc_core, subscribe_with_null_state_name_fails);
  tcase_add_test(tc_core, subscribe_with_zero_permissions_fails);
  tcase_add_test(tc_core, subscribe_with_null_state_name_and_zero_permissions_fails);
  tcase_add_test(tc_core, subscribe_and_unsubscribe);
  tcase_add_test(tc_core, create_and_free_state);
  tcase_add_test(tc_core, free_NULL_state_fails);
  tcase_add_test(tc_core, subscribe_with_NULL_state_fails);
  tcase_add_test(tc_core, create_and_free_transaction);
  tcase_add_test(tc_core, free_NULL_transaction_fails);
  tcase_add_test(tc_core, start_transaction_with_NULL_transaction_fails);
  tcase_add_test(tc_core, start_transaction_with_NULL_state_fails);
  tcase_add_test(tc_core, sensible_transaction_aborted);
  tcase_add_test(tc_core, sensible_transaction_committed);
  tcase_add_test(tc_core, abort_transaction_twice_succeeds);
  tcase_add_test(tc_core, commit_transaction_twice_fails);
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
   printf("The light is GREEN\n");
 } else {
   printf("The light is RED\n");
 }
 return number_failed;
}

// vim: set tabstop=8 softtabstop=2 shiftwidth=2 expandtab:
