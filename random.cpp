#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ARRAY_SIZE (24 * 1024 * 1024 / sizeof(double))  // 12MB分の配列 (CPUのL3キャッシュサイズに合わせて変更)

// 配列全体を何度もアクセスする関数
void access_array(double *array) {
    for (size_t i = 0; i < ARRAY_SIZE; i++) {
        double index = rand();
        array[i*8%ARRAY_SIZE] += index;  // データにアクセスして変更
        // printf("%f\n", array[i*8%ARRAY_SIZE]);
    }
}

int main() {
    double *array = (double*)malloc(ARRAY_SIZE * sizeof(double));  // メモリに配列を確保
    if (array == NULL) {
        fprintf(stderr, "メモリの確保に失敗しました\n");
        return 1;
    }

    // 配列を初期化
    for (size_t i = 0; i < ARRAY_SIZE; i++) {
        array[i] = (double)rand() / RAND_MAX;
    }

    // 無限に配列をアクセスし続ける
    while (1) {
        access_array(array);
    }

    for(size_t i = 0; i < ARRAY_SIZE; i++){
        printf("%f", array[i]);
    }

    free(array);  // ここには到達しないが、メモリ解放を明示しておく
    return 0;
}
