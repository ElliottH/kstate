/*
 * Unit tests for kstate.
 *
 * Written using check - see http://check.sourceforge.net/
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

START_TEST(test_null_state_name1)
{
  struct kstate_state state;
  int rv = kstate_subscribe(&state, NULL, 0);
  ck_assert_int_eq(rv, -EINVAL);
}
END_TEST

START_TEST(test_null_state_name2)
{
  struct kstate_state state;
  int rv = kstate_subscribe(&state, NULL, 0);
  fail_unless(rv == -EINVAL, "A NULL state name is an invalid value");
}
END_TEST

Suite *test_kstate_suite(void)
{
  Suite *s = suite_create("Kstate");

  /* Core test case */
  TCase *tc_core = tcase_create("Core");
  tcase_add_test(tc_core, test_null_state_name1);
  tcase_add_test(tc_core, test_null_state_name2);
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
 return number_failed;
}

// vim: set tabstop=8 softtabstop=2 shiftwidth=2 expandtab:
