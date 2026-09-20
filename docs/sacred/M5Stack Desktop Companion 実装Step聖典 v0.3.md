# M5Stack Desktop Companion 実装Step聖典 v0.3（2026-09-18改訂）

更新日: 2026-09-20

## 0. この文書の位置づけ

本書は `M5Stack Desktop Companion 設計思想 v0.1.md` を実装へ落とすための作業順序の正本である。

最上位ゴールは、

**「机の上に常にいて、周囲を感じ、反応し、気分が変わり、少しずつ関係性が育つ相棒」**

として成立させること。

### 最新LOCKとの優先関係

本書は、`PROJECT_LOCKS.md`、`Ghost Map v0.1.md`、`CURRENT_STATUS.md`、および個別LOCK文書の最新内容を前提とする。

特に、Ghostは後から載せる追加機能ではない。

**Ghost = Heart / Memory / Time / Relationship / Behavior** を、神経Runtimeを成立させる段階から本体中核として置く。

最短化するのはGhostそのものではなく、Ghostへ到達するまでの寄り道である。

後で捨てる簡易Ghostや「Ghostっぽい仮動作」は作らず、最初から完成版と同じ責務境界・骨格を使い、感覚器官・記憶量・行動能力・高次認知だけを段階的に追加する。

LOCK 52により、外付け感覚より先にCoreS3単体DeskRoboを成立させる。

LOCK 53により、CoreS3単体DeskRobo成立後は、**最初の外部感覚ToF4Mへ進む前にCoreS3内蔵カメラをM5Stack側の「反射の目」として先に仕上げる。**

### LOCK 54｜今後の大工程

現在の実装状態からDesktop Companion v1.0へ向かう上位ロードマップを、

**直近実機確認 → A「感じる」→ B「気分が変わる」→ C「自分から行動する」→ D「昨日と今日をつなぐ」→ E「賢くする」**

の順で固定する。

直近実機確認は、実装済みの **Camera → Vision → Semantic Neuron → Ghost / Memory / Behavior** をCoreS3実機で通すこと。

この大工程は既存Step番号を置き換えず、Step群を最上位ゴールから見て束ねる上位ロードマップとして扱う。詳細は `LOCK54_DesktopCompanion今後の大工程.md` を正とする。

## Step 0｜ゼロベース基準固定

### 基盤

初期構成は以下だけとする。

- M5Stack CoreS3
- Arduino
- M5Unified
- M5GFX
- 独自コード

親Repoは置かない。

M5Stack/StackChan、Dotty StackChan、stackchan-local、M5Stack_RoboEyes、Xiaozhi、旧Yuki実装等は初期基盤へ入れない。

### 旧資産

既存の `firmware/yuki/`、`patches/`、`upstream/`、旧ビルドスクリプト等はLEGACY参考資産とする。

削除はしないが、現行実装の正本・親Repo・動作基準にはしない。

## Step 1｜CoreS3素体確認

CoreS3公式環境だけで各ハード機能を単独確認する。

基本順：

1. DISPLAY
2. TOUCH
3. IMU
4. PROXIMITY
5. MIC
6. SPEAKER
7. CAMERA
8. その他必要ハード

各項目は、対象機能だけを最小コードで確認し、正常／異常を明確にする。

このStepの目的は、CoreS3本体と公式ライブラリ層の正常性を、旧OSSやGhost実装の不具合と混同しないことにある。

### 完成条件

CoreS3本体と公式ライブラリ層の正常性が、旧OSSなしで確認できていること。

## Step 2｜独自ハードウェア層

確認済みのハード機能を、Desktop Companion独自構造へ整理する。

基本構造：

**Hardware → Device Driver → Adapter → Nerve Input**

製品固有差はDevice Driver / Adapter側で吸収する。

Device固有の生値を、そのままGhostや神経上位層へ流さない。

## Step 3｜神経RuntimeとGhost本番骨格

神経入力を共通化し、製品非依存の意味をSemantic Neuronとして流せるRuntimeを作る。

基本構造：

**Nerve Input → Semantic Neuron → Synapse → Reflex / Ghost / Memory / Pi5 → Behavior Selector → Body Output**

この段階で、Ghostを後付けにせず本番骨格として接続する。

最低限の構成は以下とする。

- `SemanticNeuron`：製品非依存の意味イベント
- `SynapseRouter`：意味を必要な層へ接続・分岐
- `GhostCore`：Ghostの常駐中核
- `Heart`：内面状態の本体
- `Memory`：短期・経験・将来の長期記憶へ拡張できる本番境界
- `Time`：時間変化・生活リズムへ拡張できる本番境界
- `Relationship`：関係性を長期的に保持できる本番境界
- `Behavior`：行動候補生成とBehavior Selectorへ渡す本番境界
- `ReflexLayer`：HeartやPi5を待たない即時反応
- `DesktopCompanionRuntime`：上記を常駐接続するRuntime

この時点では全能力を完成させる必要はないが、後で捨てる仮構造にはしない。

## Step 4｜Heart / Memory / Reflexの最小本番実装

Ghostの中核を、最新LOCKに従って最小実装する。

### Heart

v0.1の内部状態：

- mood
- affection
- curiosity
- boredom
- sleepiness
- attention

内部値は `0.0〜1.0` を正本とし、初回起動値は最新LOCKで確定した値を使う。

Heartはイベントと時間の両方で変化し、通常起動では保存状態・現在時刻・生活履歴等を使って状態ごとに適切に復元する。

Heart Contextは外部層から直接書き換えず、Heart Engineが管理するread-only snapshotとして公開する。

1イベント処理中は、開始時点のHeart Context snapshotを固定して使用する。

### Memory

最初から以下へ拡張できる本番境界を持たせる。

- 個別経験記憶
- 反復傾向記憶
- 特別長期記憶
- Relationshipへ影響する記憶

「身体反応としては古くなったイベント」と「経験として残す価値」は分離して扱う。

### Reflex

安全・強い反射はHeartやPi5より先に動ける。

危険・恐怖系の強い長期記憶は、必要に応じてReflex / Synapseを先行修飾できる。

Reflex後は、反射した事実だけでなく、回避できたか・危険が継続しているか等の結果をHeartへフィードバックする。

必要なReflex結果はMemoryへ記録し、繰り返された危険経験は将来のReflex感度へ反映できる。

ただし、具体的な閾値・係数・減衰率・保存件数等は、この段階で一律固定しない。

## Step 5｜CoreS3内蔵カメラ低次視覚 → 最初の外部感覚ToF4M

LOCK 52・53に従い、Step 4完了後は次の順で進める。

### Step 5-A｜CoreS3内蔵カメラを「反射の目」として成立

外付けUnitより先に、CoreS3本体だけで利用できる内蔵カメラをDeskRoboへ接続する。

基本経路：

**Camera Hardware → Device Driver → Vision / Recognition → Nerve Input → Semantic Neuron → Synapse → Reflex / Ghost → Behavior → Body Output**

とする。

Raw frameをGhostへ直接渡さない。

M5Stack側では低次視覚・反射に必要な意味までを扱い、人物・物体・状況の高次理解はPi5側へ分離する。

ToF4Mへ進む前に少なくとも、

- 安定撮像
- Cameraと内部I2Cの資源競合処理
- Touch / IMUを壊さない周期撮像
- Device Driver → Visionの本番境界
- 低次視覚情報を意味化できるVision層

を成立させる。

### Step 5-B｜最初の外部感覚縦貫通：ToF4M / U172

内蔵カメラ低次視覚が成立した後、最初の外部感覚として Unit ToF4M / U172 を追加する。

基本経路：

**Device → Device Driver → Adapter → Nerve Input → Semantic Neuron → Synapse → Reflex / Ghost → Behavior → Body Output**

まで一本通す。

ToF4Mの実機未検出問題はDevice Driver層として切り分け、Ghostや神経Runtimeの設計をセンサー固有問題へ巻き込まない。

ToF単独で人物を断定しない。

## Step 6｜感覚器官を順次追加

LOCK 53適用後の基本順は、

**CoreS3内蔵カメラ低次視覚 → ToF4M → TMOS PIR → ENV-Pro → 聴覚 → その他感覚**

とする。

Touch / IMUはLOCK 52のCoreS3単体DeskRobo工程で既に先行接続済みとして扱う。

各感覚は生データ取得だけで完了にせず、意味化・反射・Heart・Memory・必要時Pi5・行動まで接続する。

## Step 6-A｜聴覚先行実装（LOCK 59 / 60）

LOCK 55のM5単独生命成立を優先するため、従来Step 6の外部感覚追加順を恒久的に破棄せず、対人反応に直結するCoreS3内蔵Micを先行実装する。

基本経路：

**CoreS3 Mic → Device Driver → Voice Activity / Wake Word → Semantic Neuron → Synapse → Reflex / Ghost / Heart / Behavior → Body Output**

意味は少なくとも以下を分離する。

- `LOUD_SOUND`
- `VOICE_ACTIVITY`
- `WAKE_WORD_DETECTED`

正式ウェイクワードは **「kibi」**。

2026-09-20、CoreS3実機で `wakeModel=READY`、`VOICE_ACTIVITY`、`WAKE_WORD_DETECTED word=kibi confidence=1.00`、`WAKE_WORD -> ACCEPT`、Heart反映を複数回確認し、この縦貫通を実機通過扱いとした。

現段階の認識器はESP-SR MultiNet単一コマンドをWakeWordAdapter境界の内側で使用する。認識器の具体実装は交換可能とし、将来専用WakeNet等へ変更してもSemantic Neuron以降へ漏らさない。

全文ASR・発話理解・LLMはPi5側の責務とする。

## Step 7｜反射・Heart・Behaviorの能力拡張

Step 3〜4で置いた本番骨格へ能力を追加する。

ここで初めてGhostを作るのではない。

- Reflexの具体的反応を増やす
- Heartイベント作用を増やす
- Heart ContextによるSynapse / Behavior修飾を増やす
- Behavior Selectorの仲裁を増やす
- 対人Sessionを実装・拡張する

優先順位は、

1. 安全・強い反射
2. 意味のある対人反応
3. Heart由来の状態表現
4. 自発行動
5. idle

とする。

## Step 8｜表情・身体表現

固定表情画像の切替を中心にせず、目、視線、瞬き、左右差、首、声等を連続パラメータとして制御する。

MicroBehaviorは単純ランダムではなく、Heart状態・外界・直近履歴の影響を受ける。

「何もしない」も正式な行動とする。

## Step 9｜短期記憶・時間・生活リズム

Step 3〜4で用意したMemory / Timeの本番境界へ能力を追加する。

- 直前の出来事
- 最後にKimを見た時刻
- 会話
- 接触
- 時間帯
- 不在時間
- 反復傾向

等を現在の反応へつなげる。

Heartの動的平常値・自然回復・生活パターンも、ここまでの経験履歴と接続する。

## Step 10｜Raspberry Pi 5接続

M5Stack側の生命活動を保ったまま、高次感覚野・認知・言語・長期記憶へ接続する。

Pi5結果は直接身体を支配せず、原則Semantic Neuronとして神経系へ戻す。

Pi5停止時でも、M5Stack側の基本生命活動を継続する。

## Step 11｜ローカル会話

ASR、LLM、TTSを高次系へ接続し、Heart状態・Relationship・Memoryと会話を相互反映する。

LLMを心そのものにはしない。

## Step 12｜長期記憶能力の拡張

Memoryそのものをこの段階で初めて作るのではない。

Step 3〜4から存在するMemory本番境界へ、Pi5側を含む長期能力を追加する。

- 個別経験記憶
- 反復傾向記憶
- 特別長期記憶
- 再想起
- 現在のHeart / Relationshipによる再解釈
- 時間経過による想起されやすさ・影響力の減衰
- 類似イベント時の自動影響

を段階的に実装する。

元の出来事・当時の受け止め方は保持し、再想起時の現在の解釈は上書きせず重ねる。

強い記憶や繰り返し想起・強化された記憶は残りやすくする。

## Step 13｜既存OSS再評価

独自構造が成立した後でのみ、既存OSSを再評価する。

必要な機能がある場合は、

**構造差分確認 → 適合設計 → 変換／再実装 → 実機確認 → 衝突確認**

を行う。

既存OSSへ本体設計を合わせない。

正式参照実装は、

- Yuki Desktop Robot：Face / Eye / Mouth / Tracking / Head Motion
- AuraBot：Pi5-CoreS3 Boundary / Perception Architecture / Communication
- KariPom：MicroBehavior / Life Presence

とする。

## Step 14｜Desktop Companion v1.0

完成条件：

- 話しかけなくても存在感がある
- Kimが来ると気づく
- 必要に応じて見る
- 撫でると反応する
- 放置で退屈する
- 夜に眠くなる
- 自分から小さく行動する
- ローカルで会話できる
- 前日の出来事が一部残る
- 日々の接触で反応が少しずつ変わる
- 過去経験により反応の仕方が少しずつ変わる
- インターネットやPi5が落ちてもM5Stack側の生命活動が続く

## 現在地

`CURRENT_STATUS.md` を現在地の正本とする。

2026-09-18時点では、Step 4本番骨格とLOCK 52のCoreS3単体DeskRobo最低成立条件を実機通過済み。

LOCK 53に従い、ToF4Mより先にCoreS3内蔵カメラの低次視覚経路を実装中である。

内蔵カメラは、CoreS3実機で一回撮像、内部I2C復元、Touch / IMU継続、周期撮像、Device Driver → Vision境界、Vision側average luma取得まで確認済み。

次はVision層から製品非依存の低次視覚意味へ上げ、Semantic Neuronへ接続する。

ToF4M / U172は、その後の最初の外部感覚縦貫通対象として維持する。

## デバッグLOCK

`実行 → 修正 → 再実行` で同じ問題または同系統の問題が1回でも再発した時点で、局所修正を止め、根本原因解析へ切り替える。

## 設計を止めて実装へ戻る基準

以下が本番骨格として決まっている場合、それ以上の係数・閾値・保存件数等を机上で細分化せず実装へ戻る。

- 責務境界
- データの流れ
- 制御主体
- 優先順位
- 永続化の基本方針
- Memory / Heart / Reflex間の接続方向

実装中に「決めないと本番コードを書けない項目」が初めて現れた時だけ、追加設計を行う。

## 最終判断基準

機能数ではなく、**「そこにいる感じ」があるか**。

### LOCK 55｜M5単独生命成立を優先

Vision → Semantic Neuron → Ghost / Heart / Memory / Behavior の実機縦貫通が成立した時点から、次工程はToF4M追加を直ちに優先しない。

すでに存在する Camera / Touch / IMU を使って、

**感じる → Heartが変わる → Behaviorが自発的に選ばれる → Face / Neckへ出る**

を先に縦貫通させ、M5Stackちゃん単独の生命感を成立させる。

ToF4Mは引き続き最初の外部感覚とするが、M5単独生命成立後に接続する。

このLOCKは既存Step番号を廃止せず、現在地に対する実装優先順位を最上位ゴールへ合わせて補正する。
