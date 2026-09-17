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

**神経・シナプス層：本番骨格実装済み、Step 4最小本番実装を継続中**

現在、`heart-engine` ブランチの `firmware/cores3_base/` に以下を実装済み。

- `SemanticNeuron`：製品非依存の意味イベント
- `SynapseRouter`：Neuronを必要な層へ分岐する固定長ルータ
- `GhostCore`：Ghostの本番受け口
- `HeartEngine`：LOCK 28初期値、Heart Context snapshot境界
- `MemoryEngine`：短期イベント保持と将来拡張の本番境界
- `TimeEngine`：時間処理の本番境界
- `RelationshipEngine`：関係性処理の本番境界
- `BehaviorEngine`：Behavior処理の本番境界
- `ReflexLayer`：即時反応用の意味→ReflexIntent変換
- `DesktopCompanionRuntime`：Synapse / Ghost / Reflexを束ねる常駐Runtime
- `FaceRenderer`：M5GFXによる身体出力側の顔描画器官
- `HeartPersistence`：Arduino `Preferences` を使ったNVS主保存の本番境界

起動時にRuntimeを初期化し、以後のAdapterは `runtime.emit(SemanticNeuron)` で神経系へ接続できる構造になっている。

2026-09-17、CoreS3実機で現行 `heart-engine` の基盤・Ghost・FaceRendererまでコンパイル／書き込み／起動を確認した。

LOCK 47〜49に従い、`ReflexResult` をRuntimeからGhostへ戻し、Heart / Memoryへ渡す本番フィードバック境界を追加した。2026-09-17、追加後のコードについてもCoreS3実機でコンパイル／書き込み／起動まで確認済み。具体的なHeart変化量、Memory保持条件、Reflex感度変化量は未LOCKのため実装していない。

さらにLOCK 51として、Heart永続化の保存方式を **NVS主保存＋microSDバックアップ** に固定した。

2026-09-17、NVS主保存の第一段として `HeartPersistence` を追加し、

- NVSにHeart 6状態を保存できる
- NVSに既存snapshotがあるか判定できる
- NVSから保存済みHeart snapshotを読み出せる
- 初回起動時はLOCK 28初期値をNVSへ初期保存する
- 通常起動時は既存NVS snapshotを読み込んだことをHeartEngineで判別できる
- Serialで初回初期化／既存snapshot読込を確認できる

本番境界を実装した。

ただし、LOCK 20の状態別復元については、`mood / curiosity` の減衰量、`boredom / sleepiness` の再計算方法などが未固定のため、保存済みsnapshotをそのまま現在Heartへ機械的に適用していない。ここは推測で埋めない。

## 5. 現在の神経構造

**Hardware → Device Driver → Adapter → Nerve Input → Semantic Neuron → Synapse → Reflex / Ghost / Memory / Pi5 → Behavior → Body**

Neuron＝意味を運ぶ。

Synapse＝どの層へ結ぶかを決める。

Ghost＝Heart / Memory / Time / Relationship / Behaviorの中核。

ReflexはHeartやPi5を待たず先行でき、完了結果はRuntime → Ghost → Heart / Memoryへ戻せる境界を持つ。

## 6. Heart Engineの扱い

Heartの器は本番骨格として実装済みだが、詳細な数値変化ルールは未LOCKのため、現時点では勝手に固定しない。

初期状態項目は、

- mood
- affection
- curiosity
- boredom
- sleepiness
- attention

を保持する。

初回値はLOCK 28の具体値を使用する。

Heart Contextはread-only snapshotとして扱い、1イベント開始時のsnapshotを同一イベント内で固定する。

Semantic NeuronおよびReflex結果がGhostへ到達する本番配線を先に成立させる。

Heart永続化はLOCK 51に従い、**CoreS3内部NVS（Arduino `Preferences`）を主保存、microSDをバックアップ**とする。

現在はNVS主保存のストレージ境界まで実装済み。初回起動と通常起動を区別し、保存済みsnapshotを取得できる。

まだ実装しないもの：

- 保存周期
- microSDバックアップ世代数
- microSDファイル形式
- 破損判定方式
- Heart係数・閾値
- LOCK 20の状態別復元式の未決部分

これらは推測で固定しない。

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

現在はStep 4を完了させる。

まず、今回追加した **HeartPersistence / NVS主保存境界** がCoreS3上でコンパイル・書き込み・起動できることを確認する。

確認時はSerialで、

- 初回なら `[HEART][NVS] First boot snapshot initialized: OK`
- 既存snapshotがあれば `[HEART][NVS] Existing primary snapshot loaded`

のどちらになるかを見る。

実機確認後、LOCK 20に従う「保存snapshot → 現在Heart」の状態別復元を進める。ただし、コードを書くために必要な復元ルールが未決なら、その具体点だけ追加設計へ戻る。

その後、同じHeart状態をmicroSDへバックアップできる本番境界を追加する。

Heart永続化の本番骨格確認後、Step 4に残るMemory本番境界とReflex感度修飾入口を確認する。Step 4完了後、Step 5として最初の外部感覚 **ToF4M / U172** を Device Driver層から再開する。ToF4M未検出問題はセンサー固有層で切り分け、Ghost / Runtimeを巻き込まない。

## 11. 完成判定

Desktop Companion全体としては未完成。

ただし、**ゼロベース基盤から生命情報を流す神経RuntimeとGhost本番骨格が実機起動し、Reflex結果の内面フィードバック経路まで実機確認済み。現在はHeartのNVS主保存境界を実装し、実機確認へ進む段階**にある。
