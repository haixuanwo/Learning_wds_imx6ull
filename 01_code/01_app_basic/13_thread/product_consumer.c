#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define BUFFER_SIZE 5
#define PRODUCER_COUNT 3
#define CONSUMER_COUNT 3
#define ITEMS_PER_PRODUCER 10

// 共享缓冲区结构
typedef struct {
    int buffer[BUFFER_SIZE];
    int count;           // 当前缓冲区中的项目数
    int in;              // 生产者放入位置
    int out;             // 消费者取出位置
    pthread_mutex_t mutex;           // 互斥锁
    pthread_cond_t not_empty;        // 缓冲区非空条件
    pthread_cond_t not_full;         // 缓冲区非满条件
    int stop;            // 停止标志
} Buffer;

Buffer shared_buffer;

// 初始化缓冲区
void buffer_init(Buffer *buf) {
    buf->count = 0;
    buf->in = 0;
    buf->out = 0;
    buf->stop = 0;
    pthread_mutex_init(&buf->mutex, NULL);
    pthread_cond_init(&buf->not_empty, NULL);
    pthread_cond_init(&buf->not_full, NULL);
}

// 销毁缓冲区
void buffer_destroy(Buffer *buf) {
    pthread_mutex_destroy(&buf->mutex);
    pthread_cond_destroy(&buf->not_empty);
    pthread_cond_destroy(&buf->not_full);
}

// 生产者：向缓冲区添加数据
void produce(Buffer *buf, int item, int producer_id) {
    pthread_mutex_lock(&buf->mutex);

    // 等待缓冲区非满
    while (buf->count == BUFFER_SIZE && !buf->stop) {
        printf("Producer %d: Buffer full, waiting...\n", producer_id);
        pthread_cond_wait(&buf->not_full, &buf->mutex);
    }

    if (buf->stop) {
        pthread_mutex_unlock(&buf->mutex);
        return;
    }

    // 生产数据
    buf->buffer[buf->in] = item;
    buf->in = (buf->in + 1) % BUFFER_SIZE;
    buf->count++;

    printf("Producer %d: Produced item %d, buffer count: %d\n",
           producer_id, item, buf->count);

    // 通知消费者缓冲区非空
    pthread_cond_signal(&buf->not_empty);
    pthread_mutex_unlock(&buf->mutex);

    // 模拟生产时间
    usleep((rand() % 500) * 1000);
}

// 消费者：从缓冲区取出数据
int consume(Buffer *buf, int consumer_id) {
    pthread_mutex_lock(&buf->mutex);

    // 等待缓冲区非空
    while (buf->count == 0 && !buf->stop) {
        printf("Consumer %d: Buffer empty, waiting...\n", consumer_id);
        pthread_cond_wait(&buf->not_empty, &buf->mutex);
    }

    if (buf->stop && buf->count == 0) {
        pthread_mutex_unlock(&buf->mutex);
        return -1;  // 结束信号
    }

    // 消费数据
    int item = buf->buffer[buf->out];
    buf->out = (buf->out + 1) % BUFFER_SIZE;
    buf->count--;

    printf("Consumer %d: Consumed item %d, buffer count: %d\n",
           consumer_id, item, buf->count);

    // 通知生产者缓冲区非满
    pthread_cond_signal(&buf->not_full);
    pthread_mutex_unlock(&buf->mutex);

    // 模拟消费时间
    usleep((rand() % 800) * 1000);

    return item;
}

// 生产者线程函数
void* producer_func(void* arg) {
    int producer_id = *(int*)arg;

    for (int i = 0; i < ITEMS_PER_PRODUCER; i++) {
        int item = producer_id * 100 + i;  // 生成唯一的产品ID
        produce(&shared_buffer, item, producer_id);
    }

    printf("Producer %d: Finished producing %d items\n",
           producer_id, ITEMS_PER_PRODUCER);

    return NULL;
}

// 消费者线程函数
void* consumer_func(void* arg) {
    int consumer_id = *(int*)arg;
    int consumed_count = 0;

    while (1) {
        int item = consume(&shared_buffer, consumer_id);
        if (item == -1) {
            break;  // 收到结束信号
        }
        consumed_count++;
    }

    printf("Consumer %d: Finished consuming %d items\n",
           consumer_id, consumed_count);

    return NULL;
}

int main() {
    srand(time(NULL));

    // 初始化缓冲区
    buffer_init(&shared_buffer);

    // 创建生产者线程
    pthread_t producers[PRODUCER_COUNT];
    int producer_ids[PRODUCER_COUNT];

    for (int i = 0; i < PRODUCER_COUNT; i++) {
        producer_ids[i] = i + 1;
        pthread_create(&producers[i], NULL, producer_func, &producer_ids[i]);
    }

    // 创建消费者线程
    pthread_t consumers[CONSUMER_COUNT];
    int consumer_ids[CONSUMER_COUNT];

    for (int i = 0; i < CONSUMER_COUNT; i++) {
        consumer_ids[i] = i + 1;
        pthread_create(&consumers[i], NULL, consumer_func, &consumer_ids[i]);
    }

    // 等待所有生产者完成
    for (int i = 0; i < PRODUCER_COUNT; i++) {
        pthread_join(producers[i], NULL);
    }

    // 通知消费者可以退出了
    printf("\nAll producers finished. Notifying consumers to exit...\n");

    pthread_mutex_lock(&shared_buffer.mutex);
    shared_buffer.stop = 1;  // 设置停止标志
    pthread_cond_broadcast(&shared_buffer.not_empty);  // 唤醒所有等待的消费者
    pthread_cond_broadcast(&shared_buffer.not_full);   // 唤醒所有等待的生产者
    pthread_mutex_unlock(&shared_buffer.mutex);

    // 等待所有消费者完成
    for (int i = 0; i < CONSUMER_COUNT; i++) {
        pthread_join(consumers[i], NULL);
    }

    // 清理资源
    buffer_destroy(&shared_buffer);

    printf("\nProducer-Consumer simulation completed successfully!\n");

    return 0;
}
