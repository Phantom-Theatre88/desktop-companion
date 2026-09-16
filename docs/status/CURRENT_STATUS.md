# Desktop Companion — CURRENT STATUS

更新日: 2026-09-17

この文書は、このプロジェクトの「現在地」の正本である。

## 1. 最上位ゴール

M5Stackを、単なる音声AI端末ではなく、

**「机の上に常にいて、周囲を感じ、反応し、気分が変わり、少しずつ関係性が育つ相棒」**

として成立させる。

評価基準は機能数ではなく、**「そこにいる感じ」があるか**。

## 2. 現在の正式実装方針

実装基盤はゼロベースを維持する。

- M5Stack CoreS3
- Arduino
- M5Unified
- M5GFX
- 独自コード

既存OSSを親Repoにしない。

基盤コードは実装済みで、プロジェクト上は**OS相当の基盤準備完了**として次段へ進む。

Ghostは後付け機能ではなく、最初から本体中核として接続する。

## 3. Desktop Robo参照実装

Ghost本体は独自実装のまま維持し、以下を正式な参照実装として採用済み。

- Yuki Desktop Robot：Face / Eye / Mouth / Tracking / Head Motion
- AuraBot：Pi5-CoreS3 Boundary / Perception Architecture / Communication
- KariPom：MicroBehavior / Life Presence

参照実装の都合でGhost構造を曲げない。

## 4. 現在の実装段階

**基盤／OS相当：準備完了**

**神経・シナプス層：実装開始**

現在、`heart-engine` ブランチの `firmware/cores3_base/` に以下を実装済み。

- `SemanticNeuron`：製品非依存の意味イベント
- `SynapseRouter`：Neuronを必要な層へ分岐する固定長ルータ
- `GhostCore`：GhostのNeuron受け口と短期イベント保持
- `ReflexLayer`：即時反応用の意味→ReflexIntent変換
- `DesktopCompanionRuntime`：Synapse / Ghost / Reflexを束ねる常駐Runtime

起動時にRuntimeを初期化し、以後のAdapterは `runtime.emit(SemanticNeuron)` で神経系へ接続できる構造になっている。

## 5. 現在の神経構造

**Hardware → Device Driver → Adapter → Nerve Input → Semantic Neuron → Synapse → Reflex / Ghost / Memory / Pi5 → Behavior → Body**

Neuron＝意味を運ぶ。

Synapse＝どの層へ結ぶかを決める。

Ghost＝Heart / Memory / Time / Relationship / Behaviorの中核。

## 6. Heart Engineの扱い

Heartの器は実装を開始しているが、詳細な数値変化ルールは未LOCKのため、現時点では勝手に固定しない。

初期状態項目は、

- mood
- affection
- curiosity
- boredom
- sleepiness
- attention

を保持する。

Semantic NeuronがGhostへ到達する本番配線を先に成立させる。

## 7. 最初の感覚縦貫通

最初の外部感覚対象は引き続き **Unit ToF4M / U172**。

ただしToF4Mは実機側で未検出問題が残っているため、神経・シナプス本体をセンサー固有問題へ巻き込まない。

ToF4M側は、

**Device Driver → Adapter → PROXIMITY_* Semantic Neuron**

までを独立実装し、検出問題解決後に現在のRuntimeへ接続する。

## 8. 購入済みハードウェア

### M5Stack側

- Unit ToF4M / U172
- Unit TMOS PIR / U185
- ENV-Pro / U169
- Unit Hub / U006

### Raspberry Pi 5側

- SSD
- Raspberry Pi Camera Module 3 Wide

## 9. M5Stack／Pi5の役割

### M5Stack

身体＋生命維持できる低次脳＋反射系＋Heart Engineの常時稼働部分。

### Raspberry Pi 5

高次感覚野＋認知脳＋言語＋長期記憶。

Pi5停止時でもM5Stack側の基本生命活動を継続する。

## 10. 次の実装

現在のRuntimeを基準に、感覚器官ごとのAdapterを1本ずつ接続する。

最優先はToF4Mだが、ハード未検出問題はDevice Driver層として切り分ける。

その後、TMOS PIR、ENV-Pro、視覚、Touch、IMU、聴覚へ拡張する。

各感覚は生データ取得で終わらせず、Semantic Neuron → Synapse → Ghost / Reflexまで縦貫通させる。

## 11. 完成判定

Desktop Companion全体としては未完成。

ただし、**ゼロベース基盤から生命情報を流す神経Runtime実装へ移行した段階**に入った。
