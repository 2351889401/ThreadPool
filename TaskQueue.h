#pragma once
#include <pthread.h>
#include <queue>

using namespace std;
using callback = void (*)(void*);

template <typename T>
class Task{
public:
    Task() {
        function = nullptr;
        _arg = nullptr;
    }
    Task(callback f, void* arg) {
        function = f;
        _arg = (T*)arg;
    }
    ~Task() {}

    callback function;
    T* _arg;
};

template <typename T>
class taskQueue{
public:
    taskQueue();
    ~taskQueue();
    void addTask(Task<T> task);
    void addTask(callback function, void* arg);
    Task<T> takeTask();
    int getTaskNum();
private:
    pthread_mutex_t m_mutex;
    queue<Task<T> > m_queue;
};