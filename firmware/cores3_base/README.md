# CoreS3 Base — Step 0

M5Stack Desktop Companion の新しい実装基準点。

## 目的

過去の Stack-chan 系・派生OSS・混在ライブラリ・旧初期化前提を一切引き継がず、M5Stack CoreS3 公式環境から独自実装を開始する。

## 固定する素体

- Hardware: M5Stack CoreS3
- Framework: Arduino
- Library: M5Unified
- Graphics: M5GFX
- Application: M5Stack Desktop Companion 独自コード

## このStepでは入れないもの

- Stack-chan本家
- AI_StackChan_Ex
- stack-chan-ko
- RoboEyes
- esp32-camera
- 外付けセンサー用ライブラリ
- Heart Engine
- Nerve / Semantic Neuron
- 表情・人格・会話

これらはStep 0の起動基準には含めない。

## 実機ゼロ化

新しい素体を書き込む前に、CoreS3のFlashを全消去する。

目的は旧Firmwareだけでなく、旧設定・NVS等の残留状態を新基準へ持ち込まないこと。

Flash全消去後に `cores3_base.ino` を書き込む。

## Step 0 完成条件

1. 旧Firmwareを前提にせずビルドできる。
2. CoreS3へ書き込める。
3. 起動後、画面に `CoreS3 BASE` と `STEP 0 / CLEAN START` が表示される。
4. Serial 115200で `[STEP0] CoreS3 clean base started` が確認できる。
5. Stack-chan系コード・RoboEyes・Camera・センサー・Heart等が起動に一切関与していない。

ここまで成功した状態を、Step 1「CoreS3素体検査」の基準点とする。
