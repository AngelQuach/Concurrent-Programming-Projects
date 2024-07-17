#ifndef DATA_STRUCTURE_H
#define DATA_STRUCTURE_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define MAX_URLS 500
#define TABLE_SIZE 500
#define MAX_PNG_URLS 500
#define URL_SIZE 256

/* Set up the shared arrays */
/*Circular queue for URL Frontiers*/
typedef struct{
    char *urls[MAX_URLS];
    int front;
    int rear;
    int count;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    int waiting_thread;
}CircularQueue;

/*Hash table for URL Visited*/
/*define a structure for each entry in the hash table*/
typedef struct{
    char* url;
    struct HashEntry* next; /*pointer to the next entry in the linked list*/
}HashEntry;

/*define the hash table structure*/
typedef struct{
    HashEntry* buckets[TABLE_SIZE]; /*array of pointers to hash entries*/
    pthread_mutex_t lock;
}HashTable;

/*arrays for PNG URL*/
typedef struct{
    char *urls[MAX_PNG_URLS];
    int count;
    pthread_mutex_t lock;
}URL_Array;

void initQueue(CircularQueue *q);
int enqueue( CircularQueue *q, char* url);
char* dequeue( CircularQueue *q , int *num_threads );
void freeQueue(CircularQueue *q);
unsigned int hash(const char* url);
void initHashTable(HashTable* table);
void addHashURL(HashTable* table, char* url);
int checkHashURL(HashTable* table, char* url);
void freeHashTable(HashTable* table);
void initArray(URL_Array *arr);
int addPNGURL(URL_Array* arr, char* url);
int countPNGURL(URL_Array *arr);
void freeArray(URL_Array *arr);

#endif