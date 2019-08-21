.SILENT:

default:
	gcc -g -Wall -Wextra lab3a.c -o lab3a
test: default
	./lab3a trivial.img | sort > test.csv
clean:
	rm -rf lab3a-505180791.tar.gz lab3a test.csv
dist:
	tar -cvzf lab3a-505180791.tar.gz lab3a.c Makefile README
