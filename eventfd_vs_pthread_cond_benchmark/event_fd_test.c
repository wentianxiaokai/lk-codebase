#include <stdio.h>  
#include <unistd.h>  
#include <sys/time.h>  
#include <stdint.h>  
#include <pthread.h>  
#include <sys/eventfd.h>  
#include <sys/epoll.h>  
#include "get_clock.h"

// 主线程 write，通知子线程；
// 然后子线程 read，表示得到通知,然后 write，通知主线程
// 最后，主线程 read，表示得到子线程的通知。
// 所以总共是 2 次事件通知的耗时。
// 在 32核 2.4Ghz，120G内存的配置环境下，一次事件通知的耗时的平均值是  3.8 微秒。
// event_fd 的性能要比 pipe 和 pthread_cond_wait 的性能要好。
// gcc -o event_fd_test.out event_fd_test.c get_clock.c -lpthread 


int efd = -1;  
int read_flag = 0;
int ep_fd = -1; 
struct epoll_event read_event;  
struct epoll_event events[10];  
#define TEST_TIMES 1000


void *read_thread(void *dummy)  
{  
    int ret = 0;  
    uint64_t count = 0;
  
    while (1)  
    {
        if( 1 == read_flag )    continue;
        ret = epoll_wait(ep_fd, &events[0], 10, 5000);  
        if (ret > 0)  
        {  
            int i = 0;  
            for (; i < ret; i++)  
            {  
                if (events[i].events & EPOLLHUP)  
                {  
                    printf("epoll eventfd has epoll hup.\n");  
                    goto fail;  
                }  
                else if (events[i].events & EPOLLERR)  
                {  
                    printf("epoll eventfd has epoll error.\n");  
                    goto fail;  
                }  
                else if (events[i].events & EPOLLIN)  
                {  
                    int event_fd = events[i].data.fd;  
                    ret = read(event_fd, &count, sizeof(count));  
                    if (ret < 0)  
                    {  
                        perror("read fail:");  
                        goto fail;  
                    }  
                    else  
                    {  
                        count = 4;  
                        ret = write(event_fd, &count, sizeof(count));  
                        read_flag = 1;
                    }  
                }  
            }  
        }  
        else if (ret == 0)  
        {  
            /* time out */  
            printf("epoll wait timed out.\n");  
            break;  
        }  
        else  
        {  
            perror("epoll wait error:");  
            goto fail;  
        }  
    }  
  
    fail:  
    if (ep_fd >= 0)  
    {  
        close(ep_fd);  
        ep_fd = -1;  
    }  
  
    return NULL;  
}  
  

int cmp(const void * a, const void * b)
{
     return((*(double*)a-*(double*)b>0)?1:-1);
}

int main(int argc, char *argv[])  
{  
    pthread_t pid = 0;  
    uint64_t count = 0;  
    int ret = 0;  
    int i = 0;  


    efd = eventfd(0, 0);  
    if (efd < 0)  
    {  
        perror("eventfd failed.");  
        goto fail;  
    }  
  

    if (efd < 0)  
    {  
        printf("efd not inited.\n");  
        goto fail;  
    }  

    ep_fd = epoll_create(1024);  
    if (ep_fd < 0)  
    {  
        perror("epoll_create fail: ");  
        goto fail;  
    }  
  

    read_event.events = EPOLLHUP | EPOLLERR | EPOLLIN;  
    read_event.data.fd = efd;  

    ret = epoll_ctl(ep_fd, EPOLL_CTL_ADD, efd, &read_event);  
    if (ret < 0)  
    {  
        perror("epoll ctl failed:");  
        goto fail;  
    }  
    
    ret = pthread_create(&pid, NULL, read_thread, NULL);  
    if (ret < 0)  
    {  
        perror("pthread create:");  
        goto fail;  
    }  
    
    int no_cpu_freq_fail = 0;
    double mhz;
    mhz = get_cpu_mhz(no_cpu_freq_fail);
    
    double round_time[TEST_TIMES];
    // double once_time[TEST_TIMES];
    int j = 0;  
    cycles_t dwStart,dwEnd; 
    int success_count = 0;
    for (i = 0; i < TEST_TIMES; i++)  
    {  
        count = 4;  
        dwStart = get_cycles();
        // printf("success write to efd, write %d :bytes(%llu) at %lds %ldus\n",i, count, tv_start.tv_sec, tv_start.tv_usec);  
        ret = write(efd, &count, sizeof(count));  
        if (ret < 0)  
        {  
            perror("write event fd fail:");  
            goto fail;  
        }  
        else  
        {    
            while ( 0 == read_flag)  ;

            {
                ret = epoll_wait(ep_fd, &events[0], 10, 5000);
                // printf("ret:%d\n", ret);
                if (ret > 0)  
                {
                    if( ret != 1 )
                    {
                        printf("ret:%d,epoll_wait error?\n",ret );
                    }

                    for (j=0; j < ret; j++)
                    {
                        if (events[j].events & EPOLLHUP)  
                        {  
                            printf("epoll eventfd has epoll hup.\n");  
                            goto fail;  
                        }  
                        else if (events[j].events & EPOLLERR)  
                        {  
                            printf("epoll eventfd has epoll error.\n");  
                            goto fail;  
                        }  
                        else if (events[j].events & EPOLLIN)  
                        {  
                            int event_fd = events[j].data.fd;  
                            ret = read(event_fd, &count, sizeof(count));  
                            if (ret < 0)  
                            {  
                                perror("read fail:");  
                                goto fail;  
                            }  
                            else  
                            {  
                                read_flag = 0;
                                success_count ++;
                                dwEnd = get_cycles();
                                // printf("success read from efd, read %d bytes(%llu) at %lds %ldus\n",ret, count, tv_end.tv_sec, tv_end.tv_usec);
                                round_time[i] = ((dwEnd - dwStart) / mhz)/2.0;
                                // once_time[i] = round_time[i]/2.0;
                            }  
                        }  
                    }  
                }  
                else if (ret == 0)  
                {  
                    /* time out */  
                    printf("epoll wait timed out.\n");  
                    break;  
                }  
                else  
                {  
                    perror("epoll wait error:");  
                    goto fail;  
                }  
            }  
        }  
        usleep(1000);  
    }
    printf("success_count:%d\n", success_count);
    if( i != success_count )
    {
        printf("error\n");
        return 0;
    }
    qsort(round_time,TEST_TIMES,sizeof(round_time[0]),cmp);
    int int_min_perc = 0;
    int int_mid_perc = (int)(TEST_TIMES/2);
    int int_max_perc = TEST_TIMES-1;
    int int_99_perc = (int)(TEST_TIMES * 0.99);
    if( int_99_perc == TEST_TIMES)   int_99_perc = TEST_TIMES-1;
    int int_99_9_perc = (int)(TEST_TIMES * 0.999);
    if( int_99_9_perc == TEST_TIMES)   int_99_9_perc = TEST_TIMES-1;


    double sum = 0;
    for(i=0;i<TEST_TIMES;i++)
    {
        sum += round_time[i];
        // printf("round_time:%ld us \t,once_time:%.4lf us\n", round_time[i],(round_time[i])/2.0);
    }
    // printf("avg:%.4lf, %.4lf\n", sum/1000.0,sum/2000.0);

    printf("次数\t:%d\n",TEST_TIMES);

    printf( "最小值[%d]\t:%.4lf us\n",int_min_perc,round_time[int_min_perc] );
    printf( "中间值[%d]\t:%.4lf us\n",int_mid_perc,round_time[int_mid_perc] );
    printf( "99%%值[%d]\t:%.4lf us\n",int_99_perc,round_time[int_99_perc] );
    printf( "99.9%%值[%d]\t:%.4lf us\n",int_99_9_perc,round_time[int_99_9_perc] );
    printf( "最大值[%d]\t:%.4lf us\n",int_max_perc,round_time[int_max_perc] );

    printf("平均值\t:%.4lf us\n",sum/TEST_TIMES );


fail:  
    if (0 != pid)  
    {  
        pthread_join(pid, NULL);  
        pid = 0;  
    }  
  
    if (efd >= 0)  
    {  
        close(efd);  
        efd = -1;  
    }  



    return ret;  
}  