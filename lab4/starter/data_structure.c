#include "data_structure.h"

/* Functions for url Frontiers */

/*initialize url frontiers*/
void initQueue(CircularQueue *q){
    /*set up each entry of the array*/
    for(int i = 0; i < MAX_URLS; i++){
        q->urls[i] = malloc(sizeof(char)*URL_SIZE);
        memset(q->urls[i], 0, URL_SIZE);
    }
    q->front = 0;
    q->rear = -1;
    q->count = 0;
    pthread_mutex_init(&(q->lock), NULL);
    pthread_cond_init(&(q->not_empty), NULL);
    q->waiting_thread = 0;
}

/*add provided url to the array and returns -1 if unsuccessful*/
int enqueue( CircularQueue *q, char* url){
    pthread_mutex_lock(&(q->lock));
    if(q->count == MAX_URLS){ /*if the queue is full*/
        pthread_mutex_unlock(&(q->lock));
        return -1;
    }
    q->rear = ( q->rear + 1 ) % MAX_URLS;
    q->urls[q->rear] = strdup(url); /*store the url in the queue positioned at rear*/
    q->count ++;
    pthread_cond_signal(&(q->not_empty));
    pthread_mutex_unlock(&(q->lock));
    return 0;
}

/*remove url from array and returns a copy of it*/
char* dequeue( CircularQueue *q, int *num_threads ){
    pthread_mutex_lock(&(q->lock));
    q->waiting_thread++;
    while(q->count == 0){
        if(q->waiting_thread == *num_threads){
            /* All threads are wating and no more URLs will be added */
            q->waiting_thread--;
            q->count += *num_threads;     /* for awaken threads to exit the while loop */
            pthread_cond_broadcast(&(q->not_empty));    /* Wake up all waiting threads */
            pthread_mutex_unlock(&q->lock);
            return NULL;
        }
        /* Otherwise, wait for urls to be added */
        pthread_cond_wait(&(q->not_empty), &(q->lock));
    }
    q->waiting_thread--;

    char* url = strdup(q->urls[q->front]); /*get the element at the front of the queue*/
    if(url != NULL){
        free(q->urls[q->front]);
        q->urls[q->front] = NULL;           /*reset url at the entry after copying*/
    }
    q->front = (q->front + 1) % MAX_URLS;
    q->count--;
    pthread_mutex_unlock(&(q->lock));
    return url;
}

void freeQueue(CircularQueue *q){
    pthread_mutex_lock(&(q->lock));
    for(int i = 0; i < q->count; i++){
        int index = (q->front + i) % MAX_URLS;
        free(q->urls[index]);
    }
    pthread_mutex_unlock(&(q->lock));
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
    }
    pthread_mutex_init(&(table->lock), NULL);
}

/*add URL to the hash table*/
void addHashURL(HashTable* table, char* url){
    int index = hash(url);  /*compute the hash index for the URL*/
    pthread_mutex_lock(&(table->lock));

    /*allocate new memory for the new entry*/
    HashEntry* newEntry = (HashEntry*)malloc(sizeof(HashEntry));
    newEntry->url = strdup(url);
    newEntry->next = table->buckets[index]; /*insert the new entry at the head of the linked list*/
    table->buckets[index] = newEntry;

    pthread_mutex_unlock(&(table->lock));
}

/*check if the URL already in the Hash table*/
int checkHashURL(HashTable* table, char* url){
    unsigned int index = hash(url);
    pthread_mutex_lock(&(table->lock));

    /* Traverse the linked list at the computed index*/
    HashEntry* entry = table->buckets[index];
    while(entry != NULL){
        if(strcmp(entry->url, url) ==0){
            pthread_mutex_unlock(&(table->lock));
            return 1; /*URL found*/
        }
        entry = entry->next; /* Move to the next entry */
    }
    pthread_mutex_unlock(&(table->lock));
    return 0; /* URL not found */
}

/*cleanup Hash table(visited url)*/
void freeHashTable(HashTable* table){
    pthread_mutex_lock(&(table->lock));
    for(int i = 0; i < TABLE_SIZE; i ++){
        HashEntry* entry = table->buckets[i];
        while(entry){
            HashEntry* tmp = entry;
            entry = entry->next;
            free(tmp->url);
            free(tmp);
        }
    }
    pthread_mutex_unlock(&(table->lock));
}


/* Functions for url arrays */

/*initialize array ptr sent*/
void initArray(URL_Array *arr){
    arr->count = 0;
    pthread_mutex_init(&(arr->lock), NULL); 
}

/*add a new png url to the array, return -1 is unsuccessful*/
int addPNGURL(URL_Array* arr, char* url){
    pthread_mutex_lock(&(arr->lock));
    if(arr->count == MAX_PNG_URLS){
        pthread_mutex_unlock(&(arr->lock));
        return -1; /*array is full*/
    }
    arr->urls[arr->count] = strdup(url);
    arr->count ++;
    pthread_mutex_unlock(&(arr->lock));
    return 0;
}

/*check how many png files in the array*/
int countPNGURL(URL_Array *arr){
    pthread_mutex_lock(&(arr->lock));
    int PNG_count = arr->count;      
    pthread_mutex_unlock(&(arr->lock));
    return PNG_count;
}

/*clean up the url array*/
void freeArray(URL_Array *arr){
    pthread_mutex_lock(&(arr->lock));
    for(int i = 0; i < arr->count; i ++){
        free(arr->urls[i]);
    }
    pthread_mutex_unlock(&(arr->lock));
}