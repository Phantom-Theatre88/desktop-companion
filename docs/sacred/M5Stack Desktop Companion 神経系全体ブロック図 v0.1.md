# M5Stack Desktop Companion 神経系全体ブロック図 v0.1

更新日: 2026-09-08

本書は、Desktop Companionの感覚・反射・Heart Engine・記憶・Pi5高次認知・身体出力を一本の生命系として接続するための全体構造を定義する正本である。

上位基準は、

- `M5Stack Desktop Companion 設計思想 v0.1.md`
- `M5Stack Desktop Companion 感覚器官聖典 v0.1.md`
- `PROJECT_LOCKS.md`

とする。

## 1. 最上位ゴールとの関係

Desktop Companionは、単なる音声AI端末ではなく、

**「机の上に常にいて、周囲を感じ、反応し、気分が変わり、少しずつ関係性が育つ相棒」**

として成立させる。

本神経系は、センサー値を集めるための通信基盤ではない。

**外界を感じ、その意味を知覚し、反射し、Heartが変化し、必要に応じてPi5で高次認知し、身体として行動するための生命系**として設計する。

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
【Semantic Neuron】
PERSON_PRESENT
PERSON_APPROACHING
HEAD_TOUCH
ROOM_HOT
LOUD_SOUND
FACE_DETECTED
…
   ├────────→【Reflex Layer】
   │             ↓
   │       目・首・表情・即時反応
   │
   ├────────→【Heart Engine / Ghost】
   │             ↓
   │   mood / affection / curiosity
   │   boredom / sleepiness / attention
   │
   ├────────→【Memory】
   │       短期 / エピソード / 長期
   │
   └────────→【Pi5 高次認知】
                 ↓
        人物・物体・状況・会話・LLM
                 ↓
           Semantic Neuron
                 ↓
【Behavior Selector】
   ↓
【身体出力】
Eyes / Face / Neck / Servo / Voice / LED
```

## 3. 各層の責務

### 3.1 感覚器官

外界の物理現象を取得する。

センサー単体で人格・感情・行動を決めてはならない。

### 3.2 Raw Perception

センサーの生データを、各感覚野が扱える一次知覚へ変換する。

例：

- ToF4M：mm単位距離
- TMOS：存在・動作系の検出値
- ENV-Pro：温度・湿度・気圧等
- IMU：加速度・角速度・姿勢
- Camera：顔位置・動き・低次視覚特徴

### 3.3 Perception Integration

複数感覚を、時間関係と確信度を含めて一つの出来事へ統合する層。

最低限、以下を設計要素として持つ。

- `timestamp`：いつ観測したか
- `confidence`：その知覚をどの程度信用できるか
- センサー間の時間的整合
- 同一対象・同一出来事として扱うための統合

例：

```text
ToF：接近
TMOS：人の存在
M5 Camera：顔あり
        ↓
PERSON_APPROACHING
```

各センサーが直接Heart Engineへ独立に作用し、同じ出来事を重複加算する構造は避ける。

### 3.4 Semantic Neuron

Desk Bot内で共有する「意味」を運ぶ神経語彙。

M5Stack内部とM5Stack ↔ Pi5間で実装方式は分けても、意味モデル・語彙・ID・payload定義は共有する。

原則は、

**語彙は共通、文法と輸送手段は別。**

### 3.5 Reflex Layer

即時反応を担当する。

反射はHeart EngineやPi5の判断完了を待たない。

例：急接近、強い揺れ、接触、突然の大きな音等。

原則：

**反射は先に身体を動かし、その出来事をHeart Engineへ後から伝える。**

### 3.6 Heart Engine / Ghost

Semantic Neuronから意味化された出来事を受け取り、時間・記憶・関係性によって内面を変化させる。

最低限、

- mood
- affection
- curiosity
- boredom
- sleepiness
- attention

を連続状態として扱う。

Ghostは、Heart Engineを中心に、感情・欲求・注意・時間変化・記憶影響・自発行動理由を持つ「内面」として今後設計する。

### 3.7 Memory

短期記憶、エピソード記憶、長期関係記憶を扱う。

記憶はHeartと高次認知の双方に影響するが、M5側の生命維持をPi5長期記憶へ依存させない。

### 3.8 Pi5 高次認知

人物・物体・状況・音声・会話・マルチモーダル文脈・記憶照合等、M5単体では重い高次処理を担当する。

Pi5の結果は、直接身体を支配するのではなく、原則としてSemantic Neuronへ意味として戻す。

### 3.9 Behavior Selector

反射、Heart、記憶、Pi5高次認知等から受けた要求を統合し、実際の行動へ変換する。

複数の感覚・認知が同時に身体制御権を奪い合わない構造とする。

### 3.10 身体出力

- Eyes
- Face
- Neck
- Servo
- Voice
- LED

等へ最終行動を出力する。

## 4. M5Stack / Pi5の責務境界

### M5Stack

**身体＋生命維持できる低次脳＋反射系＋Heart Engineの常時稼働部分**を担当する。

Pi5停止時でも、

- 感じる
- 見る
- 反射する
- 気分が変わる
- 基本的な自発行動をする

ことを継続する。

### Raspberry Pi 5

**高次感覚野＋認知脳＋言語＋長期記憶**を担当する。

高次認知はM5の生命活動を補強するが、存在そのものの必須条件にはしない。

## 5. 故障・切断時の原則

センサー1個、Pi5、ネットワーク、カメラ等の一部が失われても、Desk Bot全体が停止してはならない。

状態は、

**「その感覚が使えないが、他の感覚とHeartは生きている」**

として扱う。

Pi5切断時は高次認知のみ失い、M5側の低次感覚・反射・Heart・基本行動を継続する。

## 6. 開発時ログ

感覚→意味化→Heart→行動の因果を追跡できるログを残す。

例：

```text
08:41:12 ToF 1800mm
08:41:13 TMOS presence=true
08:41:13 PERSON_APPROACHING confidence=0.82
08:41:14 attention 0.42 -> 0.61
08:41:14 LOOK_AT
```

目的は、行動がおかしいときに「なぜそうなったか」を追えるようにすること。

## 7. 実装方針

巨大なGhost／Neuronを一括実装しない。

最終構造を先に固定したうえで、**感覚器官1本を縦に貫通させて完成させる。**

基本形：

```text
Sensor
↓
Raw Perception
↓
Perception Integration
↓
Semantic Neuron
├→ Reflex
├→ Heart
└→ Pi5 cognition
↓
Behavior Selector
↓
Body Output
```

最初の縦貫通実証は **M5Stack Unit ToF4M / U172** とする。

## 8. ToF4Mへ進む前のLOCK

ToF4Mのコード実装へ入る前に、本ブロック図を全体構造として固定する。

その後、ToF4Mについて、

1. 何を感じる感覚なのか
2. 生データ
3. M5側前処理
4. timestamp / confidence
5. 意味化された知覚イベント／状態
6. 反射層への入力
7. Heart Engineへの意味
8. Pi5へ渡す条件と内容
9. Pi5から返る高次認知
10. Pi5切断時のフォールバック
11. Behavior Selectorとの接続
12. Eyes / Neck等の身体出力
13. ログと実機確認条件

までを実装仕様へ落とす。

## 9. 今後の順序

感覚野の設計順は、

**ToF4M → TMOS PIR → ENV-Pro → 視覚 → タッチ → IMU → 聴覚 → その他感覚**

とする。

各感覚を「生データ → 意味化 → 反射 → Heart → Pi5 → 行動」まで定義した後、その共通構造を基に、

- Ghost：Heart Engineを中心とする内面
- Neuron：感覚・反射・Heart・記憶・Pi5・身体を結ぶ神経系

を詳細設計する。

Ghost／Neuronはゼロからすべて発明せず、GitHub上のOSS・先行実装を調査し、`PROJECT_LOCKS.md` の適合手術ルールに従って採用・変換・再実装する。
