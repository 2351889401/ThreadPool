## 本项目是实现一个简单版本的线程池。重在了解线程池工作的核心思想、熟悉多线程编程、了解死锁问题。
本项目是在**Linux**平台上实现的基础线程池，主要参考链接：https://blog.csdn.net/weixin_45834799/article/details/121475731  
##
关于并发编程，有一些核心概念：  
（1）线程池  
（2）等待队列  
（3）信号量  
（4）锁机制  
这里仅仅实现简单的线程池。虽然是简单的实现，但是对于没有并发编程经验的我来说，是一次很有意义的经历。  
##
**1.** 为什么使用线程池？  
因为频繁的创建、销毁线程对程序的性能有损耗，线程池的存在有助于解决前面的问题。  
**2.** **Java**中线程池**ThreadPoolExecutor**的执行流程（因为线程池核心思想是基本一致的，所以借用**Java**中的线程池来解释）  
![](https://github.com/2351889401/ThreadPool/blob/main/images/liucheng.png)  
大致的思想如图所示。本质上来说还是：工作线程数目不够的时候就创建线程；空闲线程数目较多的时候就销毁线程。  
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
再比如对于线程池状态的统计，如果涉及到一些描述状态的变量不是互斥访问的，得到的线程池状态可能是不对的，因此会根据错误的状态做出错误的决策，导致线程池崩溃。  
##
# 项目实现
**1.** 线程池的初始化  
创建等待队列、创建（**pthread_create()**）一个管理者线程（管理者线程用来监督线程池状态，实现线程的动态销毁和创建）和多个工作线程（通过循环，取出等待队列的任务执行，直到线程终止）、初始化互斥量（**pthread_mutex_t**）和条件变量（**pthread_cond_t**，因为工作线程可能处于等待状态，直到有任务来将它唤醒）
```
  shutdown = false;
  minNum = min;
  maxNum = max;
  busyNum = 0;
  liveNum = min; //因为这里我们将要创建min个初始线程
  exitNum = 0;
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
