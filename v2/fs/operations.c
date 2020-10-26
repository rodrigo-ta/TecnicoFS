#include "operations.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/* Given a path, fills pointers with strings for the parent path and child
 * file name
 * Input:
 *  - path: the path to split. ATENTION: the function may alter this parameter
 *  - parent: reference to a char*, to store parent path
 *  - child: reference to a char*, to store child file name
 */
void split_parent_child_from_path(char * path, char ** parent, char ** child) {
	int n_slashes = 0, last_slash_location = 0;
	int len = strlen(path);

	// deal with trailing slash ( a/x vs a/x/ )
	if (path[len-1] == '/') {
		path[len-1] = '\0';
	}

	for (int i=0; i < len; ++i) {
		if (path[i] == '/' && path[i+1] != '\0') {
			last_slash_location = i;
			n_slashes++;
		}
	}

	if (n_slashes == 0) { // root directory
		*parent = "";
		*child = path;
		return;
	}
	
	path[last_slash_location] = '\0';
	*parent = path;
	*child = path + last_slash_location + 1;

}


/*
 * Initializes tecnicofs and creates root node.
 */
void init_fs() {
	inode_table_init();
	
	/* create root inode */
	int root = generate_new_inumber();
	inode_create(T_DIRECTORY, root);
	
	if (root != FS_ROOT) {
		printf("failed to create node for tecnicofs root\n");
		exit(EXIT_FAILURE);
	}
}


/*
 * Destroy tecnicofs and inode table
 */
void destroy_fs() {
	inode_table_destroy();
}


/*
 * Checks if content of directory is not empty.
 * Input:
 *  - entries: entries of directory
 * Returns: SUCCESS or FAIL
 */
int is_dir_empty(DirEntry *dirEntries) {
	if (dirEntries == NULL) {
		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
		if (dirEntries[i].inumber != FREE_INODE) {
			return FAIL;
		}
	}
	return SUCCESS;
}


/*
 * Looks for node in directory entry from name.
 * Input:
 *  - name: path of node
 *  - entries: entries of directory
 * Returns:
 *  - inumber: found node's inumber
 *  - FAIL: if not found
 */
int lookup_sub_node(char *name, DirEntry *entries) {
	if (entries == NULL) {
		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (entries[i].inumber != FREE_INODE && strcmp(entries[i].name, name) == 0) {
            return entries[i].inumber;
        }
    }
	return FAIL;
}

int get_parent(char * name, char * parent_name, Locks * locks, char * cmd, type * pType, union Data *pdata){
	int parent_inumber = lookup(parent_name, locks, WRITE);

	if (parent_inumber == FAIL) {
		printf("failed to %s %s, invalid parent dir %s\n", cmd, name, parent_name);
		list_unlock_all(locks);
		list_free(locks);
		return FAIL;
	}

	inode_get(parent_inumber, pType, pdata);

	if(*pType != T_DIRECTORY) {
		printf("failed to %s %s, parent %s is not a dir\n", cmd, name, parent_name);
		list_unlock_all(locks);
		list_free(locks);
		return FAIL;
	}
	return parent_inumber;
}

int get_child(char * name, char * parent_name, char * child_name, Locks * locks, char * cmd, union Data pdata){
	int child_inumber = lookup_sub_node(child_name, pdata.dirEntries);
	if(child_inumber == FAIL){
		printf("could not %s %s, does not exist in dir %s\n", cmd, name, parent_name);
		list_unlock_all(locks);
		list_free(locks);
		return child_inumber;
	}
	list_add_lock(locks, get_inode_lock(child_inumber));
	list_write_lock(locks);

	return child_inumber;
}


/*
 * Creates a new node given a path.
 * Input:
 *  - name: path of node
 *  - nodeType: type of node
 * Returns: SUCCESS or FAIL
 */
int create(char *name, type nodeType){
	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];

	/* use for copy */
	type pType;
	union Data pdata;
	Locks * locks = list_create(INODE_TABLE_SIZE);

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	if((parent_inumber = get_parent(name, parent_name, locks, "create", &pType, &pdata)) == FAIL)
		return FAIL;
	
	if (lookup_sub_node(child_name, pdata.dirEntries) != FAIL) {
		printf("failed to create %s, already exists in dir %s\n", child_name, parent_name);
		list_unlock_all(locks);
		list_free(locks);
		return FAIL;
	}
	/* create node and add entry to folder that contains new node */
	child_inumber = generate_new_inumber();

	if (child_inumber == FAIL) {
		printf("failed to create %s in  %s, couldn't allocate inode\n", child_name, parent_name);
		list_unlock_all(locks);
		list_free(locks);
		return FAIL;
	}

	list_add_lock(locks, get_inode_lock(child_inumber));
	list_write_lock(locks);
	inode_create(nodeType, child_inumber);

	if (dir_add_entry(parent_inumber, child_inumber, child_name) == FAIL) {
		printf("could not add entry %s in dir %s\n", child_name, parent_name);
		list_unlock_all(locks);
		list_free(locks);
		return FAIL;
	}
	
	list_unlock_all(locks);
	list_free(locks);

	return SUCCESS;
}

int move(char * name_src, char * name_destn){
	
	int p_inumber_src, c_inumber_src, p_inumber_destn;
	//int c_inumber_destn;
	char *p_name_src, *c_name_src, *p_name_destn, *c_name_destn;
	char name_src_copy[MAX_FILE_NAME], name_destn_copy[MAX_FILE_NAME];

	type p_src_type, p_destn_type;
	//type c_src_type, c_destn_type;
	union Data p_src_data, p_destn_data;
	//union Data c_src_data, c_destn_data;

	Locks * locks = list_create(INODE_TABLE_SIZE);

	
	strcpy(name_src_copy, name_src);
	split_parent_child_from_path(name_src_copy, &p_name_src, &c_name_src);
	if((p_inumber_src = get_parent(name_src, p_name_src, locks, "move", &p_src_type, &p_src_data)) == FAIL)
		return FAIL;
	if((c_inumber_src = get_child(name_src, p_name_src, c_name_src, locks, "move", p_src_data)) == FAIL)
		return FAIL;

	
	strcpy(name_destn_copy, name_destn);
	split_parent_child_from_path(name_destn_copy, &p_name_destn, &c_name_destn);
	if((p_inumber_destn = get_parent(name_destn, p_name_destn, locks, "move", &p_destn_type, &p_destn_data)) == FAIL)
		return FAIL;
	
	//c_inumber_destn = lookup_sub_node(c_name_destn, p_destn_data.dirEntries);


	return SUCCESS;
}


/*
 * Deletes a node given a path.
 * Input:
 *  - name: path of node
 * Returns: SUCCESS or FAIL
 */
int delete(char *name){

	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType, cType;
	union Data pdata, cdata;

	Locks * locks = list_create(INODE_TABLE_SIZE);

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	if((parent_inumber = get_parent(name, parent_name, locks, "delete", &pType, &pdata)) == FAIL)
		return FAIL;

	if((child_inumber = get_child(name, parent_name, child_name, locks, "delete", pdata)) == FAIL)
		return FAIL;

	inode_get(child_inumber, &cType, &cdata);

	if (cType == T_DIRECTORY && is_dir_empty(cdata.dirEntries) == FAIL) {
		printf("could not delete %s: is a directory and not empty\n", name);
		list_unlock_all(locks);
		list_free(locks);
		return FAIL;
	}

	/* remove entry from folder that contained deleted node */
	if (dir_reset_entry(parent_inumber, child_inumber) == FAIL) {
		printf("failed to delete %s from dir %s\n", child_name, parent_name);
		list_unlock_all(locks);
		list_free(locks);
		return FAIL;
	}

	if (inode_delete(child_inumber) == FAIL) {
		printf("could not delete inode number %d from dir %s\n", child_inumber, parent_name);
		list_unlock_all(locks);
		list_free(locks);
		return FAIL;
	}
	
	list_unlock_all(locks);
	list_free(locks);
	return SUCCESS;
}

/*
 * Lookup for a given path.
 * Input:
 *  - name: path of node
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 * locks every path to read except last one:
 * if mode = WRITE (1): locks to write mode when strtok reaches last path
 * if mode = READ (0): locks to read mode when strtok reaches last path
 * 
 */
int startlookup(char *name){
	int current_inumber;
	Locks * locks = list_create(INODE_TABLE_SIZE);
	current_inumber = lookup(name, locks, READ);
	list_unlock_all(locks);
	list_free(locks);

	return current_inumber;

}

/*
 * Lookup for a given path.
 * Input:
 *  - name: path of node
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 * locks every path to read except last one:
 * if mode = WRITE (1): locks to write mode when strtok reaches last path
 * if mode = READ (0): locks to read mode when strtok reaches last path
 * 
 */
int lookup(char *name, Locks * locks, int mode) {
	char full_path[MAX_FILE_NAME], *saveptr;
	char delim[] = "/";
	strcpy(full_path, name);
	int current_inumber = FS_ROOT;

	/* use for copy */
	type nType;
	union Data data;

	char *path = strtok_r(full_path, delim, &saveptr);

	/* locks rwlock of root inode if accessing file/directory directly in fs root */
	if(path == NULL){
		list_add_lock(locks, get_inode_lock(current_inumber));
		if(mode == WRITE)
			list_write_lock(locks);
		else if(mode == READ)
			list_read_lock(locks);
		return current_inumber;
	}

	/* search for all sub nodes */
	while (current_inumber != FAIL) {
		
		/* next path */
		path = strtok_r(NULL, delim, &saveptr);

		list_add_lock(locks, get_inode_lock(current_inumber));

		/* locks rwlock depending on mode type, if current path is the last one / next path is null */
		if(path == NULL){
			if(mode == WRITE)
				list_write_lock(locks);
			else if(mode == READ)
				list_read_lock(locks);
			break;
		}
		list_read_lock(locks);
		inode_get(current_inumber, &nType, &data);
		current_inumber = lookup_sub_node(path, data.dirEntries);
	}

	return current_inumber;
}


/*
 * Prints tecnicofs tree.
 * Input:
 *  - fp: pointer to output file
 */
void print_tecnicofs_tree(FILE *fp){
	inode_print_tree(fp, FS_ROOT, "");
	fclose(fp);
}
