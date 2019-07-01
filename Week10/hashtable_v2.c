/*
 * Author: Pierre-Henri Symoneaux
 * Modifier: Temur Kholmatov
 * The implementation was modified for use in own program
 */

// #include <stdlib.h>
// #include <string.h>

//Hashtable element structure
typedef struct hash_elem_t {
	struct hash_elem_t* next; // Next element in case of a collision
	void* data;	// Pointer to the stored element
	char* key; 	// Key of the stored element
} hash_elem_t;

//Hashtabe structure
typedef struct {
	unsigned int size;	// Hashtable size (in terms of hashed keys)
	unsigned int e_num;	// Number of element currently stored in the hashtable
	hash_elem_t** table;	// The table containaing elements
} hashtable_t;

//Structure used for iterations
typedef struct {
	hashtable_t* ht; 	// The hashtable on which we iterate
	unsigned int index;	// Current index in the table
	hash_elem_t* elem; 	// Curent element in the list
} hash_elem_it;

// Inititalize hashtable iterator on hashtable 'ht'
#define HT_ITERATOR(ht) {ht, 0, ht->table[0]}

char err_ptr;
void* HT_ERROR = &err_ptr; // Data pointing to HT_ERROR are returned in case of error

/* 	Internal funcion to calculate hash for keys.
	It's based on the DJB algorithm from Daniel J. Bernstein.
	The key must be ended by '\0' character.*/
static unsigned int ht_calc_hash(char* key)
{
	unsigned int h = 5381;
  int c;
	while ((c = *key++) != 0)
		h = ((h << 5) + h) + c;
	return h;
}

/* 	Create a hashtable with size 'size'
	and return a pointer to it*/
hashtable_t* ht_create(unsigned int size)
{
	hashtable_t* hasht = malloc(sizeof(hashtable_t));
	if(!hasht)
		return NULL;
	if((hasht->table = malloc(size*sizeof(hash_elem_t*))) == NULL)
	{
		free(hasht->table);
		return NULL;
	}
	hasht->size = size;
	hasht->e_num = 0;
	unsigned int i;
	for(i = 0; i < size; i++)
		hasht->table[i] = NULL;
	return hasht;
}

/* 	Store data in the hashtable. If data with the same key are already stored,
	they are overwritten*/
void ht_put(hashtable_t* hasht, char* key, void* data)
{
	if(data == NULL)
		return;
	unsigned int h = ht_calc_hash(key) % hasht->size;
  // printf("[HT PUT] key, hash: %s, %d\n", key, h);
	hash_elem_t* e = hasht->table[h];

	while(e != NULL)
	{
		if(!strcmp(e->key, key))
		{
      // free(e->data);
			// e->data = strdup(data);
      e->data = data;
      int* t = ((int*) e->data);
      // printf("[HT PUT]: data changed: %d\n", *t);
      return;
		}
		e = e->next;
	}

	// Getting here means the key doesn't already exist

	if((e = malloc(sizeof(hash_elem_t))) == NULL) {
		printf("[HT PUT]: [ERROR]: Could not allocate memory.\n");
		return;
	}
	e->key = strdup(key);
	e->data = strdup(data);
  int *a = (int*) data;
  int *b = (int*) e->data;
  // printf("[HT PUT]: %s %s %d %d\n", key, e->key, *a, *b);
	// Add the element at the beginning of the linked list
	e->next = hasht->table[h];
	hasht->table[h] = e;
	hasht->e_num ++;
}

/* Retrieve data from the hashtable */
void* ht_get(hashtable_t* hasht, char* key)
{
	unsigned int h = ht_calc_hash(key) % hasht->size;
  // printf("[HT GET] key, hash: %s, %d\n", key, h);
	hash_elem_t* e = hasht->table[h];
	while(e != NULL)
	{
		if(!strcmp(e->key, key))
			return e->data;
		e = e->next;
	}
	return NULL;
}

/* 	Remove data from the hashtable. Return the data removed from the table
	so that we can free memory if needed */
int ht_remove(hashtable_t* hasht, char* key)
{
	unsigned int h = ht_calc_hash(key) % hasht->size;
	hash_elem_t* e = hasht->table[h];
	hash_elem_t* prev = NULL;
	while(e != NULL)
	{
		if(!strcmp(e->key, key))
		{
			if(prev != NULL)
				prev->next = e->next;
			else
				hasht->table[h] = e->next;
      // free(e->key);
			// free(e->data);
			free(e);
			e = NULL;
			hasht->e_num --;
			return 1;
		}
		prev = e;
		e = e->next;
	}
	return 0;
}

/* List keys. k should have length equals or greater than the number of keys */
void ht_list_keys(hashtable_t* hasht, char** k, size_t len)
{
	if(len < hasht->e_num)
		return;
	int ki = 0; //Index to the current string in **k
	int i = hasht->size;
	while(--i >= 0)
	{
		hash_elem_t* e = hasht->table[i];
		while(e)
		{
			k[ki++] = e->key;
			e = e->next;
		}
	}
}

/* 	List values. v should have length equals or greater
	than the number of stored elements */
void ht_list_values(hashtable_t* hasht, void** v, size_t len)
{
	if(len < hasht->e_num)
		return;
	int vi = 0; //Index to the current string in **v
	int i = hasht->size;
	while(--i >= 0)
	{
		hash_elem_t* e = hasht->table[i];
		while(e)
		{
			v[vi++] = e->data;
			e = e->next;
		}
	}
}

/* Iterate through table's elements. */
hash_elem_t* ht_iterate(hash_elem_it* iterator)
{
	while(iterator->elem == NULL)
	{
		if(iterator->index < iterator->ht->size - 1)
		{
			iterator->index++;
			iterator->elem = iterator->ht->table[iterator->index];
		}
		else
			return NULL;
	}
	hash_elem_t* e = iterator->elem;
	if(e)
		iterator->elem = e->next;
	return e;
}

/* Iterate through keys. */
char* ht_iterate_keys(hash_elem_it* iterator)
{
	hash_elem_t* e = ht_iterate(iterator);
	return (e == NULL ? NULL : e->key);
}

/* Iterate through values. */
void* ht_iterate_values(hash_elem_it* iterator)
{
	hash_elem_t* e = ht_iterate(iterator);
	return (e == NULL ? NULL : e->data);
}

/* 	Removes all elements stored in the hashtable.
	if free_data, all stored datas are also freed.*/
void ht_clear(hashtable_t* hasht)
{
	hash_elem_it it = HT_ITERATOR(hasht);
	char* k = ht_iterate_keys(&it);
	while(k != NULL)
	{
		ht_remove(hasht, k);
		k = ht_iterate_keys(&it);
	}
}

/* 	Destroy the hash table, and free memory.
	Data still stored are freed*/
void ht_destroy(hashtable_t* hasht)
{
	ht_clear(hasht); // Delete and free all.
	free(hasht->table);
	free(hasht);
}
