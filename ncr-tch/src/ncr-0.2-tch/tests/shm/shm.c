#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <semaphore.h>
#include <stdio.h>
#include <aes.h>

sem_t *enc_sem, *get_sem;

#define SHM_SIZE 64*1024

void child(pid_t parent, void *mem)
{
	char key[16];
	char iv[16];
	struct crypto_aes_ctx ctx;
	uint32_t mem_size;

	mem_size = *((uint32_t*)mem);

	for (;;) {
		memset(iv, 0x3, sizeof(iv));
		memset(key, 0xa3, sizeof(key));
		crypto_aes_expand_key(&ctx, (void *)key, sizeof(key));

		sem_wait(enc_sem);

		mem_size = *((uint32_t*)mem);

		crypto_cbc_encrypt(&ctx, mem, mem_size, mem, iv);

		sem_post(get_sem);
	}
}

static volatile int must_finish = 0;

static void alarm_handler(int signo)
{
	must_finish = 1;
}

static double udifftimeval(struct timeval start, struct timeval end)
{
	return (double)(end.tv_usec - start.tv_usec) +
	    (double)(end.tv_sec - start.tv_sec) * 1000 * 1000;
}

static char *si_units[] = { "", "K", "M", "G", "T", 0};

static void value2human(double bytes, double time, double* data, double* speed,char* metric)
{
	int unit = 0;

	*data = bytes;
	
	while (*data > 1000 && si_units[unit + 1]) {
		*data /= 1000;
		unit++;
	}
	*speed = *data / time;
	sprintf(metric, "%sB", si_units[unit]);
}

void parent(pid_t child, void *mem)
{
	struct timeval start, end;
	uint32_t chunksize;
	double total = 0;
	double secs, ddata, dspeed;
	char metric[16];

	signal(SIGALRM, alarm_handler);

	/* set a default value in shared memory */

	for (chunksize = 512; chunksize <= (64 * 1024); chunksize *= 2) {
		memset(mem, 0x33, chunksize);

		printf("\tEncrypting in chunks of %d bytes: ", chunksize);
		fflush(stdout);

		total = 0;
		must_finish = 0;
		alarm(5);
		gettimeofday(&start, NULL);

		do {
			memcpy(mem, &chunksize, sizeof(chunksize));

			sem_post(enc_sem);

			sem_wait(get_sem);
			total += chunksize;
		} while (must_finish == 0);

		gettimeofday(&end, NULL);

		secs = udifftimeval(start, end) / 1000000.0;
		value2human(total, secs, &ddata, &dspeed, metric);
		printf("done. %.2f %s in %.2f secs: ", ddata, metric, secs);
		printf("%.3f %s/sec\n", dspeed, metric);
	}

}

int main()
{
	int shmid, shmid2;
	char c, *shm, *s, *semmem;
	pid_t pid;

	signal(SIGUSR1, SIG_IGN);
	signal(SIGUSR2, SIG_IGN);

	if ((shmid = shmget(IPC_PRIVATE, SHM_SIZE, IPC_CREAT | 0660)) < 0) {
		perror("shmget fail");
		return 1;
	}

	if ((shm = (char *)shmat(shmid, 0, 0)) == (char *)-1) {
		perror("shmat : parent");
		return 2;
	}

	if ((shmid2 =
	     shmget(IPC_PRIVATE, 2 * sizeof(sem_t), IPC_CREAT | 0660)) < 0) {
		perror("shmget fail");
		return 1;
	}

	if ((semmem = (char *)shmat(shmid2, 0, 0)) == (char *)-1) {
		perror("shmat : parent");
		return 2;
	}

	enc_sem = (void *)semmem;
	get_sem = (void *)semmem + sizeof(sem_t);

	sem_init(enc_sem, 1, 0);
	sem_init(get_sem, 1, 0);

	printf("Addresses in parent\n");
	printf("shared mem: %p\n", shm);

	s = shm;		// s now references shared mem
	for (c = 'A'; c <= 'Z'; ++c)	// put some info there
		*s++ = c;
	*s = '\0';		// terminate the sequence

	switch (pid = fork()) {
	case -1:
		perror("fork");
		return 3;
	default:
		parent(pid, shm);
		kill(pid, SIGTERM);
		wait(0);	// let the child finish
		sem_destroy(get_sem);
		sem_destroy(enc_sem);
		shmdt(shm);
		shmdt(semmem);
		shmctl(shmid, IPC_RMID, (struct shmid_ds *)0);
		shmctl(shmid2, IPC_RMID, (struct shmid_ds *)0);
		break;
	case 0:
		child(getppid(), shm);
		shmdt(semmem);
		shmdt(shm);
		break;
	}
	return 0;
}
