all:libtask.a
libtask.a:task.o channel.o fd.o
	ar -r $@ $^
task.o:task.c impl.h task.h
	gcc -g -c $< -o $@
channel.o:channel.c impl.h task.h
	gcc -g -c $< -o $@
fd.o:fd.c task.h impl.h
	gcc -g -c $< -o $@
task:unit_test.c task.o
	gcc -D_TASK_TEST -g $^ -o $@
channel:unit_test.c task.o channel.o
	gcc -D_CHAN_TEST -g $^ -o $@
fd:unit_test.c task.o fd.o
	gcc -D_FD_TEST -g $^ -o $@
clean:
	rm -f libtask.a *.o task channel fd
