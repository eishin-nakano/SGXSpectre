#!/bin/bash

# sgxspectreコマンドのベース
SGXSPECTRE_BASE_CMD="./sgxspectre"

# tasksetコマンドの変数
TASKSET_CMD="taskset -c 0"

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
