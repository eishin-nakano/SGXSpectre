import os
import pandas as pd

# CSVファイルが保存されているディレクトリを指定
directory = 'output'  # CSVファイルのあるフォルダパスを指定
output_csv = 'combined_csv_file.csv'

# 結合するための空のリストを用意
all_data = []

# CSVファイルを一つずつ読み込み、リストに追加
for filename in sorted(os.listdir(directory)):  # ファイル名順にソート
    if filename.endswith('.csv'):
        file_path = os.path.join(directory, filename)
        # CSVをDataFrameとして読み込む
        df = pd.read_csv(file_path)
        # ファイル名を示す列を追加（オプション：どのファイルのデータかを明示）
        df['source_file'] = filename
        # 'source_file'列を最初に移動
        cols = ['source_file'] + [col for col in df.columns if col != 'source_file']
        df = df[cols]
        # データをリストに追加
        all_data.append(df)

# すべてのデータフレームを縦に結合
combined_df = pd.concat(all_data, ignore_index=True)

# 最終的なCSVファイルに保存
combined_df.to_csv(output_csv, index=False, encoding='utf-8-sig')
print(f'ファイルが {output_csv} に保存されました。')

