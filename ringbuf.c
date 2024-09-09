#include "../include/ringbuf.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

void ringbuffer_init(rbctx_t *context, void *buffer_location, size_t buffer_size)
{
    context->begin = (uint8_t *) buffer_location;
    context->end = (uint8_t *) (buffer_location + buffer_size);
    context->read = context->begin;
    context->write = context->begin;

    pthread_mutex_init(&context->mutex_read, NULL);
    pthread_mutex_init(&context->mutex_write, NULL);
    pthread_cond_init(&context->signal_read, NULL);
    pthread_cond_init(&context->signal_write, NULL);

    /* your solution here */
}

int ringbuffer_write(rbctx_t *context, void *message, size_t message_len)
{
    pthread_mutex_lock(&context->mutex_write);

    size_t available_space = (context->end - context->write + context->read - context->begin - 1) % (context->end - context->begin);
    if (message_len > available_space) {
        pthread_mutex_unlock(&context->mutex_write);
        return RINGBUFFER_FULL;
    }

    for (size_t i = 0; i < message_len; ++i) {
        *context->write = ((uint8_t *)message)[i];
        context->write = (context->write + 1 == context->end) ? context->begin : context->write + 1;
    }

    pthread_cond_signal(&context->signal_read);
    pthread_mutex_unlock(&context->mutex_write);

    return SUCCESS;
    /* your solution here */
}

int ringbuffer_read(rbctx_t *context, void *buffer, size_t *buffer_len)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 1;

    pthread_mutex_lock(&context->mutex_read);

    while (context->read == context->write) {
        if (pthread_cond_timedwait(&context->signal_read, &context->mutex_read, &ts) == ETIMEDOUT) {
            pthread_mutex_unlock(&context->mutex_read);
            return RINGBUFFER_EMPTY;
        }
    }

    size_t data_len = 0;
    uint8_t *temp_read = context->read;

    while (temp_read != context->write && data_len < *buffer_len) {
        data_len++;
        temp_read = (temp_read + 1 == context->end) ? context->begin : temp_read + 1;
    }

    if (data_len > *buffer_len) {
        pthread_mutex_unlock(&context->mutex_read);
        return OUTPUT_BUFFER_TOO_SMALL;
    }

    for (size_t i = 0; i < data_len; ++i) {
        ((uint8_t *)buffer)[i] = *context->read;
        context->read = (context->read + 1 == context->end) ? context->begin : context->read + 1;
    }
    *buffer_len = data_len;

    pthread_cond_signal(&context->signal_write);
    pthread_mutex_unlock(&context->mutex_read);

    return SUCCESS;
    /* your solution here */
}

void ringbuffer_destroy(rbctx_t *context)
{
    pthread_mutex_destroy(&context->mutex_read);
    pthread_mutex_destroy(&context->mutex_write);
    pthread_cond_destroy(&context->signal_read);
    pthread_cond_destroy(&context->signal_write);
    /* your solution here */
}
