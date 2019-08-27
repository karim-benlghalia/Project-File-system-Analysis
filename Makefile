#NAME: Karim Benlghalia, Juhyeok Min
#EMAIL: kbenlghalia@ucla.edu, alswngur5445@g.ucla.edu
#ID: 105179657, 505180791

UID1=105179657
UID2=505180791
default:lab3b.py
	rm -f lab3b
	ln -s lab3b.py lab3b
	chmod u+x lab3b
	find . -type f | xargs touch
dist:
	tar -cvzf lab3b-$(UID1).tar.gz Makefile README lab3b.py
	tar -cvzf lab3b-$(UID2).tar.gz Makefile README lab3b.py
	find . -type f | xargs touch
clean:
	rm -f lab3b lab3b-$(UID1).tar.gz
	rm -f lab3b lab3b-$(UID2).tar.gz
	find . -type f | xargs touch
