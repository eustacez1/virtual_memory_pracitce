#ifndef QUEUE_H
#define QUEUE_H
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int *items;
    int front;
    int rear;
    int size;
    int capacity;
} Queue;

Queue* create_queue( Queue* q, int capacity) {
    q = (Queue *) malloc(sizeof(Queue));
    q->items = (int *) malloc(sizeof(int) * capacity);
    q->front = 0;
    q->rear = -1;
    q->size = 0;
    q->capacity = capacity;
    return q;
}

void destroy_queue(Queue *q) {
    free(q->items);
    free(q);
}

int is_full(Queue *q) {
    return q->size == q->capacity;
}

int is_empty(Queue *q) {
    return q->size == 0;
}

void enqueue(Queue *q, int item) {
    if (is_full(q)) {
        int *newItems = (int *) malloc(sizeof(int) * q->capacity * 2);
        for (int i = 0; i < q->size; i++) {
            newItems[i] = q->items[(q->front + i) % q->capacity];
        }
        free(q->items);
        q->items = newItems;
        q->front = 0;
        q->rear = q->size - 1;
        q->capacity *= 2;
    }
    q->rear = (q->rear + 1) % q->capacity;
    q->items[q->rear] = item;
    q->size++;
}

int dequeue(Queue *q) {
    if (is_empty(q)) {
        printf("Queue is empty.\n");
        exit(1);
    }
    int item = q->items[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->size--;
    return item;
}
void print_queue(Queue *q) {

    if (is_empty(q)) {
        printf("Queue is empty.\n");
    } else {
        printf("Elements in the circular queue are: ");
        if (q->rear >= q->front) {
            for (int i = q->front; i <= q->rear; i++)
                printf("%d ", q->items[i]);
        } else {
            for (int i = q->front; i < q->size; i++)
                printf("%d ", q->items[i]);
            for (int i = 0; i <= q->rear; i++)
                printf("%d ", q->items[i]);
        }
        printf("\n");
    }
}

#endif