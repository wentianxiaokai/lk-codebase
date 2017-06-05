#include <sys/eventfd.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

class LogNotify
{
  public:
    LogNotify();
    void signal(void);
    void wait(void);
    void lock(void);
    void unlock(void);
    ~LogNotify();

  private:
  #if defined(HAVE_EVENTFD)
    int m_efd;
  #else
    pthread_cond_t m_cond;
    pthread_mutex_t m_mutex;
  #endif
};

LogNotify::LogNotify()
{
  #if defined(HAVE_EVENTFD)
    m_efd = eventfd(0, 0);
    assert(m_efd != -1);
  #else
    pthread_cond_init(&m_cond, NULL);
    pthread_mutex_init(&m_mutex, NULL);
  #endif
}

void LogNotify::signal(void)
{
  #if defined(HAVE_EVENTFD)
    ssize_t nr;
    uint64_t value = 1;
    nr = write(m_efd, &value, sizeof(uint64_t));
    assert(nr == sizeof(uint64_t));
  #else
    pthread_cond_signal(&m_cond);
  #endif
}

void LogNotify::wait(void)
{
  #if defined(HAVE_EVENTFD)
    ssize_t nr;
    uint64_t value = 0;
    nr = read(m_efd, &value, sizeof(uint64_t));
    assert(nr == sizeof(uint64_t));
  #else
    pthread_cond_wait(&m_cond, &m_mutex);
  #endif
}

void LogNotify::lock(void)
{
#if defined(HAVE_EVENTFD)
  // do nothing
#else
  pthread_mutex_lock(&m_mutex);
#endif
}

void LogNotify::unlock(void)
{
#if defined(HAVE_EVENTFD)
  // do nothing
#else
  pthread_mutex_unlock(&m_mutex);
#endif
}

LogNotify::~LogNotify()
{
  #if defined(HAVE_EVENTFD)
    close(m_efd);
  #else
    pthread_cond_destroy(&m_cond);
    pthread_mutex_destroy(&m_mutex);
  #endif
}

#define nr_producer 10
int consumer_loop_times = 100000;

LogNotify notify;

void *producer(void *)
{
  while (true) {
    notify.signal();
  }
}

void *consumer(void *)
{
  int i = 0;

  notify.lock();

  while (i++ < consumer_loop_times) {
    notify.wait();
  }

  notify.unlock();

  exit(0);
}


int main(int argc, char **argv)
{
  int i;
  pthread_t pid[nr_producer];

  if (argc > 1) {
    consumer_loop_times = atoi(argv[1]);
  }

  for (i = 0; i < nr_producer; i++) {
    pthread_create(&pid[i], NULL, producer, (void*)NULL);
  }

  consumer(NULL);
  return 0;
}
