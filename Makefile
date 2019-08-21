# ID: 904599241, 505458185
# EMAIL: sriramsonti1997@gmail.com, rohitfalor@g.ucla.edu
# NAME: Venkata Sai Sriram Sonti, Rohit Falor

default:
	gcc -o lab3a lab3a.c -Wall -Wextra -lm
clean:
	rm -rf lab3a lab3a-505458185.tar.gz
dist:
	tar -cvzf lab3a-505458185.tar.gz lab3a.c README Makefile ext2_fs.h
