event_fd_test.c:
// ���߳� write��֪ͨ���̣߳�
// Ȼ�����߳� read����ʾ�õ�֪ͨ,Ȼ�� write��֪ͨ���߳�
// ������߳� read����ʾ�õ����̵߳�֪ͨ��
// �����ܹ��� 2 ���¼�֪ͨ�ĺ�ʱ��
// �� 32�� 2.4Ghz��120G�ڴ�����û����£�һ���¼�֪ͨ�ĺ�ʱ��ƽ��ֵ��  3.8 ΢�롣
// event_fd ������Ҫ�� pipe �� pthread_cond_wait ������Ҫ�á�
// gcc -o event_fd_test.out event_fd_test.c get_clock.c -lpthread 
