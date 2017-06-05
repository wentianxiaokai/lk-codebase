# lk-codebase
collect some opensource codebase
1,event_fd_test.c:
// 主线程 write，通知子线程；
// 然后子线程 read，表示得到通知,然后 write，通知主线程
// 最后，主线程 read，表示得到子线程的通知。
// 所以总共是 2 次事件通知的耗时。
// 在 32核 2.4Ghz，120G内存的配置环境下，一次事件通知的耗时的平均值是  3.8 微秒。
// event_fd 的性能要比 pipe 和 pthread_cond_wait 的性能要好。
// gcc -o event_fd_test.out event_fd_test.c get_clock.c -lpthread 

