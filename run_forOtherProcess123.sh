#!/bin/bash

# sgxspectreコマンドのベース
SGXSPECTRE_BASE_CMD="./sgxspectre"

# tasksetコマンドの変数
TASKSET_CMD="taskset -c 0"

# core1でrandを実行しつつ行う場合
RAND_CMD_1="taskset -c 1 ./rand"
RAND_CMD_2="taskset -c 2 ./rand"
RAND_CMD_3="taskset -c 3 ./rand"
RAND_CMD_4="taskset -c 4 ./rand"
RAND_CMD_5="taskset -c 5 ./rand"
RAND_CMD_6="taskset -c 6 ./rand"
RAND_CMD_7="taskset -c 7 ./rand"
RAND_CMD_8="taskset -c 8 ./rand"
RAND_CMD_9="taskset -c 9 ./rand"
RAND_CMD_10="taskset -c 10 ./rand"
RAND_CMD_11="taskset -c 11 ./rand"

# 実行したい引数の配列
ARGS_ARRAY=(1 2 3 5 10 15 20)

# average変数の値を指定 (必要に応じて設定)
average=10  # 例として10回実行

# 実行カウント初期化
run_count=0
count=0

# average回繰り返す
while [ $run_count -lt $average ]; do

    # SGXSPECTRE_CMDを1~10に変更してループ
    for sgx_cmd_suffix in 2; do

        SGXSPECTRE_CMD="$SGXSPECTRE_BASE_CMD $sgx_cmd_suffix"

        # 引数配列をループ
        for i in "${ARGS_ARRAY[@]}"; do
            count=$((count + 1))

            echo "Executing: $TASKSET_CMD $SGXSPECTRE_CMD $i 0"
            $TASKSET_CMD $SGXSPECTRE_CMD $i 0

            # 実行結果のチェック
            if [ $? -eq 0 ]; then
                echo "Execution with command $SGXSPECTRE_CMD and argument $i (count: $count) was successful."
            else
                echo "Execution with command $SGXSPECTRE_CMD and argument $i (count: $count) failed."
            fi
            echo ""

            # 乱数生成開始
            $RAND_CMD_1 &
            RANDOM_PID_1=$!

            echo "Executing: $SGXSPECTRE_CMD $i 1"
            $SGXSPECTRE_CMD $i 1

            kill $RANDOM_PID_1

            # 実行結果のチェック
            if [ $? -eq 0 ]; then
                echo "Execution with argument $i $count was successful."
            else
                echo "Execution with argument $i $count failed."
            fi
            echo ""

            # 乱数生成開始
            $RAND_CMD_1 &
            RANDOM_PID_1=$!
            $RAND_CMD_2 &
            RANDOM_PID_2=$!

            echo "Executing: $SGXSPECTRE_CMD $i 2"
            $SGXSPECTRE_CMD $i 2

            kill $RANDOM_PID_1
            kill $RANDOM_PID_2

            # 実行結果のチェック
            if [ $? -eq 0 ]; then
                echo "Execution with argument $i $count was successful."
            else
                echo "Execution with argument $i $count failed."
            fi
            echo ""

            # 乱数生成開始
            $RAND_CMD_1 &
            RANDOM_PID_1=$!
            $RAND_CMD_2 &
            RANDOM_PID_2=$!
            $RAND_CMD_3 &
            RANDOM_PID_3=$!

            echo "Executing: $SGXSPECTRE_CMD $i 3"
            $SGXSPECTRE_CMD $i 3

            kill $RANDOM_PID_1
            kill $RANDOM_PID_2
            kill $RANDOM_PID_3

            # 実行結果のチェック
            if [ $? -eq 0 ]; then
                echo "Execution with argument $i $count was successful."
            else
                echo "Execution with argument $i $count failed."
            fi
            echo ""

            # 乱数生成開始
            $RAND_CMD_1 &
            RANDOM_PID_1=$!
            $RAND_CMD_2 &
            RANDOM_PID_2=$!
            $RAND_CMD_3 &
            RANDOM_PID_3=$!
            $RAND_CMD_4 &
            RANDOM_PID_4=$!

            echo "Executing: $SGXSPECTRE_CMD $i 4"
            $SGXSPECTRE_CMD $i 4

            kill $RANDOM_PID_1
            kill $RANDOM_PID_2
            kill $RANDOM_PID_3
            kill $RANDOM_PID_4

            # 実行結果のチェック
            if [ $? -eq 0 ]; then
                echo "Execution with argument $i $count was successful."
            else
                echo "Execution with argument $i $count failed."
            fi
            echo ""

            # 乱数生成開始
            $RAND_CMD_1 &
            RANDOM_PID_1=$!
            $RAND_CMD_2 &
            RANDOM_PID_2=$!
            $RAND_CMD_3 &
            RANDOM_PID_3=$!
            $RAND_CMD_4 &
            RANDOM_PID_4=$!
            $RAND_CMD_5 &
            RANDOM_PID_5=$!

            echo "Executing: $SGXSPECTRE_CMD $i 5"
            $SGXSPECTRE_CMD $i 5

            kill $RANDOM_PID_1
            kill $RANDOM_PID_2
            kill $RANDOM_PID_3
            kill $RANDOM_PID_4
            kill $RANDOM_PID_5

            # 実行結果のチェック
            if [ $? -eq 0 ]; then
                echo "Execution with argument $i $count was successful."
            else
                echo "Execution with argument $i $count failed."
            fi
            echo "" 

            # 乱数生成開始
            $RAND_CMD_1 &
            RANDOM_PID_1=$!
            $RAND_CMD_2 &
            RANDOM_PID_2=$!
            $RAND_CMD_3 &
            RANDOM_PID_3=$!
            $RAND_CMD_4 &
            RANDOM_PID_4=$!
            $RAND_CMD_5 &
            RANDOM_PID_5=$!
            $RAND_CMD_6 &
            RANDOM_PID_6=$!

            echo "Executing: $SGXSPECTRE_CMD $i 6"
            $SGXSPECTRE_CMD $i 6

            kill $RANDOM_PID_1
            kill $RANDOM_PID_2
            kill $RANDOM_PID_3
            kill $RANDOM_PID_4
            kill $RANDOM_PID_5
            kill $RANDOM_PID_6

            # 実行結果のチェック
            if [ $? -eq 0 ]; then
                echo "Execution with argument $i $count was successful."
            else
                echo "Execution with argument $i $count failed."
            fi
            echo ""

            # 乱数生成開始
            $RAND_CMD_1 &
            RANDOM_PID_1=$!
            $RAND_CMD_2 &
            RANDOM_PID_2=$!
            $RAND_CMD_3 &
            RANDOM_PID_3=$!
            $RAND_CMD_4 &
            RANDOM_PID_4=$!
            $RAND_CMD_5 &
            RANDOM_PID_5=$!
            $RAND_CMD_6 &
            RANDOM_PID_6=$!
            $RAND_CMD_7 &
            RANDOM_PID_7=$!

            echo "Executing: $SGXSPECTRE_CMD $i 7"
            $SGXSPECTRE_CMD $i 7

            kill $RANDOM_PID_1
            kill $RANDOM_PID_2
            kill $RANDOM_PID_3
            kill $RANDOM_PID_4
            kill $RANDOM_PID_5
            kill $RANDOM_PID_6
            kill $RANDOM_PID_7

            # 実行結果のチェック
            if [ $? -eq 0 ]; then
                echo "Execution with argument $i $count was successful."
            else
                echo "Execution with argument $i $count failed."
            fi
            echo "" 

            # 乱数生成開始
            $RAND_CMD_1 &
            RANDOM_PID_1=$!
            $RAND_CMD_2 &
            RANDOM_PID_2=$!
            $RAND_CMD_3 &
            RANDOM_PID_3=$!
            $RAND_CMD_4 &
            RANDOM_PID_4=$!
            $RAND_CMD_5 &
            RANDOM_PID_5=$!
            $RAND_CMD_6 &
            RANDOM_PID_6=$!
            $RAND_CMD_7 &
            RANDOM_PID_7=$!
            $RAND_CMD_8 &
            RANDOM_PID_8=$!

            echo "Executing: $SGXSPECTRE_CMD $i 8"
            $SGXSPECTRE_CMD $i 8

            kill $RANDOM_PID_1
            kill $RANDOM_PID_2
            kill $RANDOM_PID_3
            kill $RANDOM_PID_4
            kill $RANDOM_PID_5
            kill $RANDOM_PID_6
            kill $RANDOM_PID_7
            kill $RANDOM_PID_8

            # 実行結果のチェック
            if [ $? -eq 0 ]; then
                echo "Execution with argument $i $count was successful."
            else
                echo "Execution with argument $i $count failed."
            fi
            echo "" 

            # 乱数生成開始
            $RAND_CMD_1 &
            RANDOM_PID_1=$!
            $RAND_CMD_2 &
            RANDOM_PID_2=$!
            $RAND_CMD_3 &
            RANDOM_PID_3=$!
            $RAND_CMD_4 &
            RANDOM_PID_4=$!
            $RAND_CMD_5 &
            RANDOM_PID_5=$!
            $RAND_CMD_6 &
            RANDOM_PID_6=$!
            $RAND_CMD_7 &
            RANDOM_PID_7=$!
            $RAND_CMD_8 &
            RANDOM_PID_8=$!
            $RAND_CMD_9 &
            RANDOM_PID_9=$!

            echo "Executing: $SGXSPECTRE_CMD $i 9"
            $SGXSPECTRE_CMD $i 9

            kill $RANDOM_PID_1
            kill $RANDOM_PID_2
            kill $RANDOM_PID_3
            kill $RANDOM_PID_4
            kill $RANDOM_PID_5
            kill $RANDOM_PID_6
            kill $RANDOM_PID_7
            kill $RANDOM_PID_8
            kill $RANDOM_PID_9

            # 実行結果のチェック
            if [ $? -eq 0 ]; then
                echo "Execution with argument $i $count was successful."
            else
                echo "Execution with argument $i $count failed."
            fi
            echo ""

            # 乱数生成開始
            $RAND_CMD_1 &
            RANDOM_PID_1=$!
            $RAND_CMD_2 &
            RANDOM_PID_2=$!
            $RAND_CMD_3 &
            RANDOM_PID_3=$!
            $RAND_CMD_4 &
            RANDOM_PID_4=$!
            $RAND_CMD_5 &
            RANDOM_PID_5=$!
            $RAND_CMD_6 &
            RANDOM_PID_6=$!
            $RAND_CMD_7 &
            RANDOM_PID_7=$!
            $RAND_CMD_8 &
            RANDOM_PID_8=$!
            $RAND_CMD_9 &
            RANDOM_PID_9=$!
            $RAND_CMD_10 &
            RANDOM_PID_10=$!

            echo "Executing: $SGXSPECTRE_CMD $i 10"
            $SGXSPECTRE_CMD $i 10

            kill $RANDOM_PID_1
            kill $RANDOM_PID_2
            kill $RANDOM_PID_3
            kill $RANDOM_PID_4
            kill $RANDOM_PID_5
            kill $RANDOM_PID_6
            kill $RANDOM_PID_7
            kill $RANDOM_PID_8
            kill $RANDOM_PID_9
            kill $RANDOM_PID_10

            # 実行結果のチェック
            if [ $? -eq 0 ]; then
                echo "Execution with argument $i $count was successful."
            else
                echo "Execution with argument $i $count failed."
            fi
            echo ""

            # 乱数生成開始
            $RAND_CMD_1 &
            RANDOM_PID_1=$!
            $RAND_CMD_2 &
            RANDOM_PID_2=$!
            $RAND_CMD_3 &
            RANDOM_PID_3=$!
            $RAND_CMD_4 &
            RANDOM_PID_4=$!
            $RAND_CMD_5 &
            RANDOM_PID_5=$!
            $RAND_CMD_6 &
            RANDOM_PID_6=$!
            $RAND_CMD_7 &
            RANDOM_PID_7=$!
            $RAND_CMD_8 &
            RANDOM_PID_8=$!
            $RAND_CMD_9 &
            RANDOM_PID_9=$!
            $RAND_CMD_10 &
            RANDOM_PID_10=$!
            $RAND_CMD_11 &
            RANDOM_PID_11=$!

            echo "Executing: $SGXSPECTRE_CMD $i 11"
            $SGXSPECTRE_CMD $i 11

            kill $RANDOM_PID_1
            kill $RANDOM_PID_2
            kill $RANDOM_PID_3
            kill $RANDOM_PID_4
            kill $RANDOM_PID_5
            kill $RANDOM_PID_6
            kill $RANDOM_PID_7
            kill $RANDOM_PID_8
            kill $RANDOM_PID_9
            kill $RANDOM_PID_10
            kill $RANDOM_PID_11

            # 実行結果のチェック
            if [ $? -eq 0 ]; then
                echo "Execution with argument $i $count was successful."
            else
                echo "Execution with argument $i $count failed."
            fi
            echo ""

            # カレントディレクトリにある "results" フォルダを確認
            if [ ! -d "./results" ]; then
                echo "Error: 'results' フォルダが存在しません。"
                exit 1
            fi

            # result_all フォルダを作成（存在しない場合）
            mkdir -p result_all

            # result_all/result1, result2, ... を探して連番を決定
            counter=1
            while [ -e "./result_all/result$counter" ]; do  # -d から -e に変更（フォルダやファイル両方を確認）
                counter=$((counter + 1))
            done

            # フォルダ名を決定してコピー
            target_dir="./result_all/result$counter"
            cp -r ./results "$target_dir"

            # コピー後に "results" フォルダ内を完全に削除
            rm -rf ./results/*  # 隠しファイルやサブフォルダも削除

        done

    done

    # 実行回数をカウントアップ
    run_count=$((run_count + 1))
done
