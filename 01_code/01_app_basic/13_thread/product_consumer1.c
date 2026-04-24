#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#define BUFFER_SIZE 5
#define PRODUCER_COUNT 2
#define CONSUMER_COUNT 2
#define TOTAL_ITEMS 20

// 信号量实现的缓冲区
typedef struct {
    int buffer[BUFFER_SIZE];
    int in;
    int out;
    int count;           // 当前缓冲区中的项目数
    sem_t empty;     // 空槽位数量
    sem_t full;      // 已使用槽位数量
    pthread_mutex_t mutex;  // 保护缓冲区的互斥锁
} SemaphoreBuffer;

SemaphoreBuffer sem_buffer;

// 初始化
void sem_buffer_init(SemaphoreBuffer *buf) {
    buf->in = 0;
    buf->out = 0;
    buf->count = 0;
    sem_init(&buf->empty, 0, BUFFER_SIZE);  // 初始有BUFFER_SIZE个空位
    sem_init(&buf->full, 0, 0);             // 初始没有已使用的位
    pthread_mutex_init(&buf->mutex, NULL);
}

// 销毁
void sem_buffer_destroy(SemaphoreBuffer *buf) {
    sem_destroy(&buf->empty);
    sem_destroy(&buf->full);
    pthread_mutex_destroy(&buf->mutex);
}

// 生产者使用信号量
void semaphore_produce(int producer_id, int item) {
    sem_wait(&sem_buffer.empty);  // 等待空槽位

    pthread_mutex_lock(&sem_buffer.mutex);

    // 生产
    sem_buffer.buffer[sem_buffer.in] = item;
    sem_buffer.in = (sem_buffer.in + 1) % BUFFER_SIZE;
    sem_buffer.count++;

    printf("[Semaphore] Producer %d: Produced item %d count[%d]\n", producer_id, item, sem_buffer.count);

    pthread_mutex_unlock(&sem_buffer.mutex);

    sem_post(&sem_buffer.full);  // 增加已使用槽位

    usleep((rand() % 300) * 1000);
}

// 消费者使用信号量
int semaphore_consume(int consumer_id) {
    sem_wait(&sem_buffer.full);  // 等待有数据的槽位

    pthread_mutex_lock(&sem_buffer.mutex);

    // 消费
    int item = sem_buffer.buffer[sem_buffer.out];
    sem_buffer.out = (sem_buffer.out + 1) % BUFFER_SIZE;
    sem_buffer.count--;

    printf("[Semaphore] Consumer %d: Consumed item %d count[%d]\n", consumer_id, item, sem_buffer.count);

    pthread_mutex_unlock(&sem_buffer.mutex);

    sem_post(&sem_buffer.empty);  // 增加空槽位

    usleep((rand() % 400) * 1000);

    return item;
}

void* semaphore_producer_func(void* arg) {
    int producer_id = *(int*)arg;
    int items_produced = 0;

    while (items_produced < TOTAL_ITEMS / PRODUCER_COUNT) {
        int item = producer_id * 1000 + items_produced;
        semaphore_produce(producer_id, item);
        items_produced++;
    }

    return NULL;
}

void* semaphore_consumer_func(void* arg) {
    int consumer_id = *(int*)arg;
    int items_consumed = 0;

    while (items_consumed < TOTAL_ITEMS / CONSUMER_COUNT) {
        semaphore_consume(consumer_id);
        items_consumed++;
    }

    return NULL;
}

void run_semaphore_example() {
    printf("\n=== Running Semaphore Implementation ===\n");

    srand(time(NULL));
    sem_buffer_init(&sem_buffer);

    pthread_t producers[PRODUCER_COUNT];
    pthread_t consumers[CONSUMER_COUNT];
    int p_ids[PRODUCER_COUNT];
    int c_ids[CONSUMER_COUNT];

    // 创建生产者
    for (int i = 0; i < PRODUCER_COUNT; i++) {
        p_ids[i] = i + 1;
        pthread_create(&producers[i], NULL, semaphore_producer_func, &p_ids[i]);
    }

    // 创建消费者
    for (int i = 0; i < CONSUMER_COUNT; i++) {
        c_ids[i] = i + 1;
        pthread_create(&consumers[i], NULL, semaphore_consumer_func, &c_ids[i]);
    }

    // 等待完成
    for (int i = 0; i < PRODUCER_COUNT; i++) {
        pthread_join(producers[i], NULL);
    }
    for (int i = 0; i < CONSUMER_COUNT; i++) {
        pthread_join(consumers[i], NULL);
    }

    sem_buffer_destroy(&sem_buffer);
    printf("Semaphore example completed!\n");
}

int main()
{
    run_semaphore_example();
    return 0;
}
