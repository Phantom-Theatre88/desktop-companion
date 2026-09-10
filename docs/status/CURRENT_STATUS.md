# Desktop Companion — CURRENT STATUS

更新日: 2026-09-10

この文書は、このプロジェクトの「現在地」の正本である。
設計判断は `docs/sacred/` の聖典を優先し、現状確認はこの文書を必ず参照する。

## 1. 最上位ゴール

M5Stackを、単なる音声AI端末ではなく、

**「机の上に常にいて、周囲を感じ、反応し、気分が変わり、少しずつ関係性が育つ相棒」**

として成立させる。

評価基準は機能数ではなく、**「そこにいる感じ」があるか**。

## 2. 現在の大きな論点

課題1「神経入力の共通仕様」と課題2「行動仲裁・対人Session・身体制御」はLOCK済み。

神経回路・Neuron・Synapseの定義もLOCKし、現在は**最初の実神経となるToF4M / U172の縦貫通実装**へ移行した。

- Neuron：意味を運ぶ神経
- Synapse：Neuronが運ぶ意味を必要な受け手へ結ぶ接続点／接続規則
- 神経回路：感覚・反射・Heart・記憶・Pi5・Behavior Selector・身体を結ぶ全体網

## 3. Visionの現在判断

最終視覚系は**2カメラ構成**。

- **M5Stack側カメラ＝反射の目**：人／顔の存在、位置、動き、単純ジェスチャ等を軽量・低遅延に処理し、視線・首・表情・反射へ接続
- **Pi5側カメラ＝理解する目**：広い視野、高次人物／物体／状況認識、記憶照合、会話・認知への視覚情報供給

Pi5側高次視覚カメラは **Raspberry Pi Camera Module 3 Wide** を採用LOCK。

## 4. 購入済み・正式登録済みハードウェア

### M5Stack側感覚器官

- **Unit ToF4M / U172**：購入済み。距離・接近感覚。I2C `0x29`。
- **Unit TMOS PIR / U185**：購入済み。人の在席・存在・活動感覚。I2C `0x5A`。
- **Unit ENV-Pro / U169**：購入済み。温度・湿度・気圧を基本とする環境感覚。I2C `0x77`。
- **Unit Hub / U006**：購入済み。HY2.0-4P/Groveを1→3分岐する末梢神経の分岐部。

3センサーはI2Cアドレスが異なるためアドレス上は同一バスで共存可能。実機統合時に電源・バス負荷・タイミングを確認する。

### Raspberry Pi 5側

- SSD：購入済み
- 高次視覚：**Raspberry Pi Camera Module 3 Wide 採用LOCK**

## 5. M5Stack／Pi5の役割分担

### M5Stack

**身体＋生命維持できる低次脳＋反射系＋Heart Engineの常時稼働部分**。

Pi5が停止しても、見る・感じる・反応する・気分が変わる・基本的自発行動を継続する。

### Raspberry Pi 5

**高次感覚野＋認知脳＋言語＋長期記憶**。

人物・物体・状況理解、高度ASR、会話、LLM、長期記憶照合等を担当する。

## 6. 神経系の現在LOCK

全体骨格：

**外界 → 感覚器官 → Raw Perception → Perception Integration → Semantic Neuron → Synapse → Reflex / Heart / Memory / Pi5高次認知 → Behavior Selector → 身体出力**

重要ルール：

- Perception Integrationで `timestamp`、`confidence`、複数感覚の統合を扱う。
- 各センサー値を直接Heartへ垂れ流さない。
- ToF単独では人物を断定せず、近接意味 `PROXIMITY_*` までを出す。
- 反射はHeart Engine／Pi5の判断完了を待たない。
- Pi5結果は原則Semantic Neuronへ意味として返す。
- Synapseは接続先・分岐・接続条件を扱い、現段階では学習・重み・可塑性を必須仕様にしない。
- 複数センサーやPi5が身体制御権を直接奪い合わない。
- 一部センサーやPi5が停止しても、M5側の生命活動を継続する。
- 感覚→意味化→神経配送→Heart／反射→行動の因果をログで追跡可能にする。

## 7. 聖典Stepへの暫定再マッピング

- Step 0｜基準固定: **再整理中**
- Step 1｜親Repo基準動作確認: **部分達成**
- Step 2｜表情エンジン: **部分達成・聖典仕様との差あり**
- Step 3｜反射層: **部分達成・Vision系実機未達**
- Step 4｜Heart Engine v0.1: **未実装**
- Step 5｜自発行動: **先行実装あり・Heart Engine非連携**
- Step 6｜短期記憶・生活リズム: **ごく一部のみ**
- Step 7｜Pi5ローカル接続: **未実装**
- Step 8｜ローカル会話: **未実装。現状Xiaozhi経由**
- Step 9｜長期記憶: **未実装**
- Step 10｜感覚器官追加: **ToF4M神経1本目の実装開始。Neuron/Synapse骨格とToF近接意味化コード作成済み。ハードウェア取得・身体反応・実機確認は未達**
- Step 11｜v1.0: **未達**

## 8. 現在の小ゴール

**ToF4M / U172を最初の実神経として、Neuron＋Synapseを含む神経回路を1本縦に通す。**

実装仕様正本：
`docs/implementation/ToF4M_U172_神経1本目_実装仕様_v0.1.md`

今回作成済み：

- `stackchan/neural/neural_circuit.h/.cpp`
  - `SemanticEvent`
  - `Synapse`
  - `NeuralCircuit`
- `stackchan/senses/tof4m/tof4m_perception.h/.cpp`
  - 生距離サンプル受け
  - `PROXIMITY_APPROACHING`
  - `PROXIMITY_NEAR`
  - `PROXIMITY_LEAVE`
  - near/leaveのヒステリシス

未達：

1. U172実機からの距離取得ドライバ
2. timestamp / confidenceの実取得
3. SynapseからReflex Layerへの実接続
4. Heart Engineへの配送先実装
5. Behavior Selectorへの行動要求接続
6. Eyes / Neck等の身体反応
7. 因果ログ
8. ESP-IDF 5.5.4ビルド確認
9. 実機確認

## 9. 確認済み不整合・ブロッカー

### U172ドライバ適合

現行YukiはESP-IDF 5.5.4ネイティブC/C++。M5Stack公式U172ライブラリ／例はArduino/M5Unit系が中心のため、そのままコピーせずESP-IDF側へ適合させる必要がある。

### AGENTS.mdの参照切れ

`AGENTS.md` は `docs/sacred/M5Stack Desktop Companion 実装Step聖典 v0.3.md` を必読指定しているが、現リポジトリの `docs/sacred/` には当該ファイルが存在しない。

この参照切れは今後修正が必要。内容を推測して新規作成・置換はしない。

## 10. 次回最初に確認すること

1. StackChan K151 / CoreS3の外部Grove I2Cが現行ESP-IDFファーム内でどのbus／HALを使用すべきか確認
2. U172/VL53L1XのESP-IDF適合ドライバ方針決定
3. 実機Raw Perception取得
4. `ToF4MPerception` へ実サンプルを接続
5. 最初のSynapseをReflex Layerへ接続

## 11. 完成判定

現在は **部分実装中**。

Neuron/Synapseの最小骨格とToF近接意味化まではコード化したが、神経1本目の完成条件である実機入力→神経配送→身体反応までは未達。