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
	parent_inumber = lookup(parent_name, locks, WRITE);


	inode_get(parent_inumber, &pType, &pdata);

	if (parent_inumber == FAIL) {
		printf("failed to create %s, invalid parent dir %s\n", name, parent_name);
		list_unlock_all(locks);
		list_free(locks);
		return FAIL;
	}


	if(pType != T_DIRECTORY) {
		printf("failed to create %s, parent %s is not a dir\n", name, parent_name);
		list_unlock_all(locks);
		list_free(locks);
		return FAIL;
	}

	if (lookup_sub_node(child_name, pdata.dirEntries) != FAIL) {
		printf("failed to create %s, already exists in dir %s\n",
		       child_name, parent_name);
		list_unlock_all(locks);
		list_free(locks);
		return FAIL;
	}

	/* create node and add entry to folder that contains new node */
	child_inumber = generate_new_inumber();

	list_add_lock(locks, get_inode_lock(child_inumber));
	list_write_lock(locks);

	inode_create(nodeType, child_inumber);

	if (child_inumber == FAIL) {
		printf("failed to create %s in  %s, couldn't allocate inode\n", child_name, parent_name);
		list_unlock_all(locks);
		list_free(locks);
		return FAIL;
	}

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


	parent_inumber = lookup(parent_name, locks, WRITE);

	if (parent_inumber == FAIL) {
		printf("failed to delete %s, invalid parent dir %s\n", child_name, parent_name);
		list_unlock_all(locks);
		list_free(locks);
		return FAIL;
	}

	inode_get(parent_inumber, &pType, &pdata);

	if(pType != T_DIRECTORY) {
		printf("failed to delete %s, parent %s is not a dir\n", child_name, parent_name);
		list_unlock_all(locks);
		list_free(locks);
		return FAIL;
	}

	child_inumber = lookup_sub_node(child_name, pdata.dirEntries);
	list_add_lock(locks, get_inode_lock(child_inumber));
	list_write_lock(locks);

	if (child_inumber == FAIL) {
		printf("could not delete %s, does not exist in dir %s\n", name, parent_name);
		list_unlock_all(locks);
		list_free(locks);
		return FAIL;
	}

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
	pthread_rwlock_t * current_lock;
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
	while (1) {
		current_lock = get_inode_lock(current_inumber);

		rwlock_read_lock(current_lock);
		inode_get(current_inumber, &nType, &data);
		current_inumber = lookup_sub_node(path, data.dirEntries);
		rwlock_unlock(current_lock);

		if(current_inumber == FAIL)
			break;
			
		list_add_lock(locks, get_inode_lock(current_inumber));
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
