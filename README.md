# M5Stack Desktop Companion

M5Stack CoreS3を、単なる音声AI端末ではなく、

**「机の上に常にいて、周囲を感じ、反応し、気分が変わり、少しずつ関係性が育つ相棒」**

として成立させるプロジェクトです。

## 現在の正式実装方針

2026-09-11から、実装基盤は**ゼロベース**へ正式変更しています。

初期構成は以下だけです。

- M5Stack CoreS3
- Arduino
- M5Unified
- M5GFX
- 本プロジェクト独自コード

既存Stack-chan系OSSを親Repoにはしません。

`stack-chan-ko`、M5Stack/StackChan、Dotty StackChan、stackchan-local、RoboEyes、Xiaozhi、旧Yuki実装等は、初期基盤には入れません。

## 現在地

- Step 0｜ゼロベース基準固定：**完了**
- Step 1｜CoreS3素体確認：**次に開始**

最初にM5Stackちゃんらしい機能は作らず、CoreS3公式環境だけでハードウェアを一つずつ確認します。

確認順：

1. DISPLAY
2. TOUCH
3. IMU
4. PROXIMITY
5. MIC
6. SPEAKER
7. CAMERA
8. その他必要ハード

その後、

**Hardware → Device Driver → Adapter → Nerve Input → M5Stackちゃん**

の順で独自構築します。

## 正本

設計・実装判断では、以下を正本とします。

1. `docs/sacred/M5Stack Desktop Companion 設計思想 v0.1.md`
2. `docs/sacred/M5Stack Desktop Companion 実装Step聖典 v0.3.md`
3. `docs/sacred/M5Stack Desktop Companion 感覚器官聖典 v0.1.md`
4. `docs/sacred/M5Stack Desktop Companion 神経系全体ブロック図 v0.1.md`
5. `docs/sacred/PROJECT_LOCKS.md`
6. `docs/status/CURRENT_STATUS.md`

## LEGACY参考資産

このリポジトリには、以前のYuki／StackChanベース開発のコード・patch・資料が残っています。

主な対象：

- `firmware/yuki/`
- `patches/`
- `upstream/stackchan/`
- 旧StackChanビルド用スクリプト
- 過去のBuild Week資料・スライド・DEVPOST

これらは**歴史資料・参考実装**として保持しています。

現行Desktop Companionの、

- 親Repo
- 初期基盤
- 現在のビルド手順
- 現在地
- 完成判定

には使用しません。

必要な機能を後から参考にする場合も、現行アーキテクチャへ適合させるための再評価を必須とします。

## 現在のビルド手順

ゼロベースCoreS3用の新規プロジェクトは、これからStep 1で作成します。

そのため、旧 `scripts/prepare-firmware.sh` やESP-IDF 5.5.4 / Xiaozhi / StackChan向け手順は、**現在のビルド手順ではありません**。

新基盤の実機確認手順が確立した時点で、この節を更新します。

## 最終判断基準

機能数ではなく、

**「そこにいる感じ」があるか。**

これをDesktop Companionの最終判断基準とします。
