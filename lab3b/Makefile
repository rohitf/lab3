# NAME: Rohit Falor
# EMAIL: rohitfalor@g.ucla.edu
# ID: 505458185

# CC = /usr/local/cs/bin/gcc -pthread
CC = gcc
ADD = lab2_add
LIST = lab2_list
ID = 505458185

build:
	$(CC) -Wall -Wextra -o $(ADD) $(ADD).c

tests:
	rm -rf $(ADD).csv

	# None
	for (( i=10; i<=100000; i*=10)); \
		do \
			./$(ADD) --threads=1 --iterations=$$i >> $(ADD).csv; \
		done

	# Add-1, 2, 3
	for t in 2 4 8 12; \
	do \
		for i in 10 20 40 80 100 1000 10000 100000; \
		do \
				./$(ADD) --threads=$$t --iterations=$$i >> $(ADD).csv; \
				./$(ADD) --threads=$$t --iterations=$$i --yield >> $(ADD).csv; \
		done \
	done

	# Add-4
	for t in 2 4 8 12; \
	do \
		./$(ADD) --threads=$$t --iterations=1000 --yield >> $(ADD).csv; \
		./$(ADD) --threads=$$t --iterations=10000 --yield --sync=m >> $(ADD).csv; \
		./$(ADD) --threads=$$t --iterations=1000 --yield --sync=s >> $(ADD).csv; \
		./$(ADD) --threads=$$t --iterations=10000 --yield --sync=c >> $(ADD).csv; \
	done

	# Add-5
	for t in 1 2 4 8 12; \
	do \
		./$(ADD) --threads=$$t --iterations=10000 >> $(ADD).csv; \
		./$(ADD) --threads=$$t --iterations=10000 --sync=m >> $(ADD).csv; \
		./$(ADD) --threads=$$t --iterations=10000 --sync=s >> $(ADD).csv; \
		./$(ADD) --threads=$$t --iterations=10000 --sync=c >> $(ADD).csv; \
	done

graphs:
	gnuplot ./lab2_add.gp

clean:
	$(RM) $(ADD) lab2a-$(ID).tar.gz

dist:
	tar -czvf lab2a-$(ID).tar.gz README $(ADD).c $(LIST).c SortedList.c *.png *.csv Makefile 
