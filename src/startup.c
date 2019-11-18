#include <fts.h>
#include <stdio.h>

#include "startup.h"
#include "maps.h"
#include "tree.h"

void load_access_vectors_normal(char *av_path)
{

	char **paths = calloc(2, sizeof(char *));

	paths[0] = av_path;

	FTS *ftsp = fts_open(paths, FTS_PHYSICAL, NULL);

	FTSENT *file = fts_read(ftsp);

	while (file) {

		if (file->fts_level != 0 && file->fts_info == FTS_D
		    && 0 != strcmp(file->fts_name, "perms")) {
			// Directory being visited the first time

			insert_into_decl_map(file->fts_name, "class",
			                     DECL_CLASS);
		} else if (file->fts_info == FTS_F
		           && 0 != strcmp(file->fts_name, "index")) {
			// File

			insert_into_decl_map(file->fts_name, "perm", DECL_PERM);
		}
		file = fts_read(ftsp);
	}
	fts_close(ftsp);
	free(paths);
}

void load_access_vectors_source()
{

}

void load_modules_normal()
{

}

static int is_space(char c)
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static char *strip_space(char *str)
{

	while (is_space(*str)) {
		str++;
	}

	char *end = str;

	while (!is_space(*end)) {
		end++;
	}

	*end = '\0';

	return str;
}

enum selint_error load_modules_source(char *modules_conf_path)
{
	FILE *fd = fopen(modules_conf_path, "r");

	if (!fd) {
		return SELINT_IO_ERROR;
	}

	char *line = NULL;
	ssize_t len_read = 0;
	size_t buf_len = 0;
	while ((len_read = getline(&line, &buf_len, fd)) != -1) {
		if (len_read <= 1 || line[0] == '#') {
			continue;
		}
		char *pos = line;
		while (*pos != '\0' && is_space(*pos)) {
			pos++;
		}
		if (pos[0] == '#' || pos[0] == '\0') {
			continue;
		}
		pos = strtok(line, "=");
		if (!pos) {
			free(line);
			return SELINT_PARSE_ERROR;
		}
		char *mod_name = strip_space(pos);
		pos = strtok(NULL, "=");
		if (!pos) {
			free(line);
			return SELINT_PARSE_ERROR;
		}
		char *status = strip_space(pos);
		insert_into_mods_map(mod_name, status);
		if (strtok(NULL, "=")) {
			free(line);
			return SELINT_PARSE_ERROR;
		}
	}
	free(line);

	return SELINT_SUCCESS;
}

static int mark_transform_interfaces_one_file(struct policy_node *ast) {
	int marked_transform = 0;
	struct policy_node *cur = ast;
	while (cur) {
		if (cur->flavor == NODE_IF_DEF &&
		    cur->first_child &&
		    !is_transform_if(cur->data.str)) {
			struct policy_node *child = cur->first_child;
			while (child &&
			       (child->flavor == NODE_START_BLOCK ||
			        child->flavor == NODE_REQUIRE ||
			        child->flavor == NODE_GEN_REQ)) {
				child = child->next;
			}
			if (!child) {
				// Nothing in interface besides possibly require
				cur = dfs_next(cur);
				continue;
			}
			if (child->flavor == NODE_IF_CALL) {
				if (is_transform_if(child->data.ic_data->name)) {
					mark_transform_if(cur->data.str);
					marked_transform = 1;
				}
			}
		}
		cur = dfs_next(cur);
	}
	return marked_transform;
}

enum selint_error mark_transform_interfaces(struct policy_file_list *files)
{
	struct policy_file_node *cur;
	int marked_transform;
	do {
		marked_transform = 0;
		cur = files->head;
		while (cur) {
			marked_transform = marked_transform ||
			                   mark_transform_interfaces_one_file(cur->file->ast);
			cur = cur->next;
		}
	} while (marked_transform);

	return SELINT_SUCCESS;
}
