## 本项目是实现一个简单版本的线程池。重在了解线程池工作的核心思想、熟悉多线程编程。
本项目是在**Linux**平台上实现的基础线程池，主要参考链接：https://blog.csdn.net/weixin_45834799/article/details/121475731  
##
关于并发编程，有一些核心概念：  
（1）线程池  
（2）等待队列  
（3）信号量  
（4）锁机制  
这里仅仅实现简单的线程池。虽然是简单的实现，但是对于没有并发编程经验的我来说，是一次很有意义的经历。  
（多线程编程要考虑的点比较多，感觉自己仍没有非常熟练） 
##
**1.** 为什么使用线程池？  
因为频繁的创建、销毁线程对程序的性能有损耗，线程池的存在有助于解决前面的问题。  
**2.** **Java**中线程池**ThreadPoolExecutor**的执行流程如下图（因为线程池核心思想是基本一致的，所以借用**Java**中的线程池来解释）  
![](https://github.com/2351889401/ThreadPool/blob/main/images/liucheng.png)  
本质上来说还是：工作线程数目不够的时候就创建线程；空闲线程数目较多的时候就销毁线程。  
**3.** 本次项目没有**Java**线程池中实现的这些细节（比如核心、非核心线程的区别；等待队列是否排满的判断、拒绝策略的执行等），只是简单的遵循：工作线程数目不够的时候就创建线程；空闲线程数目较多的时候就销毁线程，这一基本思想。  
##
# 本项目的一些细节：  
**1.** 预编译指令 **#pragma once** 效果为：头文件在编译的过程仅被包含一次，防止重复定义的问题。  
**2.** 本项目的编译指令和运行指令如下：  
```
g++ test.cpp -o test -lpthread
./test
```
**3.** 多线程编程的一个重要因素是线程安全————通过互斥锁（**mutex**）保证：  
比如线程池的等待队列，如果“取任务执行”这一操作不是互斥的，可能会导致同一个任务被多个线程执行，这是我们不想看到的；  
再比如对于线程池状态的统计，如果一些描述状态的变量不是互斥访问的，得到的线程池状态可能是不对的，因此会根据错误的状态做出错误的决策，导致线程池崩溃。  
##
# 项目实现
**1.** 线程池的初始化（类中定义的变量直接在这里介绍了）  
创建等待队列、创建（**pthread_create()**）一个管理者线程（管理者线程用来监督线程池状态，实现工作线程的动态销毁和创建）和多个工作线程（通过循环，取出等待队列的任务执行，直到线程终止）、初始化互斥量（**pthread_mutex_t**）和条件变量（**pthread_cond_t**，因为工作线程可能处于等待状态，直到有任务来将它唤醒）
```
  shutdown = false; //线程池是否关闭
  minNum = min; //最少的工作线程数量
  maxNum = max; //最大的工作线程数量
  busyNum = 0; //处于工作状态的工作线程数量
  liveNum = min; //当前存活的线程数量 因为这里我们开始时将要创建min个初始线程
  exitNum = 0; //本次要销毁的空闲线程数量
  if((m_taskQ = new taskQueue<T>) == nullptr) { //创建等待队列
      cout << "malloc taskQ failed..." << endl;    
      break;
  }
  if(pthread_mutex_init(&poolMutex, NULL) != 0 || pthread_cond_init(&notEmpty, NULL) != 0) { //互斥量和条件变量初始化
      cout << "init mutex or cond failed..." << endl;
      break;
  }

  pthread_create(&managerID, NULL, manager, this); //创建管理者线程
  
  if((workerIDs = new pthread_t[max]) == nullptr) {
      cout << "malloc workerIDs failed..." << endl;
      break;
  }
  memset(workerIDs, 0, sizeof(pthread_t) * maxNum);
  for(int i=0; i<min; i++) { //创建若干工作线程
      pthread_create(&workerIDs[i], NULL, worker, this);
  }
```
**2.** 每当向任务队列添加任务时，通过条件变量 “**notEmpty**” 去唤醒（**pthread_cond_signal()**）等待任务的工作线程
```
  m_taskQ->addTask(task);
  pthread_cond_signal(&notEmpty);
```
**3.** 工作线程的执行流程：一开始竞争互斥锁；之后如果有任务去执行任务；没有任务则进入空闲状态，在循环中等待唤醒。  
每次的唤醒信号，只有一个工作线程可以接收，该工作线程处于下面代码中的“等待（**pthread_cond_wait()**）”状态
```
while(1) {
    pthread_mutex_lock(&pool->poolMutex);
    while(pool->m_taskQ->getTaskNum() == 0 && !pool->shutdown) {//等待的线程 它面对的结果是：新任务到来执行新任务 线程池关闭kill 空闲线程数量太多kill
        pthread_cond_wait(&pool->notEmpty, &pool->poolMutex);
        if(pool->exitNum > 0) {
            pool->exitNum--;
            if(pool->liveNum > pool->minNum) {
                pool->liveNum--;
                pthread_mutex_unlock(&pool->poolMutex);
                pool->threadExit();
            }
        } 
    }
    if(pool->shutdown) {
        pthread_mutex_unlock(&pool->poolMutex);
        pool->threadExit();
    }
    Task<T> now_task = pool->m_taskQ->takeTask();
    
    ...
}
```
处于“等待（**pthread_cond_wait()**）”状态的工作线程被唤醒后，会面临3种不同的结果：  
（1）如果新任务到来，在唤醒后，下一次循环的判断条件：
```
pool->m_taskQ->getTaskNum() == 0
```
此时不再成立，因此跳出循环去等待队列取任务执行。之后再次竞争互斥锁、继续等待。  
（2）如果线程池关闭（**shutdown = true**），导致该线程收到了唤醒信号，此时：
```
!pool->shutdown
```
不再成立，因此跳出循环执行下面的语句（通过“**threadExit()**”函数）终止线程：
```
if(pool->shutdown) {
    pthread_mutex_unlock(&pool->poolMutex);
    pool->threadExit();
}
```
（3）如果是空闲线程过多，导致线程池（或者说“管理者线程”）要销毁一定数量的空闲线程，则：
```
pool->exitNum > 0
```
成立，此时被唤醒的线程会被终止。  
**4.** 管理者线程的执行流程：每隔一段时间查看线程池状态（存活线程数量、处于工作中的线程数量等，需要通过互斥量保证线程安全），当工作线程数目不够的时候就创建线程、空闲线程数目较多的时候就销毁线程。  
```
while(!pool->shutdown) {
    sleep(3); //时间间隔
    
    pthread_mutex_lock(&pool->poolMutex);
    int busy = pool->busyNum;
    int live = pool->liveNum;
    int q_size = pool->m_taskQ->getTaskNum();
    pthread_mutex_unlock(&pool->poolMutex);

    if(live > 2 * busy && live > pool->minNum) { //空闲线程较多 销毁线程 每次销毁NUMBER个 也就是2个
        pthread_mutex_lock(&pool->poolMutex);
        pool->exitNum = NUMBER;
        pthread_mutex_unlock(&pool->poolMutex);
        for(int i=0; i<NUMBER; i++){
            pthread_cond_signal(&pool->notEmpty);
        }
    }

    if(live < pool->maxNum && live < q_size) { //工作线程较少 任务较多 每次增加NUMBER个
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
```
**5.** 线程的终止函数（通过pthread_exit()函数的调用）  
```
pthread_t tid = pthread_self();
for(int i=0; i<maxNum; i++) {
    if(workerIDs[i] == tid) {
        workerIDs[i] = 0;
        break;
    }
}
cout << "************************* thread " << tid%10000 << " is exiting! *************************" << endl;
pthread_exit(NULL);
```
**6.** 线程池的析构函数  
正常情况下，当析构函数执行的时候，意味着“线程池完成了全部的任务，可以进入销毁（线程池）状态了”。  
本项目中，如果主线程提供足够的延时，那么“管理者线程”（代码在“**4.**”中）会销毁空闲线程直至最少数量，如下的代码段所示：
```
if(live > 2 * busy && live > pool->minNum) { //空闲线程较多 销毁线程 每次销毁NUMBER个 也就是2个
    pthread_mutex_lock(&pool->poolMutex);
    pool->exitNum = NUMBER;
    pthread_mutex_unlock(&pool->poolMutex);
    for(int i=0; i<NUMBER; i++){
        pthread_cond_signal(&pool->notEmpty);
    }
}
```
也可能存在的情况是：主线程没有提供足够的延时，“管理者线程”销毁的线程数量不多，没有到规定的最少工作线程数量。  
无论是哪种，都可以通过“存活的线程数量”来销毁剩余线程，如下的代码段所示：
```
for(int i=0; i<liveNum; i++) {
    pthread_cond_signal(&notEmpty);
}
```
**7.** 测试程序（**test.cpp**）和测试结果
```
#include "ThreadPool.h"
#include "ThreadPool.cpp"

void func(void* arg) {
    cout << "hello world   " << *(int*)arg << endl;
    sleep(1); //让每个工作线程执行任务慢些 这样“管理者”线程有更大的概率去创建新的工作线程
}

int main()
{
    ThreadPool<int>* pool = new ThreadPool<int>(3, 10);
    int* x = new int[100];
    for(int i=0; i<100; i++) {
        x[i] = i + 100;
        pool->addTask(Task<int>(func, &x[i]));
    }
    sleep(30); //主线程延时长一些 让管理者线程尽量销毁更多的空闲线程
    delete pool;
    delete [] x;
    return 0;
}
```
可以从下图看到线程的创建过程（不是一起创建的，因为每次最多创建2个）：最开始有3个初始工作线程（这里没有体现），而工作线程最大是10个，所以一共创建了如下的7个工作线程  
![](https://github.com/2351889401/ThreadPool/blob/main/images/thread_create.png)  
而“管理者线程”在所有任务结束后开始销毁线程，如下图：从10个工作线程销毁到最少的3个，一共销毁了7个工作线程  
![](https://github.com/2351889401/ThreadPool/blob/main/images/thread_exit.png)  
最终，“**test**”程序运行到线程池的析构函数：
```
delete pool;
```
将剩余的3个工作线程销毁：  
![](https://github.com/2351889401/ThreadPool/blob/main/images/xigou.png)  
