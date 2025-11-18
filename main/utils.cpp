#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <chrono>
#include <unordered_map>
#include <stack>
#include <string>
#include <seal/seal.h>
#include "utils.hpp"
#include <fstream>
#include <iostream>
#include <chrono>
#include <ctime>
#include <sys/stat.h>
#include <sys/types.h>

using namespace std;

void saveSecretKeyToTxt(const std::string& filename, const uint64_t* secretKey) {
    std::ofstream ofs(filename);
    if (!ofs) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return;
    }

    for (size_t i = 0; i < N; i++) {
        ofs << secretKey[i];
        if (i < N - 1)
            ofs << " ";
    }
    ofs << "\n";
    ofs.close();
}

void readSecretKey(const char *fname, uint64_t *dest) {

    std::ifstream spar(std::to_string(N) + "/parms", std::ios::in | std::ios::out | std::ios::binary);
    std::ifstream ssk(fname, std::ios::in | std::ios::out | std::ios::binary);

    seal::EncryptionParameters parms;
    parms.load(spar);
    shared_ptr<seal::SEALContext> context(new seal::SEALContext(parms));
    std::cout << "loaded parameters. N = " << parms.poly_modulus_degree() << std::endl;

    /// Load seal keys 
    cout << "Loading secretkey..." << endl;
    shared_ptr<seal::SecretKey> sk(new seal::SecretKey);
    sk->load(*context, ssk);
    std::cout << "loaded secret_key" << endl;

    for (size_t i = 0; i < N; ++i) dest[i] = sk->data().dyn_array()[i];
}

void readCiphertext(const char *fname, uint64_t dest[][N]) {
    std::ifstream scp(fname, std::ios::in | std::ios::out | std::ios::binary);
    std::ifstream spar(std::to_string(N) + "/parms", std::ios::in | std::ios::out | std::ios::binary);

    seal::EncryptionParameters parms;
    parms.load(spar);
    shared_ptr<seal::SEALContext> context(new seal::SEALContext(parms));

    cout << "Loading ciphertext..." << endl;
    shared_ptr<seal::Ciphertext> cp(new seal::Ciphertext);
    cp->load(*context, scp);
    std::cout << "loaded ciphertext" << endl;

    for (size_t i = 0; i < N; ++i) {
        dest[0][i] = cp->data(0)[i];
        dest[1][i] = cp->data(1)[i];
    }
}

void outputResult(double *read_byte, int cnt, int runcnt, double accr, string strfname, int len, int mode) {
    //string strfname(fname);
    string str = "results/"+strfname+"_";
    str += to_string(cnt)+"_"+to_string(runcnt);
    ofstream outputfile(str);
    if(mode == 1) {
        for(int i = 0; i < len; i++) {
            outputfile << read_byte[i+1] - read_byte[i] << "\n";
        }
    }else if(mode == 2) {
        for(int i = 0; i < len; i++) {
            outputfile << read_byte[i] << "\n";
        }
    }else{
        printf("no matching\n");
    }
    outputfile << accr << "\n";
    outputfile.close();
}

static void ensure_directory(const std::string &path) {
    struct stat st{};
    if (stat(path.c_str(), &st) == -1) {
        mkdir(path.c_str(), 0755);
    }
}

// std::string generate_timestamp_filename(const std::string &prefix,
//                                         const std::string &ext) {
//     using namespace std::chrono;

//     auto now = system_clock::now();
//     auto ms  = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

//     std::time_t t = system_clock::to_time_t(now);
//     std::tm local_tm = *std::localtime(&t);

//     char buf[256];
//     std::sprintf(
//         buf,
//         "%s_%04d%02d%02d_%02d%02d%02d_%03lld.%s",
//         prefix.c_str(),
//         local_tm.tm_year + 1900,
//         local_tm.tm_mon + 1,
//         local_tm.tm_mday,
//         local_tm.tm_hour,
//         local_tm.tm_min,
//         local_tm.tm_sec,
//         static_cast<long long>(ms.count()),
//         ext.c_str()
//     );

//     return std::string(buf);
// }

// void dump_result_csv_timestamp(const uint8_t *result, int finished_byte) {
//     // 出力ディレクトリ
//     std::string dir = "results/leaked_key";

//     // ディレクトリ作成（存在していてもOK）
//     ensure_directory("results");
//     ensure_directory(dir);

//     // ファイル名を生成
//     std::string filename =
//         dir + "/" + generate_timestamp_filename("leaked_key", "csv");

//     FILE *fp = std::fopen(filename.c_str(), "w");
//     if (!fp) {
//         std::perror("fopen");
//         return;
//     }

//     // CSV header
//     std::fprintf(fp, "index,guessed_byte\n");

//     for (int i = 0; i < finished_byte; i++) {
//         std::fprintf(fp, "%d,%u\n", i, (unsigned int)result[i]);
//     }

//     std::fclose(fp);
//     std::printf("Saved CSV: %s\n", filename.c_str());
// }
