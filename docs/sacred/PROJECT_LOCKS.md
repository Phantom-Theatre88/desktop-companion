# M5Stack Desktop Companion — 重要LOCK集

更新日: 2026-09-17

この文書は、プロジェクト全体へ常時適用する正式LOCKの索引である。

個別LOCK文書が存在する項目は個別文書の詳細を正式内容とし、本索引は全体を見失わないための一覧とする。内容が競合した場合は、より新しいLOCKを優先する。

## LOCK 1｜最上位ゴール

M5Stackを、単なる音声AI端末ではなく、

**「机の上に常にいて、周囲を感じ、反応し、気分が変わり、少しずつ関係性が育つ相棒」**

として成立させる。

評価基準は機能数ではなく、**「そこにいる感じ」があるか**。

## LOCK 2｜実装基盤はゼロベース

既存OSSを親Repoにしない。

初期構成は以下だけとする。

- M5Stack CoreS3
- Arduino
- M5Unified
- M5GFX
- 独自コード

`stack-chan-ko`、M5Stack/StackChan、Dotty StackChan、stackchan-local、M5Stack_RoboEyes、Xiaozhi、旧Yuki実装等は初期基盤へ入れない。

最初にCoreS3公式APIだけで各ハード機能を単独確認し、素体の正常性を確立する。

以後の基本構造は、

**Hardware → Device Driver → Adapter → Nerve Input → M5Stackちゃん**

とする。

## LOCK 3｜旧実装の扱い

リポジトリ内の旧Yuki／StackChan系コード、patch、upstream、旧ビルドスクリプト等は削除せず、**LEGACY参考資産**として保持する。

ただし、

- 現行基盤ではない
- 正常動作の基準にしない
- 親Repoにしない
- 新コードへ自動的に移植しない
- 旧コードの都合で新設計を曲げない

ことを固定する。

## LOCK 4｜ハードウェア単体確認を先行

初期実装ではM5Stackちゃんらしい機能を作らない。

まず、

**DISPLAY → TOUCH → IMU → PROXIMITY → MIC → SPEAKER → CAMERA → その他必要ハード**

を一つずつ独立確認する。

各確認では、対象ハード以外の機能を極力混ぜず、どの層が正常かを明確にする。

## LOCK 5｜神経入力の共通仕様

製品固有の出力を、そのまま神経へ渡さない。

基本構造は、

**Device → Adapter → Nerve Input**

とする。

距離値・座標・加速度等の生値は前段で扱い、神経へは `NEAR`、`APPROACH`、`LEAVE`、`STROKE_DETECTED` 等の製品非依存の共通意味入力を渡す。

カメラは、

**Camera → Vision / Recognition → Nerve Input**

とする。

## LOCK 6｜Heart Engine

LLMそのものを「心」にしない。

Heart Engineは少なくとも、mood、affection、curiosity、boredom、sleepiness、attentionと、その時間変化・イベント変化・行動選択・表情反映・記憶接続を担当する。

Pi5やLLMが停止していても、M5Stack側の身体・感情・基本行動は生き続ける。

## LOCK 7｜実装順

LLMを先に中心へ置かない。

先に、

**見る → 感じる → 反応する → 気分が変わる → 自分から行動する**

を成立させ、その後に言葉を与える。

ただし実コードでは、その前にCoreS3のハードウェア単体確認を完了させる。

## LOCK 8｜デバッグの1回ループルール

`実行 → 修正 → 再実行` で同じ問題または同系統の問題が1回でも再発した時点で、局所修正を止める。

壊れている層・設計前提・データフロー・制御主体・資源競合・上位ゴールとの整合性を根本から見直す。

## LOCK 9｜OSS・ドナー利用

OSSは初期基盤へ入れない。

CoreS3公式環境と独自層が正常に成立した後、必要性が明確な機能だけを候補として評価する。

採用時は、

**ドナー機能 → 構造差分確認 → 適合設計 → 変換／再実装 → 実機確認 → 衝突確認**

の順を必須とする。

「ドナーで動く」は「Desktop Companionで適合する」と同義ではない。

## LOCK 10｜長期プロジェクト判断順

すべての設計・実装・デバッグ・完成判定は、

**最終ゴール → 全体構造 → 現在の小ゴール**

の順で判断する。

## LOCK 11｜M5Stack＋Raspberry Pi 5の役割

- M5Stack：身体＋生命維持できる低次脳＋反射系＋Heartの常時稼働部分
- Raspberry Pi 5：高次感覚野＋認知脳＋言語＋長期記憶

Pi5停止時でもM5Stack側の基本生命活動を継続する。

## LOCK 12｜視覚は2カメラ構成

- M5Stack側カメラ：反射の目
- Raspberry Pi Camera Module 3 Wide：理解する目

Pi5側高次視覚カメラは Raspberry Pi Camera Module 3 Wide を採用LOCKする。

## LOCK 13｜購入済み感覚器官

- M5Stack Unit ToF4M / U172
- M5Stack Unit TMOS PIR / U185
- M5Stack ENV-Pro / U169
- M5Stack Unit Hub / U006
- Raspberry Pi Camera Module 3 Wide
- Raspberry Pi 5用SSD

感覚器官の実装順は、CoreS3本体の単体確認完了後に改めて開始する。

## LOCK 14｜神経系全体構造

上位構造は、

**外界 → 感覚器官 → Raw Perception → Perception Integration → Semantic Neuron → Synapse → Reflex / Heart / Memory / Pi5高次認知 → Behavior Selector → 身体出力**

とする。

神経系は全体構造を先に固定し、その後に感覚1本ずつ縦貫通実装する。

最初の外部感覚の縦貫通対象は ToF4M / U172 とする。ただし、CoreS3素体確認より先には進めない。

## LOCK 15｜行動仲裁・対人Session・身体制御

優先順位は、

1. 安全・強い反射
2. 意味のある対人反応
3. Heart由来の状態表現
4. 自発行動
5. idle

とする。

対人関与は単発行動ではなく対人Sessionとして保持する。身体全体を一括占有せず、競合した身体出力だけBehavior Selectorが仲裁する。

## LOCK 16｜旧方針の失効

以下の旧方針は正式に失効する。

- `stack-chan-ko` を親Repoとする
- 旧Yuki／StackChanファームを現行実装の土台とする
- ESP-IDF 5.5.4旧Yuki構成を現行の必須基盤とする
- ToF4M縦貫通実装をCoreS3素体確認より先に進める
- RoboEyes、Xiaozhi、Dotty、stackchan-local等を最初から組み込む

このLOCK 16に反する過去文書・過去コード・会話記録は、歴史資料またはLEGACY参考資産としてのみ扱う。

## LOCK 17｜Desktop Robo参照実装の正式採用

Ghost本体は独自実装のままとし、他のDesktop Roboを親Repo・基盤・完成形として採用しない。

一方で、既に実装済みの優れた機能を0から再発明しないため、以下を**正式な参照実装（ドナー）**として採用する。

### Yuki Desktop Robot

主な参照範囲：

- Face / Eye / Mouth
- 顔検出後の視線追従
- 首のPan/Tilt追従
- 顔位置の平滑化
- デッドゾーン、移動量制限、更新周期制限
- 顔消失時の追従解除
- 会話状態・VADと身体動作の優先制御
- ジェスチャー検出

YukiのESP-IDF / LVGL / StackChan構造そのものは採用しない。アルゴリズムと実装知見を現行のArduino + M5Unified + M5GFX + 独自構造へ適合させて再実装する。

### AuraBot

主な参照範囲：

- Raspberry Pi 5 とESP32系デバイスの役割分担
- Vision / Speech / LLM / Presence等の高次処理と身体側の分離
- Pi5 ↔ CoreS3通信設計
- MQTT / WebSocket等を含む通信境界の考え方
- Perception構造と責任分界

AuraBotの構造をそのまま複製せず、Desktop Companionの神経系・Semantic Neuron・Ghost構造を優先する。

### KariPom

主な参照範囲：

- 待機中の微小な動き
- 瞬き、視線、小さな身体動作
- 発話・音声に連動した表現
- 「何もしていない時にも存在している」ためのMicroBehavior

MicroBehaviorはGhost / Behavior配下の生命感表現として扱い、単純ランダム動作だけで人格を表現しない。

### 採用原則

参照実装から取り込む対象は主として、

- Perception
- Body
- Tracking
- Communication Boundary
- MicroBehavior

とする。

**Heart / Memory / Time / Relationship / Behaviorの中核判断はDesktop Companion独自のGhostを正本とする。**

参照コードを直接コピーするか、アルゴリズムのみ再実装するかは、ライセンス・依存関係・現行構造との差分を確認して個別決定する。

このLOCKはLOCK 2・LOCK 3・LOCK 9を上書きしない。既存OSSを親Repoにしないゼロベース基盤は維持する。

## LOCK 18｜Heart Engine数値表現

Heart Engineの `mood / affection / curiosity / boredom / sleepiness / attention` は、以下の共通仕様で扱う。

- 内部値：`0.0〜1.0`
- 表示・デバッグ値：`0〜100`
- イベント1回あたりの変化量：原則 `-0.1〜+0.1`
- 計算後は必ず `0.0〜1.0` の範囲へ収める

表示値は内部値を人間が確認しやすくするための表現であり、Heart Engine内部計算は `0.0〜1.0` を正本とする。

## LOCK 19｜Heart Engine起動・永続化

Heart Engineは、通常起動時に前回終了時の状態を引き継ぐ。

- 初回起動または明示的な初期化時のみ、項目ごとの基準初期値を使用する
- 2回目以降の通常起動では、保存済みのHeart状態から再開する
- Heartの初期値設定と、ToF・IMU等のセンサーキャリブレーションは別処理とする
- センサーのキャリブレーション結果によってHeartの保存値を直接初期化しない

これにより、再起動しても関係性や内面状態が毎回ゼロへ戻らない構造とする。

## LOCK 20｜Heart再起動時の状態復元

Heart状態は、すべてを同じ方法で前回値へ復元しない。

- `affection`：長期関係状態として永続し、前回値を引き継ぐ
- `mood`：前回値を参照するが、起動時には平常値方向へ減衰させて復元する
- `curiosity`：前回値を参照するが、起動時には基準値方向へ減衰させて復元する
- `boredom`：保存値をそのまま復元せず、停止からの経過時間・現在状況を使って起動時に再計算する
- `sleepiness`：保存値をそのまま復元せず、現在時刻・生活リズムを使って起動時に再計算する
- `attention`：一時的な注意状態として、起動時には平常値へ戻す

一時的な感情や注意を電源断の瞬間のまま凍結保存しない。

一方で、`affection` のような長期関係状態は再起動で失わない。

## LOCK 21｜Heart平常値の長期変化

Heartの平常値は固定定数にしない。

- `mood / curiosity / attention` 等の平常値は、長期的な経験・関係性・生活履歴により少しずつ変化できる
- 起動時の復元や一時感情の減衰先は、その時点での各項目の平常値を使う
- 平常値の変化は一時イベントより十分遅くする
- 平常値はGhostの長期的な性格・傾向の一部として扱う

## LOCK 22｜Heart平常値の変化速度

Heartの平常値は、短時間のイベントではなく**数日〜数週間単位**でゆっくり変化させる。

一時的な出来事は現在値へ反映し、同種の経験が繰り返され長期傾向として蓄積した場合に平常値へ反映する。

## LOCK 23｜Heart平常値の学習材料

Heartの長期的な平常値変化は、単純なイベント回数だけで決めない。

- Semantic Neuronとして入ったイベントの種類・回数
- 継続時間と最近の傾向
- Relationship / Heart状態
- 時間帯・曜日・よく一緒にいる時間
- 起動時間・不在時間・日々の反復傾向

等を組み合わせる。

## LOCK 24｜Heart平常値は良化・悪化の両方向へ変化する

好ましい経験で良い方向へ、長期間の不在・嫌がる刺激・生活リズムの乱れ等の反復で悪い方向へも変化できる。

単発イベントで人格傾向を急変させない。

## LOCK 25｜Heartの回復性

悪化したHeart状態・平常値は、自然回復と良い経験の両方で回復できる。

過去経験を即座に帳消しにはせず、永久に悪化状態へ固定される構造にもしない。

## LOCK 26｜Heart回復先は動的な平常値

Heartが一時状態から回復する時は、固定初期値ではなく、その時点のRelationship・長期履歴を反映した動的平常値へ戻る。

## LOCK 27｜Heart初回起動時の基礎人格

完全な初回起動時は、少し好奇心が高く、少し人懐っこい基礎人格を持たせる。

初期人格は固定された完成人格ではなく、経験・関係性・生活履歴で育つ。

## LOCK 28｜Heart初回起動時の具体値

- `mood = 0.60`
- `affection = 0.55`
- `curiosity = 0.65`
- `boredom = 0.15`
- `sleepiness = 0.15`
- `attention = 0.50`

初期人格の見え方は、

**「初対面でも人を嫌っていない。好奇心が強めで、周囲をよく見る。少しだけ元気で、反応も一拍早い。ただし落ち着きはある。」**

とする。

## LOCK 29｜Heartイベント強度は3段階

Heartへ影響するイベントは、基本強度を**弱・中・強**で扱う。

イベント種別は作用先と方向を定義し、強度は作用量の段階を表す。同種イベントでも継続時間・文脈・Relationship・直前状態等により分類は変わり得る。

## LOCK 30｜3段階ラベルと実際の連続変化量を分離

弱・中・強は人間向けの分類ラベルであり、固定数値そのものではない。

内部の実際の変化量は連続値とし、同じラベルでも内容・継続時間・Relationship・Heart・履歴・文脈により変化してよい。

## LOCK 31｜Heart影響量の基本優先順

可変なHeart影響量を考える時は、基本的に、

1. イベント内容そのもの
2. Relationship
3. 現在のHeart状態
4. 継続時間

の順で意味を重視する。

これは固定係数順ではなく、解釈原則である。

## LOCK 32｜1イベントは主作用＋複数副作用を持てる

1つのイベントをHeart項目1つだけに固定しない。

主作用を1つ持ち、意味上自然な範囲で複数の副作用を持てる。

## LOCK 33｜Heart Contextで神経・行動を修飾

Heartは身体へ直接「笑う」「首を向ける」等の命令を出さない。

Heart状態はHeart Contextとして神経回路・Synapse・Behavior Selector側の判断を修飾する。

## LOCK 34｜Heart Contextはread-only snapshot

Heart内部状態を更新する主体はHeart Engineだけとする。

Synapse、Behavior Selector、Reflex、Memory、Pi5等はHeart内部を直接書き換えず、read-only snapshotを参照する。

## LOCK 35｜1イベント中はHeart Context snapshotを固定

1イベント処理単位では、開始時点のHeart Contextを取得し、そのイベント処理中は同じsnapshotを使う。

処理途中のHeart変化で同一イベント判断を揺らさない。

## LOCK 36｜近接した複数イベントは優先処理

複数イベントを単純FIFOだけで扱わない。

安全・強い反射等の重要度が高いイベントを先に処理し、低優先イベントは必要に応じて後段へ残す。

## LOCK 37｜遅延イベントは現在でも意味があるか再評価

待機したイベントは、処理時点でまだ意味があるかを再評価する。

身体反応として古くなったイベントは反応対象から外してよい。

## LOCK 38｜反応対象と記憶対象を分離

身体反応として古くなったことと、経験としてMemoryへ残す価値は別に扱う。

身体反応を捨てても、経験まで必ず捨てるとは限らない。

## LOCK 39｜Memoryは出来事と受け止め方を記録できる

意味のある経験について、何が起きたかだけでなく、その時のHeart ContextやGhostがどう受け取ったかを結び付けて記憶できる。

## LOCK 40｜反復イベントは個別記憶＋傾向記憶

同種イベントをすべて無期限に個別保存しない。

必要な個別記憶は残しつつ、反復は「最近多い」「いつもの時間帯」等の傾向として集約できる。

## LOCK 41｜傾向記憶は時間減衰し、新経験で再強化される

反復傾向は固定値ではなく、最近の経験で強まり、パターンが止まれば徐々に弱くなる。

## LOCK 42｜特別な出来事は個別長期記憶として残せる

非常に嬉しい、怖い、長期不在後の再会、初回、節目、関係性上重要等の出来事は、傾向へ平均化せず個別の長期記憶として保持できる。

すべての強い出来事を永久保存するわけではない。

## LOCK 43｜長期記憶は想起されやすさ・影響力が減衰する

長期記憶データを機械的に毎秒削るのではなく、時間経過により「想起されやすさ」「現在への影響力」が弱まり得る構造とする。

強い記憶、繰り返し想起された記憶、再強化された記憶は残りやすい。

## LOCK 44｜原記憶を保持しつつ現在の再解釈を重ねる

元の出来事と当時の受け止め方は保持する。

再想起時には、現在のHeart / Relationshipによる新しい解釈を上書きせず重ねられる。

## LOCK 45｜強い長期記憶は類似イベント時に自動影響できる

強い長期記憶は、明示的な想起処理を待たず、似た出来事が起きた時に現在のHeart / Behaviorへ影響できる。

## LOCK 46｜危険・恐怖系長期記憶はReflex / Synapseへ先行修飾できる

危険・恐怖・強い警戒に関わる記憶は、Heartで意識的に解釈する前にReflex / Synapse側へ影響し、身体反応を先行させてよい。

通常の好み・親しさ・懐かしさ等は、Heart / Relationshipを通じてBehaviorへ影響することを基本とする。

## LOCK 47｜Reflex身体反応結果をHeartへ返す

Reflexが身体を先に動かした場合、その反応結果もHeartへフィードバックする。

身体反応と内面を切り離さない。

## LOCK 48｜Reflexの成否・危険継続状態までHeartへ返す

Reflexが起きた事実だけでなく、回避できたか、まだ危険が続いているか等の結果までHeart更新材料にする。

## LOCK 49｜必要なReflex結果をMemoryへ記録する

Reflexの結果も、今後の経験・危険判断・傾向形成に意味があるものはMemoryへ残せる。

## LOCK 50｜危険経験の反復でReflex感度が変化できる

繰り返し危険だった刺激には、より早く・より強く反応する方向へReflex感度を変化させてよい。

長期間安全な状態が続けば、その感度は徐々に戻り得る。

具体的な閾値・増減率・時間定数は実装・実機観察なしに先行固定しない。

## LOCK 51｜Heart永続化保存方式

Heartの永続化は、**NVS主保存＋microSDバックアップ**の二重化構成とする。

- CoreS3内部NVSを主保存先とし、Arduino環境では `Preferences` を使用する
- 通常起動時はNVSを第一の復元元とする
- microSDは独立したバックアップ／復旧経路とする
- microSD未挿入・マウント失敗時でも、NVSからのHeart復元とM5Stack側の基本生命活動を継続する
- NVS保存異常・破損時にmicroSDバックアップから復旧できる構造を目指す
- 保存周期、バックアップ世代数、ファイル形式、破損判定方式、細かな復旧優先順位は実装上必要になるまで先行固定しない

詳細は `LOCK51_Heart永続化保存方式.md` を正とする。

## 実装順に関する最新統合LOCK

GhostはStep後半で初めて追加する機能ではない。

**Heart / Memory / Time / Relationship / Behaviorの本番骨格を、Semantic Neuron / Synapse / GhostCore / Reflex / Runtimeの成立段階から接続する。**

後段ではGhostそのものを作り直さず、感覚器官、記憶量、高次認知、行動能力を段階的に追加する。

詳細な最新作業順は `M5Stack Desktop Companion 実装Step聖典 v0.3.md` の本文（内容上v0.4へ更新済み）と `CURRENT_STATUS.md` を正とする。

## 設計停止・実装復帰LOCK

責務境界、データフロー、制御主体、優先順位、永続化方針、Heart / Memory / Reflex間の接続方向が本番骨格として決まっている場合、係数・閾値・保存件数等の派生質問を机上で増やさない。

実装中に「これを決めないと本番コードを書けない」という具体的な未決事項が現れた時だけ、追加設計へ戻る。

## LOCK 54｜Desktop Companion 今後の大工程

現在の実装状態からDesktop Companion v1.0へ向かう上位ロードマップを、以下の順で固定する。

**直近実機確認 → A「感じる」→ B「気分が変わる」→ C「自分から行動する」→ D「昨日と今日をつなぐ」→ E「賢くする」**

直近実機確認は、すでに実装済みの、

**Camera → Vision → Semantic Neuron → Ghost / Memory / Behavior**

をCoreS3実機で通すこと。

Aでは感覚器官をSemantic Neuronまで意味化してGhostへ接続する。

BではHeart Engineへイベント・時間による実際の内面変化を接続する。

CではHeart / Memory / Time / 感覚から理由のある自発行動を選択し、Face・視線・内蔵首等へ出力する。

DではTime / Memory / Relationshipを実際に機能させ、過去経験と長期関係を現在へ接続する。

EではPi5を高次認知・言語・高次記憶として接続する。Pi5やLLMをGhostそのものにはしない。

このLOCKは既存の実装Step番号を置き換えず、最上位ゴールから見た大工程へ束ね直した上位ロードマップとする。

詳細は `LOCK54_DesktopCompanion今後の大工程.md` を正とする。
