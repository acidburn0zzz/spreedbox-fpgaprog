
#include <ctype.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include <wiringPi.h>

#include <pthread.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static const char *device = "/dev/spidev0.0";
static int fd = -1;

static uint8_t bits = 8;
static uint32_t speed = 2*1000*1000;
static uint16_t spidelay = 0;

typedef int bool;
#define true 1
#define false 0

#define UNUSED(x) (void)(x)

void close_spi();


static void errexit(const char *s)
{
  fprintf(stderr, "%s\n",s);

  if (fd>=0) { close_spi(); }

  exit(10);
}

void init_spi()
{
  fd = open(device, O_RDWR);
  if (fd < 0)
    { errexit("can't open device"); }

  int ret;

  /*
   * bits per word
   */
  ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
  if (ret == -1)
    errexit("can't set bits per word");

  ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
  if (ret == -1)
    errexit("can't get bits per word");

  /*
   * max speed hz
   */
  ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
  if (ret == -1)
    errexit("can't set max speed hz");

  ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
  if (ret == -1)
    errexit("can't get max speed hz");


  uint8_t mode = SPI_CPOL | SPI_CPHA;
  ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
  if (ret == -1)
    errexit("can't set spi mode");

  /*
    printf("spi mode: 0x%x\n", mode);
    printf("bits per word: %d\n", bits);
    printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);
  */
}


void close_spi()
{
  if (fd>=0) close(fd);
  fd = -1;
}


void send_spi(const uint8_t* data, int n)
{
  uint8_t* rx = malloc(n);

  struct spi_ioc_transfer tr;
  tr.tx_buf = (unsigned long)data;
  tr.rx_buf = (unsigned long)rx;
  tr.len = n;
  tr.delay_usecs = spidelay;
  tr.speed_hz = speed;
  tr.bits_per_word = bits;


  const int blksize=4096;
  int i=0;

  while (i<n) {
    int len = n-i;
    if (len>blksize) len=blksize;

    tr.tx_buf = (unsigned long)&data[i];
    tr.len = len;

    i+= len;

    int ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 1)
      {
        perror("can't send spi message");
      }
  }
}


uint8_t* readFile(const char* filename, int* len)
{
  FILE* fh = fopen(filename,"rb");
  if (!fh) {
    perror("cannot open file");
    exit(10);
  }

  fseek(fh,0,SEEK_END);
  *len = ftell(fh);
  fseek(fh,0,SEEK_SET);

  uint8_t* data = (uint8_t*)malloc(*len);

  int n = fread(data,1,*len,fh);
  if (n!=*len) {
    perror("error reading bitmap file");
    exit(10);
  }

  // printf("read file of length %d\n",*len);

  return data;
}


void print_help(const char* prgname)
{
  fprintf(stderr,
          "usage: %s ice40.bitmap\n"
          " -d NAME  SPI device (default: %s)\n",
          prgname,device);
}

int main(int argc, char *argv[])
{
  int opt;
  while ((opt = getopt(argc, argv, "hd:")) != -1) {
    switch (opt) {
    case 'h':
      print_help(argv[0]);
      exit(5);
    case 'd':
      device = optarg;
      break;
    }
  }

  if (optind==argc ||  // no file given
      optind<argc-1) { // >=two files given
    print_help(argv[0]);
    exit(5);
  }


  const char* bitmap_file = argv[optind];

  int len;
  uint8_t* data = readFile(bitmap_file, &len);

  init_spi();

  wiringPiSetup();
  pinMode(6, OUTPUT); // CRESET
  pinMode(5, OUTPUT); // SS
  //pinMode(14, OUTPUT); // CLK
  pinMode(4, INPUT); // CDONE

  digitalWrite(6, 0); // CRESET=0
  digitalWrite(5, 0);
  //digitalWrite(14, 1);
  usleep(1);

  digitalWrite(6, 1); // CRESET=1
  usleep(500);

  int cdone = digitalRead(4);
  //printf("initial cdone: %d\n",cdone);
  if (cdone==1) {
    free(data);
    close_spi();

    fprintf(stderr,"error writing to FPGA, CDONE=1 after reset\n");
    exit(5);
  }

  send_spi(data,len);

  usleep(500);
  cdone = digitalRead(4);
  //printf("cdone: %d\n",cdone);

  if (cdone) {
    usleep(100);

    // send dummy 0-bits

    uint8_t nulldata = 0;

    int i;
    for (i=0;i<8;i++)
      send_spi(&nulldata,1);
  }

  free(data);
  close_spi();

  if (cdone==0) {
    fprintf(stderr,"error writing to FPGA: no response from FPGA\n");
    exit(5);
  }

  return 0;
}
