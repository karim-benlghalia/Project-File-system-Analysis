#NAME: Karim Benlghalia, Juhyeok Min
#EMAIL: kbenlghalia@ucla.edu, alswngur5445@g.ucla.edu
#ID: 105179657, 505180791
.SILENT:

default:
	gcc -g -Wall -Wextra lab3a.c -o lab3a
clean:
	rm -rf lab3a-505180791.tar.gz lab3a test.csv
dist:
	tar -cvzf lab3a-505180791.tar.gz lab3a.c Makefile README ext2_fs.h
