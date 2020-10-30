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
 * Unlock every lock, free list and return FAIL
 */
int exit_and_unlock(Locks * locks){
	list_unlock_all(locks);
	list_free(locks);
	return FAIL;
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
	int parent_inumber = lookup_node(parent_name, locks, WRITE);

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

	if((parent_inumber = lookup_node(parent_name, locks, WRITE)) == FAIL){
		printf("failed to create %s, invalid parent dir %s\n", name, parent_name);
		return exit_and_unlock(locks);
	}

	inode_get(parent_inumber, &pType, &pdata);

	if(pType != T_DIRECTORY){
		printf("failed to create %s, parent %s is not a dir\n", name, parent_name);
		return exit_and_unlock(locks);
	}
	
	if (lookup_sub_node(child_name, pdata.dirEntries) != FAIL){
		printf("failed to create %s, already exists in dir %s\n", child_name, parent_name);
		return exit_and_unlock(locks);
	}

	if ((child_inumber = generate_new_inumber()) == FAIL){
		printf("failed to create %s in  %s, couldn't allocate inode\n", child_name, parent_name);
		return exit_and_unlock(locks);
	}

	list_add_lock(locks, get_inode_lock(child_inumber));
	list_write_lock(locks);
	inode_create(nodeType, child_inumber);

	if (dir_add_entry(parent_inumber, child_inumber, child_name) == FAIL){
		printf("could not add entry %s in dir %s\n", child_name, parent_name);
		return exit_and_unlock(locks);
	}
	exit_and_unlock(locks);
	return SUCCESS;
}

/*
 * Move a source path to a destination path
 * Input:
 *  - src_name: path of source node
 *  - destn_name: path of destination node
 * Returns: SUCCESS or FAIL
 */
int move(char * src_name, char * destn_name){
	Locks * locks = list_create(INODE_TABLE_SIZE);
	
	/* variables to store source path information */
	char *src_parent_name, *src_child_name, src_name_copy[MAX_FILE_NAME];
	int src_parent_inumber, src_child_inumber;
	type src_parent_type;
	union Data src_parent_data;

	strcpy(src_name_copy, src_name);
	split_parent_child_from_path(src_name_copy, &src_parent_name, &src_child_name);

	printf("src_parent_name = %s\n", src_parent_name);

	if ((src_parent_inumber = lookup_node(src_parent_name, locks, WRITE)) == FAIL) {
		printf("failed to move from %s, invalid source parent dir %s\n", src_name, src_parent_name);
		return exit_and_unlock(locks);
	}

	inode_get(src_parent_inumber, &src_parent_type, &src_parent_data);

	if(src_parent_type != T_DIRECTORY) {
		printf("failed to move from %s, source parent %s is not a dir\n", src_name, src_parent_name);
		return exit_and_unlock(locks);
	}

	if((src_child_inumber = lookup_sub_node(src_child_name, src_parent_data.dirEntries)) == FAIL){
		printf("could not move from %s, does not exist in dir %s\n", src_name, src_parent_name);
		return exit_and_unlock(locks);
	}

	list_add_lock(locks, get_inode_lock(src_child_inumber));
	list_write_lock(locks);

	/* variables to store destination path information */
	char *destn_parent_name, *destn_child_name, destn_name_copy[MAX_FILE_NAME];
	int destn_parent_inumber, destn_child_inumber;
	type destn_parent_type;
	union Data destn_parent_data;

	strcpy(destn_name_copy, destn_name);
	split_parent_child_from_path(destn_name_copy, &destn_parent_name, &destn_child_name);


	if ((destn_parent_inumber = lookup_node(destn_parent_name, locks, WRITE)) == FAIL) {
		printf("failed to move to %s, invalid destination parent dir %s\n", destn_name, destn_parent_name);
		return exit_and_unlock(locks);
	}

	inode_get(destn_parent_inumber, &destn_parent_type, &destn_parent_data);

	if(destn_parent_type != T_DIRECTORY) {
		printf("failed to move to %s, destination parent %s is not a dir\n", destn_name, destn_parent_name);
		return exit_and_unlock(locks);
	}

	if((destn_child_inumber = lookup_sub_node(destn_child_name, destn_parent_data.dirEntries)) != FAIL){
		printf("could not move to %s, already exists in dir %s\n", destn_name, src_parent_name);
		return exit_and_unlock(locks);
	}


	if (dir_add_entry(destn_parent_inumber, src_child_inumber, destn_child_name) == FAIL){
		printf("failed to move %s to %s. Could not add entry %s in dir %s\n", src_name, destn_name, destn_child_name, destn_parent_name);
		return exit_and_unlock(locks);
	}

	if (dir_reset_entry(src_parent_inumber, src_child_inumber) == FAIL) {
		printf("failed to move %s to %s. Failed to delete %s from dir %s\n", src_name, destn_name, src_child_name, src_parent_name);
		return exit_and_unlock(locks);
	}
	exit_and_unlock(locks);
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

	if ((parent_inumber = lookup_node(parent_name, locks, WRITE)) == FAIL) {
		printf("failed to delete %s, invalid parent dir %s\n", name, parent_name);
		return exit_and_unlock(locks);
	}

	inode_get(parent_inumber, &pType, &pdata);

	if(pType != T_DIRECTORY) {
		printf("failed to delete %s, parent %s is not a dir\n", name, parent_name);
		return exit_and_unlock(locks);
	}

	if((child_inumber = lookup_sub_node(child_name, pdata.dirEntries)) == FAIL){
		printf("could not delete %s, does not exist in dir %s\n", name, parent_name);
		return exit_and_unlock(locks);
	}

	list_add_lock(locks, get_inode_lock(child_inumber));
	list_write_lock(locks);
	inode_get(child_inumber, &cType, &cdata);

	if (cType == T_DIRECTORY && is_dir_empty(cdata.dirEntries) == FAIL) {
		printf("could not delete %s: is a directory and not empty\n", name);
		return exit_and_unlock(locks);
	}

	/* remove entry from folder that contained deleted node */
	if (dir_reset_entry(parent_inumber, child_inumber) == FAIL) {
		printf("failed to delete %s from dir %s\n", child_name, parent_name);
		return exit_and_unlock(locks);
	}

	if (inode_delete(child_inumber) == FAIL) {
		printf("could not delete inode number %d from dir %s\n", child_inumber, parent_name);
		return exit_and_unlock(locks);
	}
	
	exit_and_unlock(locks);
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
int lookup(char *name){
	int current_inumber;
	Locks * locks = list_create(INODE_TABLE_SIZE);
	current_inumber = lookup_node(name, locks, READ);
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
int lookup_node(char *name, Locks * locks, int mode) {
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
	list_add_lock(locks, get_inode_lock(current_inumber));
	list_read_lock(locks);
	inode_get(current_inumber, &nType, &data);

	/* search for all sub nodes */
	while ((current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {
		list_add_lock(locks, get_inode_lock(current_inumber));

		/* next path */
		path = strtok_r(NULL, delim, &saveptr);

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
