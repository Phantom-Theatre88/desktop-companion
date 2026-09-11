# M5Stack Desktop Companion 神経系全体ブロック図 v0.1

更新日: 2026-09-11

本書は、Desktop Companionの感覚・反射・Heart Engine・記憶・Pi5高次認知・身体出力を一本の生命系として接続する全体構造の正本である。

上位基準：

- `M5Stack Desktop Companion 設計思想 v0.1.md`
- `M5Stack Desktop Companion 実装Step聖典 v0.3.md`
- `M5Stack Desktop Companion 感覚器官聖典 v0.1.md`
- `PROJECT_LOCKS.md`

## 1. 最上位ゴール

Desktop Companionを、

**「机の上に常にいて、周囲を感じ、反応し、気分が変わり、少しずつ関係性が育つ相棒」**

として成立させる。

神経系は単なる通信基盤ではなく、外界を感じ、その意味を知覚し、反射し、Heartが変化し、必要に応じてPi5で高次認知し、身体として行動するための生命系とする。

## 2. 神経系全体ブロック

```text
【外界】
   ↓
【感覚器官】
ToF / TMOS / ENV-Pro / Touch / IMU / Mic / M5 Camera / Pi5 Camera
   ↓
【Raw Perception】
距離・存在・温湿度・加速度・音量・顔位置…
   ↓
【Perception Integration】
timestamp / confidence / 複数感覚の統合
   ↓
【Nerve Input / Semantic Neuron】
PERSON_PRESENT / PROXIMITY_NEAR / HEAD_TOUCH / ROOM_HOT / LOUD_SOUND / FACE_DETECTED …
   ├────────→【Reflex Layer】
   ├────────→【Heart Engine / Ghost】
   ├────────→【Memory】
   └────────→【Pi5 高次認知】
                         ↓
                  Semantic Neuron
                         ↓
【Behavior Selector】
   ↓
【身体出力】
Eyes / Face / Neck / Servo / Voice / LED
```

## 3. 各層の責務

### 感覚器官

外界の物理現象を取得する。センサー単体で人格・感情・行動を決めない。

### Raw Perception

製品固有の生値を一次知覚として扱う。

### Perception Integration

`timestamp`、`confidence`、複数感覚の時間的・意味的整合を扱う。

### Nerve Input / Semantic Neuron

製品非依存の意味を神経系へ渡す。

生値をそのまま上位へ渡さない。

### Reflex Layer

Heart EngineやPi5の判断を待たず、即時反応する。

### Heart Engine / Ghost

意味化された出来事を、時間・記憶・関係性と合わせて内面へ反映する。

### Memory

短期、エピソード、長期関係記憶を扱う。

### Pi5高次認知

人物・物体・状況・音声・会話・記憶照合等の重い処理を担当する。結果は原則Semantic Neuronへ意味として戻す。

### Behavior Selector

反射、Heart、対人Session、Pi5等からの要求を仲裁する。

### 身体出力

Eyes / Face / Neck / Servo / Voice / LED等へ最終出力する。

## 4. M5Stack / Pi5の責務境界

### M5Stack

**身体＋生命維持できる低次脳＋反射系＋Heart Engineの常時稼働部分**。

Pi5停止時でも、感じる・見る・反射する・気分が変わる・基本自発行動を継続する。

### Raspberry Pi 5

**高次感覚野＋認知脳＋言語＋長期記憶**。

高次認知はM5側の生命活動を補強するが、存在そのものの必須条件にはしない。

## 5. 故障・切断時

一部センサー、Pi5、ネットワーク、カメラ等が失われても、Desk Bot全体を停止させない。

**「その感覚は使えないが、他の感覚とHeartは生きている」**状態として扱う。

## 6. 開発時ログ

感覚 → 意味化 → 神経配送 → Heart／反射 → 行動の因果を追跡可能にする。

## 7. ゼロベース実装順

この全体構造はLOCKするが、神経系コードを最初に作らない。

先に、

1. CoreS3 + Arduino + M5Unified + M5GFXの新規プロジェクトを作る
2. CoreS3公式APIだけでDISPLAY / TOUCH / IMU / PROXIMITY / MIC / SPEAKER / CAMERAを単独確認する
3. Hardware / Device Driver境界を作る
4. Adapter境界を作る
5. Nerve Input境界を作る

その後に、感覚1本を縦貫通実装する。

## 8. 最初の外部感覚縦貫通

最初の対象は **M5Stack Unit ToF4M / U172** とする。

ただし、CoreS3素体確認より先には進めない。

```text
ToF4M
↓
Device Driver
↓
Raw Perception
↓
Adapter / Perception Integration
↓
PROXIMITY_* Nerve Input
↓
Semantic Neuron
├→ Reflex
├→ Heart
└→ Pi5 cognition（必要時）
↓
Behavior Selector
↓
Body Output
```

ToF単独で人物を断定しない。

## 9. 感覚の実装順

CoreS3素体確認完了後、外部感覚は、

**ToF4M → TMOS PIR → ENV-Pro → 視覚 → タッチ → IMU → 聴覚 → その他感覚**

の順を基本とする。

各感覚を、生データ取得だけでなく **意味化 → 反射 → Heart → 必要時Pi5 → 行動** まで通す。

## 10. 神経入力の共通仕様 LOCK

基本構造：

```text
Device
↓
Device Driver
↓
Adapter / device-specific processing
↓
意味化・正規化
↓
Nerve Input
↓
Semantic Neuron
```

製品固有の距離値、座標、加速度、静電容量値等は前段情報とする。

上位へは `NEAR`、`APPROACH`、`LEAVE`、`STROKE_DETECTED`、`PICKED_UP`、`LOUD_SOUND` 等の意味を渡す。

カメラは例外的に、

```text
Camera
↓
Vision / Recognition
↓
意味抽出
↓
Nerve Input
```

とする。

## 11. OSSの扱い

神経系・Ghost・視覚等の先行OSSは、現行基盤の成立後に参考候補として調査してよい。

ただし、既存OSSを親Repoや初期基盤にしない。旧Yuki／StackChan実装へ新設計を合わせない。

必要機能を採用する場合は、現行アーキテクチャへ合わせて再評価・変換・再実装する。
