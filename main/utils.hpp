#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string>

#define N 16384    //64KiB: 8192, 128KiB: 16384,  256KiB: 32768
#define LEN 131072		//64KiB: 65536, 128KiB: 131072,	256KiB: 262144
extern int ret;

#define SGX_ASSERT(f)  { if ( 0 != (ret = (f)) )                \
 {                                                                      \
       printf( "Error calling enclave at %s:%d (rv=0x%x)\n", __FILE__,  \
                                              __LINE__, ret);   \
        abort();                                                        \
 } }

    double* readData(char *fname, int len);
    void saveSecretKeyToTxt(const std::string& filename, const uint64_t* secretKey);
    void readSecretKey(const char *fname, uint64_t *dest);
    void readCiphertext(const char *fname, uint64_t dest[][N]);
    void outputResult(double read_byte[10], int cnt, int runcnt, double accr, std::string strfname, int len, int mode);

    // std::string generate_timestamp_filename(const std::string &prefix, const std::string &ext);
    void dump_result_csv_timestamp(const uint8_t *result, int finished_byte);

#ifdef __cplusplus
}
#endif
