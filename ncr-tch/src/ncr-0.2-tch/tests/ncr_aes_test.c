/*
 * Demo on how to use /dev/ncr device for HMAC.
 *
 * Placed under public domain.
 *
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include "../ncr.h"
#include <stdlib.h>

#define DATA_SIZE 4096

enum{
OP_ENCRYPT,
OP_DECRYPT
};

static char in_data[DATA_SIZE];
static char out_data[DATA_SIZE];

static void randomize_data(uint8_t * data, size_t data_size)
{
  int i;

  srand(time(0) * getpid());
  for (i = 0; i < data_size; i++) {
    data[i] = rand() & 0xff;
  }
}

#define KEY_DATA_SIZE 16
#define WRAPPED_KEY_DATA_SIZE 32

void test_ncr_aes_enc_dec(int fd, char *in_file_name, char *out_file_name, char *iv, int operation)
{
  FILE * in_fd;
  FILE * out_fd;
  int in_size;
  int out_size;
  ncr_key_t key = 1;
  NCR_STRUCT(ncr_session_once) op;
  struct nlattr *nla;
  size_t data_size;

  in_fd = fopen(in_file_name, "r");
  if(in_fd == NULL)
    return;

  in_size = fread(in_data, 1, DATA_SIZE, in_fd);
  if(in_size <= 0)
    return;

  fclose(in_fd);

  nla = NCR_INIT(op);
  op.f.op = ((operation == OP_ENCRYPT) ? NCR_OP_ENCRYPT : NCR_OP_DECRYPT);
  ncr_put_u32(&nla, NCR_ATTR_ALGORITHM, NCR_ALG_AES_ECB);
  ncr_put_u32(&nla, NCR_ATTR_KEY, key);
  ncr_put(&nla, NCR_ATTR_IV, iv, 32);
  ncr_put_session_input_data(&nla, NCR_ATTR_UPDATE_INPUT_DATA,
                             in_data, in_size);
  ncr_put_session_output_buffer(&nla,
                                NCR_ATTR_UPDATE_OUTPUT_BUFFER,
                                out_data, sizeof(out_data), &data_size);
  NCR_FINISH(op, nla);

  if (ioctl(fd, NCRIO_SESSION_ONCE, &op)) {
    fprintf(stderr, "Error: %s:%d\n", __func__, __LINE__);
    perror("ioctl(NCRIO_SESSION_ONCE)");
    return;
  }

  out_fd = fopen(out_file_name, "w");
  if(out_fd == NULL)
    return;

  out_size = fwrite(out_data, data_size, 1, out_fd);
  fclose(out_fd);

  return;
}

void test_ncr_aes_dec(int fd, char *in_file_name, char *out_file_name, char *iv)
{
  return;
}

int main(int argc, char *argv[])
{
  int opt;
  int operation;
  char * secrete;
  char * in;
  char * out;
  char iv[32] = {0};
  int fd = -1;

  operation = OP_ENCRYPT;

  if(argc <= 1)
  {
    fprintf(stderr, "Usage: %s [ -e or -d ] -i inFile -o outFile \n",
            argv[0]);
    exit(0);
  }

  while ((opt = getopt(argc, argv, "eds:i:o:")) != -1) {
    switch (opt) {
      case 'e':
        operation = OP_ENCRYPT;
        break;
      case 'd':
        operation = OP_DECRYPT;
        break;
      case 'i':
        in = strdup(optarg);
        break;
      case 'o':
        out = strdup(optarg);
        break;
      default: 
        fprintf(stderr, "Usage: %s [ -e or -d ] -s secret  -i inFile -o outFile \n",
                argv[0]);
        exit(0);
    }
  }

  /* Open the crypto device */
  fd = open("/dev/ncr", O_RDWR, 0);
  if (fd < 0) {
    perror("open(/dev/ncr)");
    return 1;
  }

  test_ncr_aes_enc_dec(fd, in, out, iv, operation);

  /* Close the original descriptor */
  if (close(fd)) {
    perror("close(fd)");
    return 1;
  }

  return 0;
}
