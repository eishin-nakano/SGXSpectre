#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ARRAY_SIZE 1000000  // 配列のサイズ

// 時間をナノ秒で計測する関数
long long get_time_in_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

int main() {
    // 配列を動的に割り当て
    int *array = (int *)malloc(ARRAY_SIZE * sizeof(int));
    if (array == NULL) {
        printf("メモリ割り当てに失敗しました\n");
        return -1;
    }

    // 配列を初期化
    for (int i = 0; i < ARRAY_SIZE; i++) {
        array[i] = i;
    }

    // 順次アクセスの時間を測定
    long long start_time = get_time_in_ns();
    int sum_seq = 0;
    for (int i = 0; i < ARRAY_SIZE; i++) {
        sum_seq += array[i];
    }
    long long end_time = get_time_in_ns();
    printf("順次アクセスの時間: %lld ns\n", end_time - start_time);

    // ランダムアクセスの時間を測定
    start_time = get_time_in_ns();
    int sum_rand = 0;
    for (int i = 0; i < ARRAY_SIZE; i++) {
        int index = rand() % ARRAY_SIZE;  // ランダムインデックス生成
        sum_rand += array[index];
    }
    end_time = get_time_in_ns();
    printf("ランダムアクセスの時間: %lld ns\n", end_time - start_time);

    // メモリ解放
    free(array);

    return 0;
}
