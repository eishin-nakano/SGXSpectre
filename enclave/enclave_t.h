#ifndef ENCLAVE_T_H__
#define ENCLAVE_T_H__

#include <stdint.h>
#include <wchar.h>
#include <stddef.h>
#include "sgx_edger8r.h" /* for sgx_ocall etc. */


#include <stdlib.h> /* for size_t */

#define SGX_CAST(type, item) ((type)(item))

#ifdef __cplusplus
extern "C" {
#endif

size_t ecall_get_offset(void);
void ecall_victim_function(size_t x, uint8_t* array2, unsigned int* outside_array1_size);
void ecall_decrypt(size_t x, uint64_t* c_0, uint64_t* c_1, uint64_t* plaintext, uint8_t* array2, unsigned int* outside_array1_size);
void ecall_decrypt_only(size_t x, uint64_t* c_0, uint64_t* c_1, uint64_t* plaintext, unsigned int* outside_array1_size);
void ecall_load_key(uint64_t* data);
void ecall_access_sk(void);
void ecall_pollute_cache(void);

sgx_status_t SGX_CDECL sgx_oc_cpuidex(int cpuinfo[4], int leaf, int subleaf);
sgx_status_t SGX_CDECL sgx_thread_wait_untrusted_event_ocall(int* retval, const void* self);
sgx_status_t SGX_CDECL sgx_thread_set_untrusted_event_ocall(int* retval, const void* waiter);
sgx_status_t SGX_CDECL sgx_thread_setwait_untrusted_events_ocall(int* retval, const void* waiter, const void* self);
sgx_status_t SGX_CDECL sgx_thread_set_multiple_untrusted_events_ocall(int* retval, const void** waiters, size_t total);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
