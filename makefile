all: oss user  

%.o: %.c 
	$(CC) -c -std=gnu99 $< 

oss: oss.o sharedMemory.o timestamp.o queue.o
	gcc -o oss oss.o sharedMemory.o timestamp.o queue.o -pthread
	
user: user.o sharedMemory.o timestamp.o queue.o
	gcc -o user user.o sharedMemory.o timestamp.o queue.o -pthread

clean:
	rm oss user *.o *.out