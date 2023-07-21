#include "ThreadPool.h"
#include <iostream>
#include <unistd.h>
#include <string.h>

template <typename T>
ThreadPool<T>::ThreadPool(int min, int max) {
    do{
        shutdown = false;
        minNum = min;
        maxNum = max;
        busyNum = 0;
        liveNum = min; //因为这里我们将要创建min个初始线程
        exitNum = 0;
        if((m_taskQ = new taskQueue<T>) == nullptr) {
            cout << "malloc taskQ failed..." << endl;    
            break;
        }
        if(pthread_mutex_init(&poolMutex, NULL) != 0 || pthread_cond_init(&notEmpty, NULL) != 0) {
            cout << "init mutex or cond failed..." << endl;
            break;
        }

        pthread_create(&managerID, NULL, manager, this);
        
        if((workerIDs = new pthread_t[max]) == nullptr) {
            cout << "malloc workerIDs failed..." << endl;
            break;
        }
        memset(workerIDs, 0, sizeof(pthread_t) * maxNum);
        for(int i=0; i<min; i++) {
            pthread_create(&workerIDs[i], NULL, worker, this);
        }
        return;
    }while(0);
    
    if(m_taskQ != nullptr) delete m_taskQ;
    if(workerIDs != nullptr) delete [] workerIDs;
}

template <typename T>
ThreadPool<T>::~ThreadPool() {
    for(int i=0; i<maxNum; i++) {
        cout << workerIDs[i] % 10000 << endl;
    }
    shutdown = true;
    cout << "xi gou han shu zhi xing shi cun huo xian cheng shu liang is " << liveNum << endl;
    for(int i=0; i<liveNum; i++) {
        pthread_cond_signal(&notEmpty);
    }
    //到这里manager 所有的worker线程都结束了
    //释放其他资源
    if(m_taskQ != nullptr) delete m_taskQ;
    if(workerIDs != nullptr) delete [] workerIDs;
    pthread_mutex_destroy(&poolMutex);
    pthread_cond_destroy(&notEmpty);
}

template <typename T>
void ThreadPool<T>::threadExit() {
    pthread_t tid = pthread_self();
    for(int i=0; i<maxNum; i++) {
        if(workerIDs[i] == tid) {
            workerIDs[i] = 0;
            break;
        }
    }
    cout << "************************* thread " << tid%10000 << " is exiting! *************************" << endl;
    pthread_exit(NULL);
}

template <typename T>
void ThreadPool<T>::addTask(Task<T> task) {
    if(shutdown) return;
    m_taskQ->addTask(task);
    pthread_cond_signal(&notEmpty);
}

template <typename T>
void* ThreadPool<T>::worker(void* arg) {
    ThreadPool* pool = static_cast<ThreadPool*>(arg);
    while(1) {
        pthread_mutex_lock(&pool->poolMutex);
        while(pool->m_taskQ->getTaskNum() == 0 && !pool->shutdown) {//等待的线程 它面对的结果是：新任务到来执行新任务 线程池关闭kill 空闲线程数量太多kill
            pthread_cond_wait(&pool->notEmpty, &pool->poolMutex);
            if(pool->exitNum > 0) {
                pool->exitNum--;
                if(pool->liveNum > pool->minNum) {
                    pool->liveNum--;
                    pthread_mutex_unlock(&pool->poolMutex);//观察死锁现象
                    pool->threadExit();
                }
            } 
        }
        if(pool->shutdown) {
            pthread_mutex_unlock(&pool->poolMutex);
            pool->threadExit();
        }
        Task<T> now_task = pool->m_taskQ->takeTask();
        
        pool->busyNum++;
        cout << "thread " << pthread_self()%10000 << " start working..." << endl;
        pthread_mutex_unlock(&pool->poolMutex);

        now_task.function(now_task._arg);
        cout << "thread " << pthread_self()%10000 << " end working..." << endl;

        pthread_mutex_lock(&pool->poolMutex);
        pool->busyNum--;
        pthread_mutex_unlock(&pool->poolMutex);
    }
    return nullptr;
}

template <typename T>
void* ThreadPool<T>::manager(void* arg) {
    ThreadPool* pool = static_cast<ThreadPool*>(arg);
    while(!pool->shutdown) {
        sleep(3);
        
        pthread_mutex_lock(&pool->poolMutex);
        int busy = pool->busyNum;
        int live = pool->liveNum;
        int q_size = pool->m_taskQ->getTaskNum();
        pthread_mutex_unlock(&pool->poolMutex);

        if(live > 2 * busy && live > pool->minNum) {
            pthread_mutex_lock(&pool->poolMutex);
            pool->exitNum = NUMBER;
            pthread_mutex_unlock(&pool->poolMutex);//观察死锁现象
            for(int i=0; i<NUMBER; i++){
                pthread_cond_signal(&pool->notEmpty);
            }
        }

        if(live < pool->maxNum && live < q_size) {
            pthread_mutex_lock(&pool->poolMutex);
            int counter = 0;
            for(int i=0; i<pool->maxNum; i++) {
                if(counter >= NUMBER || pool->liveNum >= pool->maxNum) break;
                if(pool->workerIDs[i] != 0) continue;
                counter++;
                pthread_create(&pool->workerIDs[i], NULL, pool->worker, pool);
                cout << "============================== thread " << pool->workerIDs[i]%10000 << " is creating! =============================" << endl;
                pool->liveNum++;
            }
            pthread_mutex_unlock(&pool->poolMutex);
        }

    }
    return nullptr;
}