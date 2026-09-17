# CoreS3 Base — 現行実装基準点

M5Stack Desktop Companion の新しいゼロベース実装基準点。

## 基盤

- Hardware: M5Stack CoreS3
- Framework: Arduino
- Library: M5Unified
- Graphics: M5GFX
- Application: M5Stack Desktop Companion 独自コード

既存Stack-chan系OSSは親Repo・初期基盤として使用しない。

## 現在地

Step 0のClean Base確認後、現在は神経RuntimeとGhost本番骨格の実装へ進んでいる。

現在の主な構成：

- `SemanticNeuron`
- `SynapseRouter`
- `DesktopCompanionRuntime`
- `ReflexLayer`
- `GhostCore`
- `HeartEngine`
- `MemoryEngine`
- `TimeEngine`
- `RelationshipEngine`
- `BehaviorEngine`

Ghostは後付けせず、`Heart / Memory / Time / Relationship / Behavior` を本番境界として最初から保持する。

## Heart Engine 現在実装

LOCK済みの初回起動値を使用する。

- mood = 0.60
- affection = 0.55
- curiosity = 0.65
- boredom = 0.15
- sleepiness = 0.15
- attention = 0.50

内部値は0.0〜1.0で扱い、`HeartContext` はread-only snapshotとして取得する。

1イベント処理では開始時点のHeart Contextを固定し、同一イベント中の各Ghost要素へ同じsnapshotを渡す。

イベントごとの具体的なHeart変化量、時間減衰率、回復係数等は、未LOCK値を勝手に実装しない。

## 起動確認

画面：

- `CoreS3 BASE`
- `NERVE / GHOST READY`

Serial 115200：

- Runtime READY / ERROR
- Synapse binding数
- Heart初期6値

を確認できる。

## 旧資産

Stack-chan本家、AI_StackChan_Ex、stack-chan-ko、RoboEyes、旧Yuki実装等はLEGACY参考資産として扱い、現行基盤へ自動的に混在させない。
