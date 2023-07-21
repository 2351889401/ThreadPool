#include "ThreadPool.h"
#include "ThreadPool.cpp"

void func(void* arg) {
    cout << "hello world   " << *(int*)arg << endl;
    sleep(1);
}

int main()
{
    ThreadPool<int>* pool = new ThreadPool<int>(3, 10);
    int* x = new int[100];
    for(int i=0; i<100; i++) {
        x[i] = i + 100;
        pool->addTask(Task<int>(func, &x[i]));
    }
    sleep(30);
    delete pool;
    delete [] x;
    return 0;
}