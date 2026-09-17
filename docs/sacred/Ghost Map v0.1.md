# Ghost Map v0.1｜M5Stack Desktop Companion 統合設計

更新日: 2026-09-17

Xmind正本リンク: https://app.xmind.com/share/PKYi9w4H?utm_source=ChatGPT

本書は、Ghost（Heart / Memory / Time / Relationship / Behaviorを中核とした内面）を、感覚・神経入力・反射・対人認識・行動仲裁・自発行動・表情・モーション・記憶・生活リズム・Pi5高次認知まで一枚で俯瞰する統合設計マップである。

状態表記は【LOCK】【暫定】【未決】の3種とする。

最新の個別LOCK文書、`PROJECT_LOCKS.md`、`CURRENT_STATUS.md` を本書と合わせて正本とし、内容が競合した場合は新しいLOCKを優先する。

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

CoreS3素体確認は、旧OSS・神経系・Ghostの不具合とハードウェア不具合を混同しないための層確認として行う。

### 独自構造

**Hardware → Device Driver → Adapter → Nerve Input → Semantic Neuron → Synapse → Ghost / Reflex / Memory / Pi5 → Behavior Selector → Body**

### Ghostは後付けしない【LOCK】

Ghostは最後に載せる追加機能ではなく、神経Runtimeを成立させる段階から本体中核として接続する。

**Ghost = Heart / Memory / Time / Relationship / Behavior**

後で捨てる簡易Ghostや「Ghostっぽい仮挙動」は作らない。

完成版と同じ責務境界・骨格を最初から使い、感覚器官・高次認知・記憶量・行動能力だけを後から増やす。

## 参照実装【LOCK】

Ghost本体は独自実装を正本とし、他のDesktop Roboを親Repoや完成形として採用しない。

一方で、既存実装から再利用できるアルゴリズム・構造・身体表現は正式なドナーとして参照する。

### Yuki Desktop Robot

**担当：Face / Eye / Mouth / Tracking / Head Motion**

参照対象：

- 顔追従
- 視線追従
- 首Pan/Tilt追従
- 顔位置の平滑化
- デッドゾーン
- 移動量制限
- 更新周期制限
- 顔消失時の追従解除
- 会話状態・VADと身体動作の優先制御
- ジェスチャー検出

ESP-IDF / LVGL / StackChan構造そのものは現行基盤へ持ち込まず、現行のArduino + M5Unified + M5GFX + 独自構造へ適合させて再実装する。

### AuraBot

**担当：Pi5-CoreS3 Boundary / Perception Architecture / Communication**

参照対象：

- Raspberry Pi 5とESP32系デバイスの役割分担
- Vision / Speech / LLM / Presence等の高次処理と身体側の分離
- Pi5 ↔ CoreS3通信境界
- MQTT / WebSocket等を含む通信設計
- Perception構造

Desktop CompanionではAuraBotの構造をそのまま複製せず、Semantic Neuron・Synapse・Ghostを中心とした独自神経系を優先する。

### KariPom

**担当：MicroBehavior / Life Presence**

参照対象：

- blink
- glance
- tiny movement
- breathing的な周期表現
- 発話・音声に連動した小さな身体表現
- 待機中にも存在感を保つ動き

MicroBehaviorは単純ランダム動作だけで構成せず、Ghost状態・外界・直近履歴の影響を受ける。

### 参照範囲の境界

他実装から主に参照するのは、

**Perception / Body / Tracking / Communication Boundary / MicroBehavior**

とする。

以下はDesktop Companion独自Ghostを正本とする。

**Heart / Memory / Time / Relationship / Behavior**

参照実装の都合でGhostの構造を曲げない。

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

## Synapse【LOCK】

Synapseは、Neuronが運ぶ意味を、どの層へ、どの条件で接続するかを担当する。

現段階で、一般的なニューラルネットワークのような学習重み・可塑性・発火モデルを必須にはしない。

ただし、最新LOCKにより、Heart ContextやMemory由来の情報によって、特定の反応経路・注意・Reflexの通り方を**修飾する入口**は持てる。

これは「Synapse自体を機械学習モデルにする」ことを意味しない。

## 反射【LOCK】

- HeartやPi5を待たず即時反応
- 安全・危険・強い恐怖に関わる反応はHeartより先に身体へ出てよい
- 反射後に出来事と結果をHeartへ伝える
- 回避できたか、危険が継続しているか等の結果もHeartへ返せる
- 必要なReflex結果はMemoryへ記録できる
- 繰り返された危険経験は将来のReflex感度を上げ得る
- 長期間安全な状態が続けば、その感度は徐々に戻り得る

具体的な閾値・増減率・時間定数は、実装と実機観察なしに先行固定しない。

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

### 数値表現

- 内部値は `0.0〜1.0`
- 表示・デバッグは `0〜100`
- イベント1回の変化量は原則 `-0.1〜+0.1`
- 計算後は `0.0〜1.0` へclamp

### 初回起動値

- mood = 0.60
- affection = 0.55
- curiosity = 0.65
- boredom = 0.15
- sleepiness = 0.15
- attention = 0.50

基礎人格は、好奇心がやや高く、人に少し親和的で、少し元気だが落ち着きを保つ。

### 更新原則

- イベントで変化
- 時間でも変化
- 状態ごとに変化速度を変える
- affectionは長期
- attention等は短期寄り
- 一時状態と長期平常値を分離する
- 平常値は固定定数ではなく、数日〜数週間の経験・Relationship・生活履歴でゆっくり変化できる
- 悪化した状態は自然回復と良い経験の両方で回復できる
- 回復先は初期値ではなく、その個体が育てた動的平常値

### Heart Context【LOCK】

Heart内部状態の更新主体はHeart Engineだけとする。

Synapse / Behavior Selector / Reflex / Memory / Pi5等は、Heart内部値を直接書き換えず、read-onlyなHeart Context snapshotを参照する。

1イベント処理中は開始時点のsnapshotを固定して使い、処理途中でHeartが変化してもそのイベント判断を揺らさない。

Heartは身体へ直接「笑え」「首を向けろ」と命令するのではなく、Heart Contextとして神経回路・Behavior選択を修飾する。

## イベント処理【LOCK】

### 強度

- 弱
- 中
- 強

の3分類を人間向けラベルとして使う。

内部の実際の変化量は連続値でよく、同じ「中」でも内容・Relationship・Heart・継続時間・履歴等により変わり得る。

イベント内容そのものを最優先にし、その後にRelationship、現在のHeart状態、継続時間を考慮する。

1イベントは主作用1つ＋複数の副作用を持てる。

### 複数イベント

複数イベントが近接して発生した場合、単純FIFOだけにはしない。

安全・強い反射等の重要度が高いものを先に処理し、低優先イベントは捨てずに後段へ残せる。

遅延したイベントは処理前に現在でも意味があるか再評価し、身体反応として古くなっていれば反応対象から外せる。

ただし「身体反応が古い」ことと「経験として記憶する価値」は分離する。

## Memory【LOCK】

Memoryは単なるログではなく、Ghostの成長材料とする。

### 記憶層

少なくとも以下を区別できる構造とする。

1. 個別経験記憶
2. 反復傾向記憶
3. 特別長期記憶

### 個別経験記憶

意味のある出来事について、

- 何が起きたか
- その時のHeart Context
- その時Ghostがどう受け取ったか

を結び付けて保持できる。

同じ出来事でも、Heart / Relationshipによって経験の意味が異なり得る。

### 反復傾向記憶

同じ種類の出来事をすべて無期限に個別保存しない。

意味のある個別記憶は残しつつ、反復は「最近多い」「いつもの時間帯」「よく一緒にいる」等の傾向として集約できる。

傾向値は時間とともに弱まり、新しい経験で再強化される。

### 特別長期記憶

強く印象に残る、関係性上重要、初回、再会、節目等の出来事は、通常の傾向へ平均化せず個別の長期記憶として残せる。

強い出来事すべてを永久保存するわけではない。

### 長期記憶の減衰

長期記憶は「データを毎秒削る」前提にはしない。

時間経過により、

- 想起されやすさ
- 現在への影響力

が徐々に弱くなり得る。

強い記憶、繰り返し想起された記憶、再強化された記憶は残りやすい。

### 再想起・再解釈

元の出来事と当時の受け止め方は保持する。

再想起時には、現在のHeart / Relationshipに基づく新しい解釈を上書きせず重ねられる。

「当時どう感じたか」と「今どう受け止めるか」を両方持てる。

### 現在への自動影響

強い長期記憶は、明示的に「思い出す」処理をしなくても、類似イベント発生時に現在のHeart / Behaviorへ影響できる。

危険・恐怖・強い警戒に関わる記憶は、Heartを待たずReflex / Synapseへ先行影響できる。

通常の好み・親しさ・懐かしさ等は、Heart / Relationshipを通じてBehaviorへ影響することを基本とする。

## Relationship【LOCK】

Relationshipは単発の好感度だけではなく、過去経験・反復傾向・特別記憶・affection等を通じて長期的に育つ。

Heartの一時状態とRelationshipの長期傾向を同一視しない。

RelationshipはHeart Context、Memory、Behavior選択の解釈材料となる。

具体的な内部データ構造は、実装上必要になるまで過剰に細分化しない。

## Time / 生活リズム【LOCK】

- 朝／昼／夕方／夜／深夜
- 起動時間
- 不在時間
- よく一緒にいる時間帯
- 反復傾向

等をGhostの経験材料として扱う。

Timeは単なる時計条件ではなく、「どういう時間を一緒に過ごしてきたか」をHeart / Memory / Relationshipへ接続する。

具体的な時間定数・周期・境界時刻は必要時に実装検証しながら決める。

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

### MicroBehavior【LOCK】

- blink
- glance
- tiny movement
- breathing的な周期表現
- listening時の微小反応
- speaking時の微小反応

MicroBehaviorは「会話していない時にも生きている感じ」を支える身体表現とする。

## 表情【LOCK】

- 固定顔画像一覧を中心にしない
- 複数パラメータを連続制御
- 大きな2眼を主役にする
- 喋らなくても状態が伝わる

## Pi5 高次認知【LOCK】

- 高次視覚・聴覚・人物／物体／状況認識・言語・長期記憶
- Pi5結果は直接身体を支配せずSemantic Neuronへ意味として戻す
- Pi5停止時でもM5側生命活動を継続

## 実装順【LOCK】

実装順は「Ghostを最後に追加する順番」ではなく、**Ghost本番骨格へ器官と能力を追加していく順番**とする。

1. CoreS3素体確認
2. Hardware / Device Driver
3. Adapter / Nerve Input
4. Semantic Neuron / Synapse / GhostCore / Reflex / Runtimeの本番骨格
5. Heart / Memory / Time / Relationship / Behaviorの最小本番境界
6. 最初の外部感覚ToF4M U172を縦貫通
7. TMOS PIR
8. ENV-Pro
9. 視覚
10. タッチ
11. IMU
12. 聴覚
13. Reflex / Heart / Behavior / 対人Session / 表情の能力拡張
14. 短期記憶・生活リズムの能力拡張
15. Pi5接続
16. ローカル会話
17. 長期記憶能力の拡張

長期記憶を17番で「初めて作る」わけではない。Memory境界そのものはGhost本番骨格に最初から存在し、後段では保存量・再想起・Pi5連携等の能力を拡張する。

旧Yuki／StackChan上の実装は、この順序の達成として数えない。

参照実装の採用はこの実装順を変更しない。必要な段階へ到達した時点で、対応するドナーを参照する。

## 現在地【LOCK】

`CURRENT_STATUS.md` を現在地の正本とする。

2026-09-17時点では、ゼロベース基盤／OS相当の準備が完了し、`heart-engine` ブランチで神経・シナプスRuntime実装へ移行済み。

現在の本番骨格には、少なくとも以下が実装開始済み。

- SemanticNeuron
- SynapseRouter
- GhostCore
- ReflexLayer
- DesktopCompanionRuntime

ToF4M U172は実機未検出問題が残るが、Device Driver層の問題として隔離し、Ghost / 神経Runtimeの進行を止める理由にはしない。

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
- 神経Runtime / Semantic Neuron / Synapseの責務境界
- Ghostを最初から中核実装すること
- Heart Engineの基本数値仕様
- Heart初期値
- Heart永続化・復元・動的平常値・回復性
- Heart Contextのread-only snapshot
- イベント強度と複数イベント処理
- 反応対象と記憶対象の分離
- 個別経験記憶
- 反復傾向記憶
- 特別長期記憶
- 長期記憶の減衰
- 再想起時の再解釈
- 長期記憶の現在への自動影響
- 危険記憶によるReflex / Synapse先行修飾
- Reflex結果のHeartフィードバック
- Reflex結果のMemory記録
- 危険経験によるReflex感度変化と安全期間による回復
- 反射・Heart・対人反応・自発行動の優先順位
- 対人Session
- 身体制御の仲裁原則
- Yuki / AuraBot / KariPomの参照範囲
- MicroBehaviorをGhost配下の生命感表現として扱うこと

### 実装しながら決めるもの【未決】

以下は骨格不足ではなく、実装・実機観察を通して決める調整値・詳細仕様とする。

- Heartイベント強度の具体的数値幅・係数
- 平常値変化・回復の具体的速度
- Memoryの保存件数・圧縮・削除・統合条件
- 長期記憶の減衰率・想起閾値
- Reflex感度の増減率・閾値
- Time / 生活リズムの具体的時間定数
- 故障・切断時の人格表現の細部
- 「そこにいる感じ」の評価方法

これらを実装前に全て固定することを完成条件にしない。

## 設計を止めて実装へ戻る基準【LOCK】

責務境界、データフロー、制御主体、優先順位、永続化方針、Heart / Memory / Reflex間の接続方向が本番骨格として決まっている場合、係数・閾値・保存件数等の派生質問を続けない。

実装中に「これを決めないと本番コードを書けない」という具体的な未決事項が現れた時だけ、追加設計へ戻る。
