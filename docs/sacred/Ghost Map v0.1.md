# Ghost Map v0.1｜M5Stack Desktop Companion 統合設計

更新日: 2026-09-11

Xmind正本リンク: https://app.xmind.com/share/PKYi9w4H?utm_source=ChatGPT

本書は、Ghost（Heart Engineを中心とした内面）を、感覚・神経入力・反射・対人認識・行動仲裁・自発行動・表情・モーション・記憶・生活リズム・Pi5高次認知まで一枚で俯瞰する統合設計マップである。

状態表記は【LOCK】【暫定】【未決】の3種とする。

## 最上位ゴール【LOCK】

### 存在させる

- 机の上に常にいる相棒
- 周囲を感じる
- 反応する
- 気分が変わる
- 関係性が育つ

### 評価基準

- 機能数ではなく「そこにいる感じ」
- 会話していない時間にも生命感がある

## 実装基盤【LOCK】

### ゼロベース

- M5Stack CoreS3
- Arduino
- M5Unified
- M5GFX
- 独自コード

親Repoは置かない。

`stack-chan-ko`、M5Stack/StackChan、Dotty StackChan、stackchan-local、RoboEyes、Xiaozhi、旧Yuki実装は初期基盤へ入れない。

### 最初の確認

**DISPLAY → TOUCH → IMU → PROXIMITY → MIC → SPEAKER → CAMERA → その他必要ハード**

M5Stackちゃんらしい機能は、この素体確認より先に作らない。

### 独自構造

**Hardware → Device Driver → Adapter → Nerve Input → M5Stackちゃん**

## 感覚・神経入力【LOCK】

### 基本骨格

**外界 → 感覚器官 → Raw Perception → Perception Integration → Nerve Input → Semantic Neuron**

### 神経回路・Neuron・Synapse

- 神経回路：感覚・反射・Heart・記憶・Pi5・身体を結ぶ生命情報網
- Neuron：意味を運ぶ
- Synapse：意味を必要な層へ結ぶ

### 神経入力の共通仕様

- Device固有値を神経へ直接入れない
- Device → Device Driver → Adapter → 意味化・正規化 → Nerve Input
- 製品交換後も神経以降の意味を変えない
- 生値は必要時のみpayloadへ保持

### カメラ

**Camera → Vision / Recognition → 意味抽出 → Nerve Input**

- M5 Camera＝反射の目
- Pi5 Camera Module 3 Wide＝理解する目

### 主な感覚器官

- ToF4M U172：距離・接近・離脱
- TMOS PIR U185：在席・存在・活動
- ENV-Pro U169：温度・湿度・気圧・環境変化
- Touch：接触・撫で・急接触
- IMU：姿勢・揺れ・持ち上げ
- Mic：音声活動・呼びかけ
- 照度：明暗・昼夜変化

## 反射【LOCK】

- HeartやPi5を待たず即時反応
- 反射後に出来事をHeartへ伝える

## 対人認識【LOCK】

### 対象区分

- Kim
- 既知の他人
- 未知の人
- 認識不能／確信度不足

### 原則

- 呼びかけ・接近への即時反応と、人物同定を分離する
- PERSON_NEAR後のIDENTIFYING中でも対人Sessionを保持できる
- 後から認識結果が返っても同一Session内の相手情報を更新する

## Heart Engine / Ghost【LOCK】

### v0.1内部状態

- mood
- affection
- curiosity
- boredom
- sleepiness
- attention

### 更新原則

- イベントで変化
- 時間でも変化
- 状態ごとに変化速度を変える
- affectionは長期
- 驚き・attentionは短期寄り

## 行動仲裁【LOCK】

優先順位：

1. 安全・強い反射
2. 意味のある対人反応
3. Heart由来の状態表現
4. 自発行動
5. idle

### 対人Session

- 会話・視線・表情・相槌・発話待ち等を同一文脈で保持
- Heart表現・自発行動・idleは原則Sessionを壊さない
- 安全・強い反射はSession中でも即時割り込み可能
- 反射後、文脈が有効なら対人状態へ戻れる

### 身体制御

- 全身を一括占有しない
- 行動は必要な身体出力だけ要求する
- 同じ身体出力に競合した時だけBehavior Selectorが仲裁する
- Heart状態は可能な限り現在行動への修飾として重ねる

## 自発行動【LOCK】

- 完全ランダムにしない
- Heart状態＋外界＋直近履歴から選ぶ
- 「何もしない」も正式な行動

## 表情【LOCK】

- 固定顔画像一覧を中心にしない
- 複数パラメータを連続制御
- 大きな2眼を主役にする
- 喋らなくても状態が伝わる

## 記憶・生活リズム【LOCK】

- 過去経験がHeart・会話・行動へ影響する
- 短期／エピソード／長期関係記憶
- 朝／昼／夕方／夜／深夜
- Pi5長期記憶がなくてもM5側生命活動は継続

## Pi5 高次認知【LOCK】

- 高次視覚・聴覚・人物／物体／状況認識・言語・長期記憶
- Pi5結果は直接身体を支配せずSemantic Neuronへ意味として戻す
- Pi5停止時でもM5側生命活動を継続

## 実装順【LOCK】

1. CoreS3素体確認
2. Hardware / Device Driver
3. Adapter
4. Nerve Input
5. 最初の外部感覚ToF4M U172を縦貫通
6. TMOS PIR
7. ENV-Pro
8. 視覚
9. タッチ
10. IMU
11. 聴覚
12. 反射・Heart・Behavior・表情を段階的に接続
13. Pi5接続
14. ローカル会話
15. 長期記憶

旧Yuki／StackChan上の実装は、この順序の達成として数えない。

## 旧GitHub実装【LEGACY参考】

- Yuki Avatar
- Yuki Curiosity
- StackChan / Xiaozhi統合
- 旧ESP-IDF 5.5.4ファーム
- 旧ToF4M Neuron / Synapseコード

これらは参考資産であり、現行基盤・親Repo・現在地ではない。

## 実装前検討課題

### LOCK済み

- 神経入力の共通仕様
- 反射・Heart・対人反応・自発行動の優先順位
- 対人Session
- 身体制御の仲裁原則

### 未決

- Heart Engineの数値仕様
- 時間の詳細設計
- 記憶境界の詳細
- 故障・切断時の人格表現
- 「そこにいる感じ」の評価方法
