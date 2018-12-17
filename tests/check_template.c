#include <check.h>
#include <stdlib.h>
#include <stdio.h>

#include "../src/template.h"
#include "../src/parse.h"
#include "../src/maps.h"

#define POLICIES_DIR "sample_policy_files/"
#define NESTED_IF_FILENAME POLICIES_DIR "nested_templates.if"

extern FILE * yyin;
extern int yyparse();

START_TEST (test_replace_m4) {
	char *orig1 = "$1_t";

	struct string_list *args = malloc(sizeof(struct string_list));
	args->string = strdup("foo");
	args->next = malloc(sizeof(struct string_list));
	args->next->string = strdup("bar");
	args->next->next = NULL;

	char *res = replace_m4(orig1, args);

	ck_assert_ptr_nonnull(res);
	ck_assert_str_eq("foo_t", res);

	free(res);

	char *orig2 = "$2";

	res = replace_m4(orig2, args);
	ck_assert_ptr_nonnull(res);
	ck_assert_str_eq("bar", res);

	free(res);

	char *orig3 = "test_$1_test";

	res = replace_m4(orig3, args);
	ck_assert_ptr_nonnull(res);
	ck_assert_str_eq("test_foo_test", res);

	free(res);

	char *orig4 = "test$2$1";

	res = replace_m4(orig4, args);
	ck_assert_ptr_nonnull(res);
	ck_assert_str_eq("testbarfoo", res);

	free(res);

	free_string_list(args);

}
END_TEST

START_TEST (test_replace_m4_too_few_args) {
	struct string_list *args = malloc(sizeof(struct string_list));
	args->string = strdup("foo");
	args->next = malloc(sizeof(struct string_list));
	args->next->string = strdup("bar");
	args->next->next = NULL;
	
	char *orig = "$3_t";

	ck_assert_ptr_null(replace_m4(orig, args));

	free_string_list(args);

}
END_TEST

START_TEST (test_replace_m4_list) {

	struct string_list *caller_args = malloc(sizeof(struct string_list));
	caller_args->string = strdup("foo");
	caller_args->next = malloc(sizeof(struct string_list));
	caller_args->next->string = strdup("bar");
	caller_args->next->next = NULL;

	struct string_list *called_args = malloc(sizeof(struct string_list));
	called_args->string = strdup("$2");
	called_args->next = malloc(sizeof(struct string_list));
	called_args->next->string = strdup("$1");
	called_args->next->next = NULL;

	struct string_list *ret = replace_m4_list(caller_args, called_args);

	ck_assert_ptr_nonnull(ret);
	ck_assert_str_eq(ret->string, "bar");
	ck_assert_ptr_nonnull(ret->next);
	ck_assert_str_eq(ret->next->string, "foo");
	ck_assert_ptr_null(ret->next->next);

	free_string_list(caller_args);
	free_string_list(called_args);
	free_string_list(ret);

}
END_TEST

struct policy_node *ast;

START_TEST (test_nested_template_declarations) {

	ast = NULL;

	yyin = fopen(NESTED_IF_FILENAME, "r");
	yyparse();
	fclose(yyin);

	struct string_list *called_args = malloc(sizeof(struct string_list));
	called_args->string = strdup("first");
	called_args->next = malloc(sizeof(struct string_list));
	called_args->next->string = strdup("second");
	called_args->next->next = malloc(sizeof(struct string_list));
	called_args->next->next->string = strdup("third");
	called_args->next->next->next = NULL;

	ck_assert_int_eq(SELINT_SUCCESS, add_template_declarations("outer", called_args, NULL, "nested_interfaces"));

	ck_assert_str_eq("nested_interfaces", look_up_in_type_map("first_t"));
	ck_assert_str_eq("nested_interfaces", look_up_in_type_map("third_foo_t"));
	ck_assert_str_eq("nested_interfaces", look_up_in_type_map("second_bar_t"));
	ck_assert_ptr_null(look_up_in_type_map("second_foo_t"));
	ck_assert_ptr_null(look_up_in_type_map("third_bar_t"));

	free_all_maps();
	free_string_list(called_args);

}
END_TEST

Suite *template_suite(void) {
	Suite *s;
	TCase *tc_core;

	s = suite_create("Template");

	tc_core = tcase_create("Core");

	tcase_add_test(tc_core, test_replace_m4);
	tcase_add_test(tc_core, test_replace_m4_too_few_args);
	tcase_add_test(tc_core, test_replace_m4_list);
	tcase_add_test(tc_core, test_nested_template_declarations);
	suite_add_tcase(s, tc_core);

	return s;
}

int main(void) {

	int number_failed = 0;
	Suite *s;
	SRunner *sr;

	s = template_suite();
	sr = srunner_create(s);
	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0)? 0 : -1;
}