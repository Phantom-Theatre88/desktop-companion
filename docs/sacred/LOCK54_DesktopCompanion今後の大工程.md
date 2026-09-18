# LOCK 54｜Desktop Companion 今後の大工程

更新日: 2026-09-18

## 決定

現在の実装状態を基準に、Desktop Companion v1.0へ向かう今後の大工程を以下の順で固定する。

### 直近小ゴール

**Vision → Semantic Neuron接続の実機確認**

すでに実装済みの、

**Camera → Vision → Semantic Neuron → Ghost / Memory / Behavior**

をCoreS3実機で通す。

確認対象は少なくとも以下とする。

- Runtime READY
- Vision由来の `MOTION_DETECTED / BRIGHTER / DARKER` がSemantic Neuronとして発生する
- Ghost / Memory / Behaviorまで配送される
- Camera使用後も内部I2Cが復元される
- Touch / IMU / Face / DeskRobo生命ループが継続する
- 静止時の誤発火、照明変化、動体、自己運動による誤検知を実機で確認する

この実機確認を通過してから、以下の大工程へ進む。

## A｜「感じる」を完成させる

外界を感じる感覚器官を、製品固有の生値ではなくSemantic Neuronまで意味化してGhostへ接続する。

基本順は既存LOCKに従い、

**CoreS3内蔵カメラ低次視覚 → ToF4M → TMOS PIR → ENV-Pro → 聴覚 → その他感覚**

とする。

Touch / IMUは既に先行接続済みとして扱う。

完成判定は、センサー値が読めることではなく、Ghostが意味として外界を受け取れること。

## B｜「気分が変わる」を完成させる

既に存在するHeart Engine本番骨格へ、イベントと時間による実際のHeart変化を接続する。

対象は、

- mood
- affection
- curiosity
- boredom
- sleepiness
- attention

とする。

Semantic Neuron、Reflex結果、時間経過、Relationship、Memory等をHeart変化へ接続し、

- 撫でられて少し嬉しくなる
- 放置で退屈する
- 驚いた後に平常へ戻る
- 夜に眠気が増える

等の内面変化を成立させる。

具体的な係数・閾値は、実装上必要になった段階で決める。

## C｜「自分から行動する」を完成させる

Heart / Memory / Time / 現在の感覚を材料に、理由のある行動選択を成立させる。

対象は、

- 視線
- 瞬き
- Face
- 内蔵2軸首サーボ
- 小さな身体表現
- 必要に応じた声
- 何もしない

を含む。

MicroBehaviorを生命維持動作だけで終わらせず、Ghostの状態から自発行動へ発展させる。

「何もしない」も正式な行動とする。

## D｜「昨日と今日をつなぐ」

Time / Memory / Relationshipを実際に機能させ、現在の反応へ過去を接続する。

対象は、

- 直前の出来事
- 最近の反復傾向
- 特別な出来事
- 不在時間
- よく一緒にいる時間帯
- affection等の長期関係変化
- 動的なHeart平常値
- 起動をまたぐ状態復元

とする。

これにより、毎日の接触で反応が少しずつ変わる状態を成立させる。

## E｜「賢くする」

M5Stack側で生命活動が成立した後、Raspberry Pi 5を高次認知系として接続する。

Pi5側の主な役割は、

- Raspberry Pi Camera Module 3 Wideによる高次Vision
- ASR
- LLM
- TTS
- 高度なMemory
- 人物・物体・状況理解

とする。

Pi5やLLMをGhostそのものにはしない。

Pi5停止時でも、M5Stack側の感覚・反射・Heart・基本行動・生命感は継続する。

## 全体順序

**直近実機確認 → A「感じる」→ B「気分が変わる」→ C「自分から行動する」→ D「昨日と今日をつなぐ」→ E「賢くする」**

とする。

このLOCKは既存の実装Step番号を置き換えるものではない。

既存Stepを、最上位ゴールから見た大工程へ束ね直した上位ロードマップとして扱う。

## 最終判断基準

最終ゴールは機能数ではなく、

**「机の上に常にいて、周囲を感じ、反応し、気分が変わり、少しずつ関係性が育つ相棒」**

として成立し、**「そこにいる感じ」があること**。
