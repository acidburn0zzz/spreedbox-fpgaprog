spreedbox-fpgarog: spreedbox-fpgaprog.c
	gcc -g -O0 -o spreedbox-fpgaprog spreedbox-fpgaprog.c -I/usr/local/include -L/usr/local/lib -lwiringPi -lpthread
