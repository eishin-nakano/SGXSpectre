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

#include "enclave_t.h"

#define N 8192			//64KiB: 8192, 128KiB: 16384,	256KiB: 32768
#define LEN 65536		//64KiB: 65536, 128KiB: 131072,	256KiB: 262144

//unsigned int array1_size = 16;
uint8_t unused1[64];
uint8_t array1[160] = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
uint8_t unused2[64];

char *secret = "The Magic Words are Squeamish Ossifrage.";
//double secretKey[] = {1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0,9.0,0.0,1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0,9.0,0.0,1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0,9.0,0.0,1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0,9.0,0.0};
uint64_t secretKey[N];
uint8_t temp = 0; /* Used so compiler won’t optimize out victim_function() */
uint64_t dummyKey[N];

size_t ecall_get_offset() { 
	temp = secretKey[0]; //Bring secrete into cache.
	return (size_t)((char*)secretKey-(char*)array1);
}

void ecall_load_key(uint64_t *data) {
	for(int i = 0; i < N; i++) secretKey[i] = data[i];
	// for(int i = 0; i < LEN; i++) secretKey[i] = i;
}

void ecall_victim_function(size_t x, uint8_t * array2, unsigned int * outside_array1_size) {
	//if (x < array1_size) {
	if (x < *outside_array1_size) {
		temp &= array2[array1[x] * 512];
	}
}

void ecall_decrypt(size_t x, uint64_t* c_0, uint64_t *c_1, uint64_t  *plaintext, uint8_t * array2, unsigned int *outside_array1_size) {
	if(x < *outside_array1_size) {
		temp &= array2[array1[x] * 512];
		plaintext[x] = c_0[x] + c_1[x]*secretKey[x];
	}
}

void ecall_decrypt_only(size_t x, uint64_t* c_0, uint64_t *c_1, uint64_t  *plaintext, unsigned int *outside_array1_size){
	if(x < *outside_array1_size) {
		plaintext[x] = c_0[x] + c_1[x]*secretKey[x];
	}
}

void ecall_access_sk(){
	for(int i=0; i<N; i++){
		dummyKey[i]+=secretKey[i];
	}

}
#include <string.h>
#define CACHE_LINE_SIZE 64
#define DATA_SIZE (12 * 1024 * 1024)  // 1MBのデータでキャッシュを汚染

void ecall_pollute_cache() {
    char data[DATA_SIZE];
    volatile int sum = 0;  // コンパイラの最適化を防ぐため volatile を使用

    // メモリを繰り返しアクセスしてキャッシュを汚染
    for (size_t i = 0; i < DATA_SIZE; i += CACHE_LINE_SIZE) {
        data[i] = (char)(i & 0xFF);  // データを書き込み
        sum += data[i];              // 値を参照して警告を防ぐ
    }

    // 最終的な結果を使わないようにしてもOK（ここでは単に計算した値を捨てる）
    (void)sum;
}
