all: oss user  

%.o: %.c 
	$(CC) -c -std=gnu99 $< 

oss: oss.o sharedMemory.o timestamp.o
	gcc -o oss oss.o sharedMemory.o timestamp.o -pthread
	
user: user.o sharedMemory.o timestamp.o
	gcc -o user user.o sharedMemory.o timestamp.o  -pthread

clean:
	rm oss user *.o *.out