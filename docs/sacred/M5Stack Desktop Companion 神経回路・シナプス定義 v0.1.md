# M5Stack Desktop Companion 神経回路・シナプス定義 v0.1

更新日: 2026-09-11

本書は、M5Stack Desktop Companionにおける **神経回路（Neural Circuit）／Neuron／Synapse** の用語と責務境界を正式LOCKする聖典である。

上位基準は以下とする。

- `M5Stack Desktop Companion 設計思想 v0.1.md`
- `M5Stack Desktop Companion 実装Step聖典 v0.3.md`
- `M5Stack Desktop Companion 神経系全体ブロック図 v0.1.md`
- `PROJECT_LOCKS.md`

## 1. 基本定義

### 神経回路 / Neural Circuit

**感覚器官 → 知覚 → 意味化 → 反射／Heart／記憶／Pi5高次認知 → 行動選択 → 身体出力**

を一つの生命系として結ぶ全体網を神経回路と呼ぶ。

### Neuron / Semantic Neuron

Desk Bot内部で共有する、**意味化された出来事・状態・要求を運ぶ神経線／神経語彙**をNeuronと呼ぶ。

例：

- `PERSON_PRESENT`
- `PERSON_APPROACHING`
- `HEAD_TOUCH`
- `LOUD_SOUND`
- `FACE_DETECTED`
- `LOOK_AT`

製品固有の生値はNeuronそのものではない。

### Synapse / シナプス

Neuronが運ぶ意味を、**どの受け手へ、どの条件で接続するかを定める接続点／接続規則**をSynapseと呼ぶ。

## 2. 三者の関係

**Neuron＝意味を運ぶ**  
**Synapse＝意味を必要な相手へ結ぶ**  
**Neural Circuit＝NeuronとSynapseによって感覚・心・認知・身体を結ぶ全体網**

基本構造：

```text
外界
↓
感覚器官
↓
Raw Perception
↓
Perception Integration
↓
Nerve Input
↓
Semantic Neuron
↓
Synapse
├→ Reflex Layer
├→ Heart Engine / Ghost
├→ Memory
└→ Pi5 高次認知
      ↓
   Semantic Neuron
      ↓
   Synapse
      ↓
Behavior Selector
↓
身体出力
```

## 3. Synapseの責務境界

Synapseは、

- どのNeuronを受けるか
- どの層へ接続するか
- 接続条件
- 複数受け手への分岐
- 必要時の受け手向け意味変換

を担当する。

感情はHeart Engine、高次認知はPi5、即時反応はReflex Layer、身体競合はBehavior Selectorが担当する。

## 4. 生物学的シナプスとの違い

現段階では以下を必須にしない。

- 学習による結合強度変化
- 発火閾値
- 可塑性
- 強化学習
- 時間依存重み
- ニューラルネットワーク学習

**「シナプス」という比喩で実装を不要に複雑化しない。**

## 5. 実装開始条件

神経回路の実装は、ゼロベース基盤のCoreS3素体確認より先に開始しない。

先に、

1. CoreS3公式環境でハードウェア単体確認
2. Device Driver層の成立
3. Adapter層の成立
4. Nerve Inputの共通境界確認

を行う。

その後、最初の外部感覚縦貫通として **M5Stack Unit ToF4M / U172** を使う。

概念構造：

```text
ToF4M
↓
Device Driver
↓
Adapter / Perception
↓
PROXIMITY_* Nerve Input
↓
Semantic Neuron
↓
Synapse
├→ Reflex
├→ Heart
└→ Pi5（必要時）
↓
Behavior Selector
↓
Eyes / Neck 等
```

ToF単独で人物を断定しない。

## 6. 実装原則

神経回路全体を一括実装しない。

ゼロベース基盤と層境界を先に成立させ、その後、感覚器官1本を縦に通す。

旧Yuki／StackChan上に作成されたNeuron／SynapseコードはLEGACY参考資産であり、新基盤へそのまま継承しない。

必要になった時に、現行構造へ合わせて再設計・再実装する。

## 7. LOCK

1. 神経回路を感覚から身体までを結ぶ生命情報網の正式呼称とする。
2. Neuron / Semantic Neuronを共通意味を運ぶ神経語彙として扱う。
3. Synapseを意味を必要な層へ接続・分岐する接続点／規則とする。
4. Synapseは現段階で学習・可塑性・重み付きニューラルネットを意味しない。
5. CoreS3素体確認と独自層境界を先に成立させる。
6. その後、ToF4Mから感覚1本ずつ縦貫通実装する。
7. 旧Yuki／StackChan用神経コードはLEGACY扱いとする。
