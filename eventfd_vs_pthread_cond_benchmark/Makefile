all:
	g++ -g -o eventfd_test main.cc -DHAVE_EVENTFD -lpthread
	g++ -g -o pthread_cond_test main.cc -lpthread
	# gcc -g -o get_clock get_clock.c

# %.o : %.c
# 	gcc  -o $@ -c $<

clean:
	rm -rf *.o
	rm -rf *_test
