# M5Stack Desktop Companion 神経回路・シナプス定義 v0.1

更新日: 2026-09-10

本書は、M5Stack Desktop Companionにおける **神経回路（Neural Circuit）／Neuron／Synapse** の用語と責務境界を正式LOCKする聖典である。

上位基準は以下とする。

- `M5Stack Desktop Companion 設計思想 v0.1.md`
- `M5Stack Desktop Companion 神経系全体ブロック図 v0.1.md`
- `PROJECT_LOCKS.md`

## 1. LOCKする基本定義

### 神経回路 / Neural Circuit

Desktop Companionの、

**感覚器官 → 知覚 → 意味化 → 反射／Heart／記憶／Pi5高次認知 → 行動選択 → 身体出力**

を一つの生命系として結ぶ全体網を **神経回路** と呼ぶ。

神経回路は単なる通信バスやイベント配信機構ではない。

外界で起きた出来事が「この子にとって何を意味するか」に変換され、その意味が必要な内面・認知・反射・身体へ伝わるための構造全体を指す。

### Neuron / Semantic Neuron

Desk Bot内部で共有する、**意味化された出来事・状態・要求を運ぶ神経線／神経語彙**を Neuron と呼ぶ。

現在の聖典で定義済みの `Semantic Neuron` を、このNeuronの中核とする。

例：

- `PERSON_PRESENT`
- `PERSON_APPROACHING`
- `HEAD_TOUCH`
- `LOUD_SOUND`
- `FACE_DETECTED`
- `LOOK_AT`

製品固有の生値はNeuronそのものではない。

### Synapse / シナプス

Neuronが運ぶ意味を、**どの受け手へ、どの条件で、どの意味として接続するかを定める接続点／接続規則**を Synapse と呼ぶ。

Synapseは、Neuronイベントの配送先・分岐・接続条件を定義する。

例：

```text
PERSON_APPROACHING
   ↓ Synapse
   ├→ Reflex Layer：即時に対象へ注意を向ける
   ├→ Heart Engine：attention等へ意味を伝える
   └→ Pi5 cognition：必要時のみ人物認識へ送る
```

## 2. 三者の関係

要約すると、

**Neuron＝意味を運ぶ**  
**Synapse＝意味を必要な相手へ結ぶ**  
**Neural Circuit＝NeuronとSynapseによって感覚・心・認知・身体を結ぶ全体網**

とする。

設計図上では、

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

という理解を基本とする。

## 3. Synapseの責務境界

Synapseが担当するのは、原則として以下である。

- どのNeuron／意味イベントを受けるか
- どの層・機能へ接続するか
- 接続条件がある場合、その条件を判定すること
- 同じ意味を複数の受け手へ分岐すること
- 必要に応じて受け手向けの意味へ変換・補足すること

Synapseそのものに人格・感情・行動選択の全責務を持たせない。

- 感情状態を決めるのは Heart Engine
- 高次認知を行うのは Pi5 cognition
- 即時身体反応を担うのは Reflex Layer
- 最終的な身体競合を仲裁するのは Behavior Selector

という責務境界を維持する。

## 4. 生物学的シナプスとの違い

本プロジェクトでの `Synapse` は、生物学的神経系を厳密に再現するものではない。

現段階では以下をSynapseの必須仕様としない。

- 学習による結合強度変化
- 発火閾値
- 可塑性
- 強化学習
- 時間依存重み
- ニューラルネットワークとしての学習

将来、Heart Engineや関係性・記憶の設計から必要性が生じた場合に拡張を検討する。

**「シナプス」という比喩が、実装を不要に複雑化させてはならない。**

## 5. 最初の実装方針

神経回路全体を一括実装しない。

既存LOCKどおり、最初の縦貫通実証は **M5Stack Unit ToF4M / U172** とする。

最初の小ゴールは、ToF4Mを単に動作させることではなく、

**神経回路の最小骨格を作り、ToF4Mから生まれた一つの意味がNeuronとSynapseを通って反射・Heart・行動系へ届くことを実証すること**

とする。

概念例：

```text
ToF4M
↓
Raw distance
↓
Adapter / Perception
↓
PERSON_APPROACHING
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

この段階ではSynapseを巨大な汎用フレームワークへしない。

まず一つの感覚を一本縦に通し、その実装で必要性が確認された機能だけを後から共通化・拡張する。

## 6. 設計図用の短縮要約

聖典、Ghost Map、ブロック図、実装説明では、必要に応じて以下の短縮表現を使用してよい。

> **神経回路：感覚・反射・Heart・記憶・Pi5・身体を結ぶ生命情報網。Neuronが意味を運び、Synapseがその意味を必要な層へ接続する。**

さらに短く表記する場合は、

> **Neuron＝意味を運ぶ／Synapse＝意味を結ぶ／神経回路＝全体をつなぐ**

とする。

## 7. LOCK

以下を正式LOCKとする。

1. `神経回路 / Neural Circuit` を、感覚から身体までを結ぶ生命情報網の正式呼称として採用する。
2. `Neuron / Semantic Neuron` を、共通意味を運ぶ神経線・神経語彙として扱う。
3. `Synapse / シナプス` を、Neuronの意味を必要な層へ接続・分岐する接続点／接続規則として正式採用する。
4. Synapseは現段階で学習・可塑性・重み付きニューラルネットを意味しない。
5. 神経回路を一括構築せず、ToF4Mから一本ずつ縦貫通実装する。
6. 新しい感覚・認知・身体機能を追加する際も、NeuronとSynapseの責務境界を維持する。
