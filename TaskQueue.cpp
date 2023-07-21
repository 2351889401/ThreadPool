#include "TaskQueue.h"

template <typename T>
taskQueue<T>::taskQueue() {
    pthread_mutex_init(&m_mutex, NULL);
}

template <typename T>
taskQueue<T>::~taskQueue() {
    pthread_mutex_destroy(&m_mutex);
}

template <typename T>
void taskQueue<T>::addTask(Task<T> task) {
    pthread_mutex_lock(&m_mutex);
    m_queue.push(task);
    pthread_mutex_unlock(&m_mutex);
}

template <typename T>
void taskQueue<T>::addTask(callback function, void* arg) {
    pthread_mutex_lock(&m_mutex);
    m_queue.push(Task<T>(function, arg));
    pthread_mutex_unlock(&m_mutex);
}

template <typename T>
Task<T> taskQueue<T>::takeTask() {
    Task<T> ans;
    pthread_mutex_lock(&m_mutex);
    if(m_queue.size() >= 1) {
        ans = m_queue.front();
        m_queue.pop();
    }
    pthread_mutex_unlock(&m_mutex);
    return ans;
}

template <typename T>
int taskQueue<T>::getTaskNum() {
    pthread_mutex_lock(&m_mutex);
    int ans = m_queue.size();
    pthread_mutex_unlock(&m_mutex);
    return ans;
}