# トラブルシューティングガイド

## Homebrew権限エラーの解決

### 問題の症状

```
cp: utimensat: /opt/homebrew/Cellar/xxx: Permission denied
cp: chmod: /opt/homebrew/Cellar/xxx: Operation not permitted
Error: Failure while executing...
```

### 原因

`/opt/homebrew` ディレクトリの所有者が `root` になっているため、一般ユーザーでは書き込みができない。

### 解決方法

#### 方法1: 所有者を変更（推奨）

**ターミナルで以下を実行:**

```bash
# Homebrewディレクトリの所有者を現在のユーザーに変更
sudo chown -R $(whoami):admin /opt/homebrew
```

**パスワード入力:**
- Macのログインパスワードを入力
- 入力中は画面に表示されません（セキュリティ機能）
- Enterキーを押す

**確認:**

```bash
ls -ld /opt/homebrew /opt/homebrew/Cellar
```

正常な出力例:
```
drwxrwxr-x  37 wataruhomma  admin  1184 Nov 16 17:42 /opt/homebrew
drwxrwxr-x  67 wataruhomma  admin  2144 Nov 16 19:18 /opt/homebrew/Cellar
```

最初の名前が自分のユーザー名（`wataruhomma`）になっていればOK。

**OpenCVを再インストール:**

```bash
brew install opencv
```

---

#### 方法2: Homebrewを完全に再インストール

方法1で解決しない場合：

**1. 既存のHomebrewをアンインストール:**

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/uninstall.sh)"
```

**2. Homebrewを再インストール:**

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

**3. 必要なパッケージを再インストール:**

```bash
brew install cmake opencv boost nlohmann-json spdlog
```

---

## よくある質問

### Q1: `sudo`コマンドとは？

**A:** 管理者権限で実行するコマンドです。セキュリティ上、パスワードが必要です。

### Q2: パスワードを入力しても何も表示されない

**A:** セキュリティ機能です。入力は正常に受け付けられています。そのまま入力してEnterを押してください。

### Q3: `Permission denied` が出続ける

**A:** 以下を確認してください：

1. **正しいパスワードを入力したか**
   ```bash
   # 別の方法で確認
   sudo echo "test"
   ```
   「test」と表示されればsudoは正常に動作しています。

2. **管理者権限があるユーザーか**
   ```bash
   groups
   ```
   出力に `admin` が含まれていることを確認。

3. **ディスクに十分な空き容量があるか**
   ```bash
   df -h /opt/homebrew
   ```

### Q4: `brew install opencv`が途中で止まる

**A:** 大量の依存ライブラリをダウンロード・インストールするため、時間がかかります（5〜15分程度）。

進行中であれば、待ちます。完全に止まった場合：

```bash
# Ctrl+C で中断
# 再度実行
brew install opencv
```

### Q5: OpenCVのインストールが何度も失敗する

**A:** 以下を試してください：

```bash
# Homebrewのクリーンアップ
brew cleanup

# キャッシュをクリア
rm -rf $(brew --cache)

# 再度インストール
brew install opencv
```

---

## その他のエラー

### CMakeが見つからない

```bash
brew install cmake
```

### pkg-configでOpenCVが見つからない

```bash
# OpenCVのパスを確認
brew info opencv

# 環境変数を設定
export PKG_CONFIG_PATH="/opt/homebrew/opt/opencv/lib/pkgconfig:$PKG_CONFIG_PATH"
```

### ビルド時にヘッダーファイルが見つからない

CMakeLists.txtで明示的にパスを指定：

```bash
cd build
cmake -DOpenCV_DIR=/opt/homebrew/opt/opencv/lib/cmake/opencv4 ..
```

---

## サポート連絡先

それでも解決しない場合は、以下の情報を添えて報告してください：

1. エラーメッセージ全文
2. 実行したコマンド
3. 以下のコマンド出力

```bash
# システム情報
uname -a
sw_vers

# Homebrew情報
brew --version
brew config

# ディレクトリ権限
ls -ld /opt/homebrew /opt/homebrew/Cellar

# インストール済みパッケージ
brew list
```
