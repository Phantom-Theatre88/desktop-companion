# M5Stack Desktop Companion — 感覚野・脳機能配置 正式設計

更新日: 2026-09-11

## 0. この文書の位置づけ

この文書は、

- `M5Stack Desktop Companion 設計思想 v0.1.md`
- `M5Stack Desktop Companion 実装Step聖典 v0.3.md`
- `PROJECT_LOCKS.md`

を前提に、Desk Botの感覚器官・感覚野・反射・Heart Engine・Pi5側高次認知の役割分担を明文化する正本である。

最上位ゴールは、

**机の上に常にいて、周囲を感じ、反応し、気分が変わり、少しずつ関係性が育つ相棒**

として成立させること。

## 1. 実装順の上位LOCK

本書は最終的な感覚配置を定義するが、実装開始は必ずゼロベース基盤から行う。

先に、

1. CoreS3 + Arduino + M5Unified + M5GFXの新規プロジェクト
2. DISPLAY
3. TOUCH
4. IMU
5. PROXIMITY
6. MIC
7. SPEAKER
8. CAMERA

を公式環境だけで単独確認する。

その後に、

**Hardware → Device Driver → Adapter → Nerve Input → 感覚野／反射／Heart**

へ進む。

## 2. M5Stackは身体と低次脳を持つ

M5Stackは単なるセンサー端末や表示端末にしない。

M5Stack側に、

- 感覚器官の常時監視
- 低次感覚処理
- 反射
- Heart Engine
- 基本自発行動
- 表情
- 視線
- 首
- 基本生活リズム

を残す。

Pi5が停止しても「生きている感じ」を維持する。

## 3. Pi5は高次認知を担当する

Pi5は、

- 高度画像認識
- 人物同定
- 状況理解
- ASR
- LLM
- TTS
- 長期記憶
- 高次認知
- 複数感覚と過去記憶を使った意味判断

を担当する。

## 4. 視覚野

### M5Stack側：反射の目

- 人／顔の存在
- 顔や人の位置
- 動きの有無
- 単純ジェスチャー
- 視線・首を向けるための低次ターゲット

### Pi5側：理解する目

採用カメラ：**Raspberry Pi Camera Module 3 Wide**

- 誰なのか
- Kimかどうか
- 何を持っているか
- 何をしているか
- 机上物体・状況の意味
- 過去記憶との照合

原則：**M5で足りる低次認識はM5で終え、深い意味理解だけPi5へ渡す。**

## 5. 聴覚野

### M5Stack側

- 音がした／していない
- 大きな音
- 音声区間
- 呼びかけ開始のきっかけ

### Pi5側

- ASR
- 話者識別
- 発話内容・文脈理解
- LLM会話
- TTS

## 6. 体性感覚野

### 感覚器官

- タッチ
- IMU
- Unit ToF4M / U172
- 将来の圧力センサー等

### M5Stack側で扱う意味

- 触られた
- 撫でられた
- 揺らされた
- 持ち上げられた
- 傾けられた
- 近づいた／離れた
- 強い接触

生命感に直結するものはPi5を待たず即時反応する。

## 7. 在席・存在感覚

購入済み：**Unit TMOS PIR / U185**

人の在席、存在、活動をカメラとは独立した低次感覚として扱う。

ToF・TMOS・Visionを統合することで、人の存在・距離・位置・人物情報を相互補完する。

## 8. 環境感覚野

購入済み：**ENV-Pro / U169**

基本対象：

- 温度
- 湿度
- 気圧

将来候補：

- VOC
- CO2相当値
- IAQ

数値を直接Heartへ流さず、暑くなった、寒くなった、湿度が上がった、気圧が変化した等の意味へ変換する。

## 9. Heart Engineとの関係

Heart EngineはM5Stack側の常時稼働部分に置く。

Pi5停止やLAN断でも、mood、affection、curiosity、boredom、sleepiness、attention等を維持する。

感覚野からHeartへは製品固有値ではなく意味化されたNerve Inputを渡す。

## 10. 神経言語との関係

共通化するもの：

- 意味モデル
- 語彙
- ID
- payload定義

分けるもの：

- M5Stack内部の軽量イベント／API
- M5Stack ↔ Pi5間の通信・シリアライズ・再接続

原則：**語彙は共通、文法と輸送手段は別。**

## 11. 購入済み感覚・接続部品

- Unit ToF4M / U172
- Unit TMOS PIR / U185
- ENV-Pro / U169
- Unit Hub / U006
- Raspberry Pi Camera Module 3 Wide
- Raspberry Pi 5用SSD

## 12. 外部感覚の実装順

CoreS3素体確認完了後、

**ToF4M → TMOS PIR → ENV-Pro → 視覚 → タッチ → IMU → 聴覚 → その他感覚**

を基本とする。

## 13. 旧実装の扱い

旧Yuki／StackChan上のVision、センサー、ESP-IDFコードはLEGACY参考資産である。

現行の感覚野実装へそのまま継承しない。

既存OSSを使う場合も、ゼロベース基盤と独自境界が成立した後に適合性を評価する。

## 14. 現在の結論

Desk Botの最終構造は、

**感覚器官 → M5Stack低次感覚野 → 必要時Pi5高次感覚野 → 反射／Heart Engine／記憶／行動 → 身体**

という分散構造とする。

ただし実装は、最初にCoreS3そのものの正常性を確認し、その後に独自層を積み上げる。
