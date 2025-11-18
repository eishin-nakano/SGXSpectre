import os
import shutil

# フォルダのパス設定
source_root = "result_all"
destination_folder = "results"

# 結果フォルダの作成
if not os.path.exists(destination_folder):
    os.makedirs(destination_folder)

# ファイル名のカウント用辞書
file_counts = {}

# サブフォルダを走査
for folder_name in os.listdir(source_root):
    folder_path = os.path.join(source_root, folder_name)
    
    # フォルダかどうか確認
    if os.path.isdir(folder_path):
        # サブフォルダ内のファイルを走査
        for file_name in os.listdir(folder_path):
            file_path = os.path.join(folder_path, file_name)
            
            # ファイルかどうか確認
            if os.path.isfile(file_path):
                # ファイル名の基礎部分を取得
                base_name, ext = os.path.splitext(file_name)
                
                # ファイル名のカウントを更新
                if file_name not in file_counts:
                    file_counts[file_name] = 0
                else:
                    file_counts[file_name] += 1
                
                # 新しいファイル名を作成
                new_file_name = f"{base_name}_{file_counts[file_name]}{ext}"
                destination_path = os.path.join(destination_folder, new_file_name)
                
                # ファイルをコピー
                shutil.copy(file_path, destination_path)

# 元のフォルダを削除
shutil.rmtree(source_root)

print("すべてのファイルをまとめました！")
