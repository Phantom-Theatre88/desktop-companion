# M5Stack Desktop Companion 実装Step聖典 v0.3

更新日: 2026-09-11

## 0. この文書の位置づけ

本書は `M5Stack Desktop Companion 設計思想 v0.1.md` を実装へ落とすための作業順序の正本である。

最上位ゴールは、

**「机の上に常にいて、周囲を感じ、反応し、気分が変わり、少しずつ関係性が育つ相棒」**

として成立させること。

## Step 0｜ゼロベース基準固定

### 基盤

初期構成は以下だけとする。

- M5Stack CoreS3
- Arduino
- M5Unified
- M5GFX
- 独自コード

親Repoは置かない。

`stack-chan-ko`、M5Stack/StackChan、Dotty StackChan、stackchan-local、M5Stack_RoboEyes、Xiaozhi、旧Yuki実装等は初期基盤へ入れない。

### 旧資産

既存の `firmware/yuki/`、`patches/`、`upstream/`、旧ビルドスクリプト等はLEGACY参考資産とする。

削除はしないが、現行実装の正本・親Repo・動作基準にはしない。

## Step 1｜CoreS3素体確認

M5Stackちゃんらしい機能は作らない。

CoreS3公式環境だけで各ハード機能を単独確認する。

基本順：

1. DISPLAY
2. TOUCH
3. IMU
4. PROXIMITY
5. MIC
6. SPEAKER
7. CAMERA
8. その他必要ハード

各項目は、対象機能だけを最小コードで確認し、正常／異常を明確にする。

### 完成条件

CoreS3本体と公式ライブラリ層の正常性が、旧OSSなしで確認できていること。

## Step 2｜独自ハードウェア層

確認済みのハード機能を、Desktop Companion独自構造へ整理する。

基本構造：

**Hardware → Device Driver → Adapter → Nerve Input**

製品固有差はDevice Driver / Adapter側で吸収する。

## Step 3｜神経入力の共通化

生値をそのままHeartや行動へ流さない。

例：

- 距離値 → NEAR / APPROACH / LEAVE
- タッチ値 → TOUCH / STROKE_DETECTED
- IMU値 → PICKED_UP / SHAKE

カメラは、

**Camera → Vision / Recognition → Nerve Input**

とする。

## Step 4｜最初の外部感覚縦貫通

最初の対象は Unit ToF4M / U172 とする。

ただしStep 1〜3を飛ばして開始しない。

**Device → Adapter → Nerve Input → Reflex / Heart → Behavior → Body Output**

まで一本通す。

## Step 5｜感覚器官を順次追加

基本順は、

**ToF4M → TMOS PIR → ENV-Pro → 視覚 → タッチ → IMU → 聴覚 → その他感覚**

とする。

各感覚は生データ取得だけで完了にせず、意味化・反射・Heart・行動まで接続する。

## Step 6｜反射層

LLMやPi5を待たずに即時反応する身体反射を作る。

安全・強い反射を最優先とする。

## Step 7｜Heart Engine v0.1

初期パラメータ：

- mood
- affection
- curiosity
- boredom
- sleepiness
- attention

イベントだけでなく時間でも変化する。

## Step 8｜表情・身体表現

固定表情画像の切替を中心にせず、目、視線、瞬き、左右差、首、声等を連続パラメータとして制御する。

## Step 9｜Behavior Selectorと自発行動

優先順位：

1. 安全・強い反射
2. 意味のある対人反応
3. Heart由来の状態表現
4. 自発行動
5. idle

「何もしない」も正式な行動とする。

## Step 10｜短期記憶と生活リズム

直前の出来事、最後にKimを見た時刻、会話、接触、時間帯等を現在の反応へつなげる。

## Step 11｜Raspberry Pi 5接続

M5Stack側の生命活動を保ったまま、高次感覚野・認知・言語・長期記憶へ接続する。

Pi5停止時でも、M5Stack側の基本生命活動を継続する。

## Step 12｜ローカル会話

ASR、LLM、TTSを高次系へ接続し、Heart状態と会話を相互反映する。

LLMを心そのものにはしない。

## Step 13｜長期記憶

エピソード記憶と長期関係記憶を実装し、Heart・会話・行動へ反映する。

## Step 14｜既存OSS再評価

ここまでの独自構造が成立した後でのみ、既存OSSを再評価する。

必要な機能がある場合は、

**構造差分確認 → 適合設計 → 変換／再実装 → 実機確認 → 衝突確認**

を行う。

既存OSSへ本体設計を合わせない。

## Step 15｜Desktop Companion v1.0

完成条件：

- 話しかけなくても存在感がある
- Kimが来ると気づく
- 必要に応じて見る
- 撫でると反応する
- 放置で退屈する
- 夜に眠くなる
- 自分から小さく行動する
- ローカルで会話できる
- 前日の出来事が一部残る
- 日々の接触で反応が少しずつ変わる
- インターネットやPi5が落ちてもM5Stack側の生命活動が続く

## デバッグLOCK

`実行 → 修正 → 再実行` で同じ問題または同系統の問題が1回でも再発した時点で、局所修正を止め、根本原因解析へ切り替える。

## 最終判断基準

機能数ではなく、**「そこにいる感じ」があるか**。
