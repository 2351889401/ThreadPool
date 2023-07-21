#pragma once
#include "TaskQueue.h"
#include "TaskQueue.cpp"

template <typename T>
class ThreadPool{
public:
    ThreadPool(int min, int max);
    ~ThreadPool();
    void addTask(Task<T> task);
    static void* worker(void*);
    static void* manager(void*);
    void threadExit();
private:
    bool shutdown;
    int minNum;
    int maxNum;
    int busyNum;
    int liveNum;
    int exitNum;
    taskQueue<T>* m_taskQ;
    pthread_t managerID;
    pthread_t* workerIDs;
    pthread_mutex_t poolMutex;
    pthread_cond_t notEmpty;
    static const int NUMBER = 2;
};