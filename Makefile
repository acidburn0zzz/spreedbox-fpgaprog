fpga: fpga.c
	gcc -g -O0 -o fpga fpga.c -I/usr/local/include -L/usr/local/lib -lwiringPi -lpthread
