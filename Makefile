# NAME: Meiyi Zheng
# EMAIL: meiyizheng@yahoo.com
# ID: 605147145

default: lab4c_tcp lab4c_tls

lab4c_tcp:
	gcc -g lab4c_tcp.c -o lab4c_tcp -lmraa -lm -Wall -Wextra

lab4c_tls:
	gcc -g lab4c_tls.c -o lab4c_tls -lmraa -lm -lssl -lcrypto -Wall -Wextra

clean:
	rm -rf *.tar.gz lab4c_tcp lab4c_tls

dist:
	tar -cvzf lab4c-605147145.tar.gz lab4c_tcp.c lab4c_tls.c Makefile README
