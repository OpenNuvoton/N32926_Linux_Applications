#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <getopt.h>
#include <stdint.h>
#include <fcntl.h>

#include "aes_sw.h"
#include "aes_cryptodev.h"

static volatile bool is_exit = false;
static void my_sighdlr (int sig)
{
	if (sig == SIGPIPE) { // Ignore SIGPIPE.
		return;
	}
	
	is_exit = true;
}

void reg_sighdlr()
{
	signal(SIGINT, my_sighdlr);
	signal(SIGHUP, my_sighdlr);
	signal(SIGQUIT, my_sighdlr);
	signal(SIGILL, my_sighdlr);
	signal(SIGABRT, my_sighdlr);
	signal(SIGFPE, my_sighdlr);
	signal(SIGKILL, my_sighdlr);
	//signal(SIGSEGV, my_sighdlr);
	signal(SIGPIPE, my_sighdlr);
	signal(SIGTERM, my_sighdlr);
}
	
enum {
	AES_BLK_SIZE				=	16,
	AES_IV_SIZE					=	16,
	AES_MAX_KEY_SIZE			=	32,
};

enum {
	AES_TESTDATA_SIZE			=	0x40000,
};

static unsigned long act_buff_size = 0;

static uint8_t buff_plain_orig[AES_TESTDATA_SIZE];	// plain data
static uint8_t buff_aes_sw_encdec[AES_TESTDATA_SIZE];
static uint8_t buff_aes_cryptodev_encrypt[AES_TESTDATA_SIZE];
static uint8_t buff_aes_cryptodev_decrypt[AES_TESTDATA_SIZE];

static uint8_t key[AES_MAX_KEY_SIZE];
static uint8_t iv[AES_IV_SIZE];

static int fd_rand = -1;

static int key_size = 16;

static bool cryptodev_ctx_inited = false;
static struct aes_cryptodev_ctx cryptodev_ctx;

class time_elapsed_log
{
public:
	time_elapsed_log(const char *prefix)
	{
		m_log_prefix = prefix;
		gettimeofday(&m_tv_begin, NULL);
	}
	
	~time_elapsed_log()
	{
		gettimeofday(&m_tv_end, NULL);
	
		int elapsed = (m_tv_end.tv_sec - m_tv_begin.tv_sec) * 1000 + (m_tv_end.tv_usec - m_tv_begin.tv_usec) / 1000;
		printf("%s%d(ms)\n", m_log_prefix, elapsed);
	}
	
private:
	const char *m_log_prefix;
	struct timeval m_tv_begin;
	struct timeval m_tv_end;
};

int main(int argc, char *argv[])
{
	reg_sighdlr();
	
	char mode[16] = {'c', 'b', 'c', 0};
	bool test_aes_sw = true;
	bool test_aes_cryptodev = true;
	
	while (1) {
		int c;
		int option_index = 0;
		
		static struct option long_options[] = {
			{"mode", required_argument, 0, 'm'},
			{"sw", required_argument, 0, 's'},
			{"cryptodev", required_argument, 0, 'c'},
			{0, 0, 0, 0}
		};
		
		c = getopt_long (argc, argv, "m:s:c:", long_options, &option_index);
		
		// Detect the end of the options.
		if (c == -1) {
			break;
		}
		
		switch (c) {
		case 'm':
			snprintf(mode, sizeof (mode), "%s", optarg);
			break;
			
		case 's':
			test_aes_sw = !!atoi(optarg);
			break;
			
		case 'c':
			test_aes_cryptodev = !!atoi(optarg);
			break;
		
		default:
			printf ("Usage: %s [-m mode][-s sw][-c cryptodev]\n", argv[0]);
			exit(1);
		}
	}
	
	int round_nr = 0;
	printf("Press Ctrl-C to exit ...\n");
	printf("Mode of operation: %s\n", mode);
	do {
		printf("[AES Demo] Round #%d ...\n", round_nr + 1);
		
		if (fd_rand == -1) {
			fd_rand = open("/dev/urandom", O_RDONLY);
			if (fd_rand == -1) {
				printf("[AES Demo] Open /dev/urandom failed: %s\n", strerror(errno));
				break;
			}
		}
		
		// Specify data size at random.
		if (read(fd_rand, &act_buff_size, sizeof (act_buff_size)) != sizeof (act_buff_size)) {
			printf("[AES Demo] Read /dev/urandom %d bytes failed\n", sizeof (act_buff_size));
			break;
		}
		act_buff_size = act_buff_size % AES_TESTDATA_SIZE;
		if (! act_buff_size) {
			act_buff_size = AES_TESTDATA_SIZE;
		}
		act_buff_size = act_buff_size & ~(AES_BLK_SIZE - 1);	// AES enc/dec requires to be block-aligned.
		if (! act_buff_size) {
			continue;
		}
		printf("[AES Demo] Plain data size: %lu\n", act_buff_size);
		
		// Generate plain data at random.
		if (read(fd_rand, buff_plain_orig, act_buff_size) != act_buff_size) {
			printf("[AES Demo] Read /dev/urandom %d bytes failed\n", act_buff_size);
			break;
		}
		
		// Specify key size at random.
		unsigned int key_rand;
		if (read(fd_rand, &key_rand, sizeof (key_rand)) != sizeof (key_rand)) {
			printf("[AES Demo] Read /dev/urandom %d bytes failed\n", sizeof (key_rand));
			break;
		}
		key_size = 64 * (key_rand % 3 + 2) / 8;	// 16-, 24-, or 32-byte key
		if (read(fd_rand, key, key_size) != key_size) {
			printf("[AES Demo] Read /dev/urandom %d bytes failed\n", key_size);
			break;
		}
		printf("[AES Demo] Key size: %d-byte\n", key_size);
		
		// Generate initial vector at random.
		if (read(fd_rand, iv, AES_IV_SIZE) != AES_IV_SIZE) {
			printf("[AES Demo] Read /dev/urandom %d bytes failed\n", AES_IV_SIZE);
			break;
		}
		
		// S/W AES init.
		aes_context sw_ctx;
		if (test_aes_sw && aes_set_key(&sw_ctx, key, key_size * 8, mode)) {
			printf("[AES Demo] aes_set_key failed\n");
			break;
		}
		
		// H/W AES init.
		if (test_aes_cryptodev && ! cryptodev_ctx_inited) {
			if (! aes_cryptodev_init(&cryptodev_ctx, key, key_size, mode)) {
				break;
			}
			cryptodev_ctx_inited = true;
		}
		
		// S/W AES encrypt.
		if (test_aes_sw) {
			memcpy(buff_aes_sw_encdec, buff_plain_orig, act_buff_size);
			{
				time_elapsed_log log("[AES Demo] S/W AES Encrypt: ");
				aes_encrypt(&sw_ctx, iv, buff_aes_sw_encdec, act_buff_size);
			}
		}
		
		// H/W AES encrypt.
		if (test_aes_cryptodev) {
			time_elapsed_log log("[AES Demo] CryptoDev AES Encrypt: ");
			if (! aes_cryptodev_encrypt(&cryptodev_ctx, iv, buff_plain_orig, buff_aes_cryptodev_encrypt, act_buff_size)) {
				break;
			}
		}
		
		// Compare S/W and H/W AES encrypted data
		if (test_aes_sw && test_aes_cryptodev) {
			if (memcmp(buff_aes_sw_encdec, buff_aes_cryptodev_encrypt, act_buff_size)) {
				printf("[AES Demo] H/W AES encrypted data differs than S/W AES encrypted data\n");
				break;
			}
		}
		
		// S/W AES decrypt.
		if (test_aes_sw) {
			time_elapsed_log log("[AES Demo] S/W AES Decrypt: ");
			aes_decrypt(&sw_ctx, iv, buff_aes_sw_encdec, act_buff_size);
		}
		
		// Compare S/W AES decrypted data with original.
		if (test_aes_sw && memcmp(buff_aes_sw_encdec, buff_plain_orig, act_buff_size)) {
			printf("[AES Demo] S/W AES decrypted data differs than original\n");
			break;
		}
		
		// H/W AES decrypt.
		if (test_aes_cryptodev) {
			time_elapsed_log log("[AES Demo] CryptoDev AES Decrypt: ");
			if (! aes_cryptodev_decrypt(&cryptodev_ctx, iv, buff_aes_cryptodev_encrypt, buff_aes_cryptodev_decrypt, act_buff_size)) {
				break;
			}
		}
		
		// Compare H/W AES decrypted data with original.
		if (test_aes_cryptodev && memcmp(buff_aes_cryptodev_decrypt, buff_plain_orig, act_buff_size)) {
			printf("[AES Demo] H/W AES decrypted data differs than original\n");
			break;
		}

		// H/W AES fini.
		if (test_aes_cryptodev && cryptodev_ctx_inited) {
			aes_cryptodev_fini(&cryptodev_ctx);
			cryptodev_ctx_inited = false;
		}
	
		printf("[AES Demo] Round #%d (OK)\n\n", round_nr + 1);
		round_nr ++;
	}
	while (! is_exit);
	
	// H/W AES fini.
	if (test_aes_cryptodev && cryptodev_ctx_inited) {
		aes_cryptodev_fini(&cryptodev_ctx);
		cryptodev_ctx_inited = false;
	}
		
	if (fd_rand >= 0) {
		close(fd_rand);
		fd_rand = -1;
	}
	
	return 0;
}
