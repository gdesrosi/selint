#include "te_checks.h"
#include "maps.h"
#include "tree.h"
#include "ordering.h"

struct check_result *check_te_order(const struct check_data *data,
                                    const struct policy_node *node)
{
	if (data->flavor != FILE_TE_FILE) {
		return NULL;
	}

	static struct ordering_metadata *order_data;
	static unsigned int order_node_arr_index;

	switch (node->flavor) {
	case NODE_TE_FILE:
		order_data = prepare_ordering_metadata(node);
		order_node_arr_index = 0;
		if (!order_data) {
			return alloc_internal_error("Failed to initialize ordering for C-001");
		}
		calculate_longest_increasing_subsequence(node, order_data, compare_nodes_refpolicy);
		break;
	case NODE_CLEANUP:
		free_ordering_metadata(order_data);
		order_data = NULL;
		break;
	default:
		if (!order_data) {
			return alloc_internal_error("Ordering data was not generated for C-001");
		}
		for (unsigned int i=order_node_arr_index; i < order_data->order_node_len; i++) {
			if (order_data->nodes[i].node == node) {
				order_node_arr_index = i;
				if (order_data->nodes[i].in_order) {
					return NULL;
				} else {
					char *reason_str = get_ordering_reason(order_data, order_node_arr_index);
					struct check_result *to_ret = make_check_result('C',
					                                                C_ID_TE_ORDER,
					                                                reason_str);
					free(reason_str);
					return to_ret;
				}
			}
		}
		return alloc_internal_error("Could not find ordering info for line");
	}
	return NULL;
}

struct check_result *check_require_block(const struct check_data *data,
                                         const struct policy_node *node)
{
	if (data->flavor != FILE_TE_FILE) {
		return NULL;
	}

	struct policy_node *cur = node->first_child;
	while (cur) {
		if (cur->flavor != NODE_DECL) {
			cur = cur->next;
			continue;
		}
		if (cur->data.d_data->flavor != DECL_CLASS &&
		    cur->data.d_data->flavor != DECL_PERM) {
			return make_check_result('S', S_ID_REQUIRE,
			                         "Require block used in te file (use an interface call instead)");
		}
		cur = cur->next;
	}
	// Require contained only object classes and permissions
	return NULL;
}

struct check_result *check_no_explicit_declaration(const struct check_data *data,
                                                   const struct policy_node *node)
{
	return NULL;
}

struct check_result *check_module_if_call_in_optional(const struct check_data
                                                      *check_data,
                                                      const struct policy_node
                                                      *node)
{

	struct if_call_data *if_data = node->data.ic_data;

	char *if_mod_name = look_up_in_ifs_map(if_data->name);

	if (!if_mod_name) {
		// Not defined as an interface.  Probably a macro
		return NULL;
	}

	if (0 == strcmp(if_mod_name, check_data->mod_name)) {
		// No issue calling interfaces in your own module
		return NULL;
	}

	char *mod_type = look_up_in_mods_map(if_mod_name);

	if (!mod_type || 0 != strcmp(mod_type, "module")) {
		// If mod_type is NULL, we have no info on this module.  We *should* have info
		// on all modules of type module, but in some cases may be missing ones that are
		// off or base.  Off and base pass the check.
		return NULL;
	}

	const struct policy_node *tmp = node;

	while (tmp->parent) {
		tmp = tmp->parent;
		if (tmp->flavor == NODE_OPTIONAL_POLICY) {
			return NULL;
		}
	}

	return make_check_result('W', W_ID_IF_CALL_OPTIONAL,
	                         "Call to interface defined in module should be in optional_policy block");

	return NULL;

}
