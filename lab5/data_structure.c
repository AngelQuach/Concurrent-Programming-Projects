#include "data_structure.h"

/* Functions for url Frontiers */

/*initialize url frontiers*/
void initQueue(CircularQueue *q){
    /*set up each entry of the array*/
     for(int i = 0; i < MAX_URLS; i++){
        q->urls[i] = malloc(sizeof(char)*URL_SIZE);
        if (q->urls[i] == NULL) {
            fprintf(stderr, "Failed to allocate memory for url %d\n", i);
            for (int j = 0; j < i; j++) {
                free(q->urls[j]);
            }
            exit(1);
        }
        memset(q->urls[i], 0, URL_SIZE);
    }
    q->front = 0;
    q->rear = -1;
    q->count = 0;
    q->waiting_handle = 0;
    q->is_done = 0;
}

/*add provided url to the array and returns -1 if unsuccessful*/
int enqueue( CircularQueue *q, char* url){
    if(q->count == MAX_URLS){ /*if the queue is full*/
        return -1;
    }
    q->rear = ( q->rear + 1 ) % MAX_URLS;
    strncpy(q->urls[q->rear], url, URL_SIZE - 1); /* store the url in the queue positioned at rear */
    q->urls[q->rear][URL_SIZE - 1] = '\0'; /* ensure null termination */
    q->count++;
    return 0;
}

/*remove url from array and returns a copy of it*/
char* dequeue( CircularQueue *q, int *num_handle ){
    q->waiting_handle++;
    if(q->count == 0){
        /* All handles are waiting and no more URLs will be added */
        q->waiting_handle--;
        return NULL;
    }
    q->waiting_handle--;

    char* url = strdup(q->urls[q->front]); /*get the element at the front of the queue*/
    if(url == NULL){
        printf("Failed to copy url to return var\n");
        return NULL;
    }
    free(q->urls[q->front]);
    q->urls[q->front] = NULL;            /*reset URL at the entry after copying*/
    q->front = (q->front + 1) % MAX_URLS;
    q->count--;
    return url;
}

void freeQueue(CircularQueue *q){
    for (int i = 0; i < MAX_URLS; i++) {
        if (q->urls[i] != NULL) {
            free(q->urls[i]);  // Free each individual URL
            q->urls[i] = NULL; // Set pointer to NULL after freeing
        }
    }
}

/* Functions for hash table (visited_url array) */

/*hash function to compute an index for a given URL*/
unsigned int hash(const char* url){
    unsigned int hash = 0;
    while(*url){
        hash = (hash <<5) + *url ++; /*compute hash value by spreading out the influence of each character in the string, reducing collisions*/
    }
    return hash % TABLE_SIZE;
}

/*initializa the hash table*/
void initHashTable(HashTable* table){
    for(int i = 0 ; i < TABLE_SIZE; i ++){
        table->buckets [i] = NULL;
        table->url[i] = NULL;
    }
    table->count = 0;
}

/*add URL to the hash table*/
void addHashURL(HashTable* table, char* url){
    int index = hash(url);  /*compute the hash index for the URL*/

    /*allocate new memory for the new entry*/
    HashEntry* newEntry = (HashEntry*)malloc(sizeof(HashEntry));
    newEntry->url = strdup(url);
    newEntry->next = table->buckets[index]; /*insert the new entry at the head of the linked list*/
    table->buckets[index] = newEntry;
    table->url[table->count] = newEntry->url;
    table->count++;
}

/*check if the URL already in the Hash table*/
int checkHashURL(HashTable* table, char* url){
    unsigned int index = hash(url);

    /* Traverse the linked list at the computed index*/
    HashEntry* entry = table->buckets[index];
    while(entry != NULL){
        if(strcmp(entry->url, url) ==0){
            return 1; /*URL found*/
        }
        entry = entry->next; /* Move to the next entry */
    }
    return 0; /* URL not found */
}

/*cleanup Hash table(visited url)*/
void freeHashTable(HashTable* table){
    for(int i = 0; i < TABLE_SIZE; i ++){
        HashEntry* entry = table->buckets[i];
        while(entry){
            HashEntry* tmp = entry;
            entry = entry->next;
            free(tmp->url);
            free(tmp);
        }
    }
}


/* Functions for url arrays */

/*initialize array ptr sent*/
void initArray(URL_Array *arr, int num_PNG){
    arr->count = 0;
    arr->PNG_needed = num_PNG;
}

/*add a new png url to the array, return -1 is unsuccessful*/
int addPNGURL(URL_Array* arr, char* url){
    if(arr->count == MAX_PNG_URLS){
        return -1; /*array is full*/
    }
    arr->urls[arr->count] = strdup(url);
    arr->count ++;
    return 0;
}

/*check how many png files in the array*/
int countPNGURL(URL_Array *arr){
    int PNG_count = arr->count;      
    return PNG_count;
}

/*clean up the url array*/
void freeArray(URL_Array *arr){
    for(int i = 0; i < arr->count; i ++){
        free(arr->urls[i]);
    }
}