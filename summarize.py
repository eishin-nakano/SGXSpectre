import os
import re
import csv

# resultディレクトリのパス
result_dir = 'results'

# 新しいCSVファイルを作成するディレクトリ
output_dir = 'output'

# resultディレクトリが存在するか確認
if not os.path.exists(result_dir):
    print(f"ディレクトリ '{result_dir}' が見つかりません。")
else:
    # outputディレクトリが存在しない場合は作成
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    # resultディレクトリ内のファイル一覧を取得し、末尾2つの数字でソートする
    def extract_file_parts(file_name):
        # ファイル名の最初の英数字部分と末尾2つの数字を正規表現で抽出
        match = re.search(r'([a-zA-Z0-9_]+)_(\d+)_(\d+)_(\d+)$', file_name)
        if match:
            # ファイル名の共通部分、末尾から2番目の数字と最後の数字を返す
            return (match.group(1), int(match.group(2)), int(match.group(3)))
        return (None, float('inf'), float('inf'))  # マッチしない場合、最後に回す

    # ファイル名から必要な情報を抽出してソート
    files = sorted(os.listdir(result_dir), key=lambda f: extract_file_parts(f)[1:])

    # ファイルの共通部分ごとにグループ化する辞書
    grouped_files = {}
    read_bytel_files = []  # 'read_bytel' 名を含むファイルを保存するリスト
    
    # ファイル一覧を処理
    for file in files:
        # ファイル名の共通部分と末尾の数字を取得
        common_part, num, additional_num = extract_file_parts(file)
        if common_part:
            # 'read_bytel' を含むファイルは別途処理
            if 'read_bytel' in common_part:
                read_bytel_files.append(file)
            
            # 最後の数字（\2）でグループ化し、同じ共通部分か確認
            if common_part not in grouped_files:
                grouped_files[common_part] = {}
            if additional_num not in grouped_files[common_part]:
                grouped_files[common_part][additional_num] = []
            grouped_files[common_part][additional_num].append((num, file))
    
    # グループごとにCSVファイルを作成
    for common_part, files_group in grouped_files.items():
        for additional_num, files in files_group.items():
            # 同じグループのファイルを列ごとにまとめて1つのCSVに書き出す
            # ファイル名に数字部分をゼロパディングして2桁にする
            csv_file_name = os.path.join(output_dir, f"{common_part}_group_{str(additional_num).zfill(2)}.csv")
            with open(csv_file_name, 'w', newline='') as csv_file:
                writer = csv.writer(csv_file)
                
                # 各ファイルを処理
                file_contents = []
                for _, file in sorted(files, key=lambda x: x[0]):  # numでソート
                    file_path = os.path.join(result_dir, file)
                    with open(file_path, 'r') as f:
                        lines = [line.strip() for line in f.readlines()]
                        file_contents.append(lines)
                
                # 各ファイルの行を列としてまとめる
                max_lines = max(len(content) for content in file_contents)
                for i in range(max_lines):
                    row = []
                    for content in file_contents:
                        if i < len(content):
                            row.append(content[i])
                        else:
                            row.append('')  # 行が足りない場合は空白で埋める
                    writer.writerow(row)
                    
            print(f"CSVファイル '{csv_file_name}' を作成しました。")