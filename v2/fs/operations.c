#include "operations.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
 * Check if first string is a subset of second string
 * Input:
 *  - str1: first string
 *  - str2: second string
 * Returns:
 *  - true: if first string is a subset of second string
 *  - false: if first string is not a subset of second string
 */
bool check_if_subset(char * str1, char * str2){
	bool is_subset = true;
	for(int i = 0; i < strlen(str1); i++)
		for(int j = 0; j < strlen(str2); j++)
			if(str1[i] != str2[i])
				is_subset = false;
	return is_subset;
}

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
	};

	exit_and_unlock(locks);
	return SUCCESS;
}

/*
 * Verify if source path is valid
 * Locks accessed i-nodes.
 * Store source parent inumber and source child inumber in dest_inumbers (int*)
 * Input:
 *  - locks: Locks pointer
 *  - src_name: path of source node
 *  - src_parent_name: parent name of source node
 *  - src_child_name: child name of source node
 *  - src_inumber: int array of source parent inumber and source child inumber
 * Returns: SUCCESS or FAIL
 */
int verify_source(Locks * locks, char * src_name, char * src_parent_name, char * src_child_name, int * src_inumbers){
	int src_parent_inumber, src_child_inumber;
	type src_parent_type;
	union Data src_parent_data;

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

	src_inumbers[0] = src_parent_inumber;
	src_inumbers[1] = src_child_inumber;

	return SUCCESS;
}

/*
 * Verify if destination path is valid
 * Locks accessed i-nodes.
 * Store destination parent inumber and destination child inumber in dest_inumbers (int*)
 * Input:
 *  - locks: Locks pointer
 *  - dest_name: path of destination node
 *  - dest_parent_name: parent name of destination node
 *  - dest_child_name: child name of destination node
 *  - src_parent_name: parent name of source node
 *  - src_parent_inumber: parent inumber of source node
 *  - dest_inumber: int array of destination parent inumber and destination child inumber
 * Returns: SUCCESS or FAIL
 */
int verify_destination(Locks * locks, char * dest_name, char * dest_parent_name, char * dest_child_name, char* src_parent_name, int src_parent_inumber, int * dest_inumbers){
	int dest_parent_inumber, dest_child_inumber;
	type dest_parent_type;
	union Data dest_parent_data;

	if((dest_parent_inumber = lookup_node(dest_parent_name, locks, DONOTHING)) == FAIL) {
		printf("failed to move to %s, invalid destination parent dir %s\n", dest_name, dest_parent_name);
		return exit_and_unlock(locks);
	}

	if(src_parent_inumber != dest_parent_inumber){
		for(int i = 0; i < locks->num; i++)

		/* checks if destination parent name is part of source parent name. If so, unlocks read-locked */
		/* i-node in source path which is the same i-node of destination parent i-node to be write-locked */
		if(check_if_subset(dest_parent_name, src_parent_name) == true){
			pthread_rwlock_t * dest_parent_inode_lock = get_inode_lock(dest_parent_inumber);
			rwlock_unlock(dest_parent_inode_lock);
		}

		/* try to write-lock dest parent i-node until it succeeds */
		if(list_try_write_lock(locks) != 0){
			pthread_rwlock_t * src_parent_inode_lock = get_inode_lock(src_parent_inumber);
			rwlock_unlock(src_parent_inode_lock);
			int acquired = 0, num_trys = 1;

			printf("%p\n", locks->rwlocks[locks->num - 1]);

			/* interlayer locking and unlocking inode of source parent until it can lock inode of destination parent (prevents deadlocks) */
			while(!acquired){
				rwlock_write_lock(src_parent_inode_lock);
				if(list_try_write_lock(locks) == 0)
					acquired = 1;
				else{
					rwlock_unlock(src_parent_inode_lock);
					usleep((rand()%(++num_trys * MAXSLEEPTIME)) * 1000); // sleep for miliseconds (increases with number of tries)
				}
			}
		}
	}

	inode_get(dest_parent_inumber, &dest_parent_type, &dest_parent_data);

	if(dest_parent_type != T_DIRECTORY) {
		printf("failed to move to %s, destination parent %s is not a dir\n", dest_name, dest_parent_name);
		return exit_and_unlock(locks);
	}


	if((dest_child_inumber = lookup_sub_node(dest_child_name, dest_parent_data.dirEntries)) != FAIL){
		printf("could not move to %s, already exists in dir %s\n", dest_name, src_parent_name);
		return exit_and_unlock(locks);
	}

	dest_inumbers[0] = dest_parent_inumber;
	dest_inumbers[1] = dest_child_inumber;

	return SUCCESS;
}

/*
 * Move a source path to a destination path
 * Input:
 *  - src_name: path of source node
 *  - dest_name: path of destination node
 * Returns: SUCCESS or FAIL
 */
int move(char * src_name, char * dest_name){
	Locks * locks = list_create(INODE_TABLE_SIZE);
	char *src_parent_name, *src_child_name, src_name_copy[MAX_FILE_NAME], *dest_parent_name, *dest_child_name, dest_name_copy[MAX_FILE_NAME];
	int src_inumbers[MAXINUMBERS], dest_inumbers[MAXINUMBERS], src_parent_inumber, src_child_inumber, dest_parent_inumber;

	/* split source path */
	strcpy(src_name_copy, src_name);
	split_parent_child_from_path(src_name_copy, &src_parent_name, &src_child_name);

	/* split destination path */
	strcpy(dest_name_copy, dest_name);
	split_parent_child_from_path(dest_name_copy, &dest_parent_name, &dest_child_name);

	/* check if both paths are the same */
	if(strcmp(src_name, dest_name) == 0){
		printf("failed to move %s to %s. Source and destination paths are the same.\n", src_name, dest_name);
		return FAIL;
	}

	/* check if destination path is a subpath of source path */
	if(check_if_subset(src_name, dest_name) == true){
		printf("failed to move %s to %s. Cannot move to a subdirectory of itself.\n", src_name, dest_name);
		return FAIL;
	}

	if(verify_source(locks, src_name, src_parent_name, src_child_name, src_inumbers) == FAIL)
		return FAIL;

	src_parent_inumber = src_inumbers[0];
	src_child_inumber = src_inumbers[1];

	/* add source child i-node lock to the list and write-locks it */
	list_add_lock(locks, get_inode_lock(src_child_inumber));
	list_write_lock(locks);

	if(verify_destination(locks, dest_name, dest_parent_name, dest_child_name, src_parent_name, src_parent_inumber, dest_inumbers) == FAIL)
		return FAIL;

	dest_parent_inumber = dest_inumbers[0];

	if (dir_add_entry(dest_parent_inumber, src_child_inumber, dest_child_name) == FAIL){
		printf("failed to move %s to %s. Could not add entry %s in dir %s\n", src_name, dest_name, dest_child_name, dest_parent_name);
		return exit_and_unlock(locks);
	}

	if (dir_reset_entry(src_parent_inumber, src_child_inumber) == FAIL) {
		printf("failed to move %s to %s. Failed to delete %s from dir %s\n", src_name, dest_name, src_child_name, src_parent_name);
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

	/* add child i-node lock to the list and write-locks it */
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
 * if mode = DONOTHING (0): DOES NOT lock inode of last path but adds it to list of locks
 * if mode = WRITE (1): locks to write mode when strtok reaches last path
 * if mode = READ (2): locks to read mode when strtok reaches last path
 * 
 */
int lookup_node(char *name, Locks * locks, int mode) {
	char *saveptr, *path;
	char delim[] = "/";
	int current_inumber = FS_ROOT;
	char *full_path = (char*) malloc(sizeof(char) * MAX_FILE_NAME);
	strcpy(full_path, name);

	/* use for copy */
	type nType;
	union Data data;

	/* search for all sub nodes */
	do{
		list_add_lock(locks, get_inode_lock(current_inumber));

		/* if next path is null which means the current path is the last one */
		if((path = strtok_r(full_path, delim, &saveptr)) == NULL){
			if(mode == WRITE)
				list_write_lock(locks);
			else if(mode == READ)
				list_read_lock(locks);
			return current_inumber;
		}
		list_read_lock(locks);
		inode_get(current_inumber, &nType, &data);
		full_path = NULL;

	} while((current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL);
	
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
