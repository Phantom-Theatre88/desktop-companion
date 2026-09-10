# M5Stack Desktop Companion 実装Step 聖典 v1.0

更新日: 2026-09-11

## 0. この文書の位置づけ

XMind「Ghost Map v0.1｜M5Stack Desktop Companion 統合設計」を上位設計図として使用する。

既にXMindおよび聖典でLOCK済みの設計は維持し、ゼロベース化の対象は実装土台とする。

実装方針は以下とする。

CoreS3の素体をゼロ化
↓
XMindで既に定義した構造を、下から順に実装へ落とす
↓
未決課題だけ、その必要なタイミングで決める

既存OSSを親Repo・母体・実装基盤にはしない。

初期構成は、

CoreS3 + Arduino + M5Unified + M5GFX + 独自コード

とする。

---

## Step 0｜CoreS3を素体へ戻す

今入っている旧実装・OSS依存・混在ライブラリを外す。

基準は、

**CoreS3 + Arduino + M5Unified + M5GFX + 独自コード**

だけ。

完成条件は、旧コードに依存せず起動できること。

---

## Step 1｜素体検査

CoreS3そのものを確認する。

- Display
- Touch
- IMU
- Mic
- Speaker
- 内蔵近接
- Camera
- I2C / GPIO
- PSRAM / メモリ

ここではM5Stackちゃんらしい動作は作らない。

**単体動作＋共存確認**まで行う。

---

## Step 2｜神経系の最小骨格を作る

XMindのLOCKそのまま。

**外界  
→ 感覚器官  
→ Raw Perception  
→ Perception Integration  
→ Nerve Input  
→ Semantic Neuron**

さらに、

**Neuron＝意味を運ぶ  
Synapse＝意味を結ぶ  
神経回路＝全体をつなぐ**

をコード上の基本構造にする。

まだHeartは作らない。

---

## Step 3｜ToF4Mを1本目の神経として縦貫通

これも既存LOCK通り。

**ToF4M U172  
→ Raw Perception  
→ Adapter / Integration  
→ Nerve Input  
→ Semantic Neuron**

ここで、

- APPROACH
- NEAR
- LEAVE

などまで通す。

**最初の1本を最後まで通して、神経構造そのものを検証する。**

---

## Step 4｜表情エンジン

既存の実装順LOCKへ戻る。

固定顔ではなく、

- Eyes
- 視線
- 瞬き
- 開き
- 左右差
- 動作速度

などを独自制御する。

ここではまだ「感情表現」ではなく、  
**上位から自由に身体出力を要求できる状態**を作る。

---

## Step 5｜反射

ToFのSemantic Neuronから、

**APPROACH  
→ 視線を向ける**

のような即時反応を作る。

原則は既存LOCK通り、

**HeartやPi5を待たない。**

反射後に出来事をHeart側へ渡せる構造にする。

---

## Step 6｜Heart Engine v0.1

既決の6状態から開始。

- mood
- affection
- curiosity
- boredom
- sleepiness
- attention

ここで初めて、

**感覚 → Heart → 表現**

がつながる。

ただし、ここで未決課題3  
**「Heart Engineの数値仕様」**  
を必要な範囲だけ決める。

---

## Step 7｜行動仲裁

既にLOCK済みの順番を実装する。

1. 安全・強い反射
2. 意味のある対人反応
3. Heart由来の状態表現
4. 自発行動
5. idle

さらに、

**全身一括占有しない  
競合する身体出力だけBehavior Selectorが仲裁する**

もここで実装する。

---

## Step 8｜自発行動

Heart＋外界＋直近履歴から行動を選ぶ。

- 見る
- ちらっと見る
- 首を傾ける
- 何もしない
- 眠る
- 起きる

など。

完全ランダムにはしない。

---

## Step 9｜短期記憶・生活リズム

XMindの順番通り。

- 直前の出来事
- 最後に対象を見た時刻
- 最近の反応
- 朝
- 昼
- 夕方
- 夜
- 深夜

を入れる。

ここで未決課題4  
**「時間の設計」**  
を必要な範囲だけ決める。

---

## Step 10｜感覚器官を1本ずつ追加

ここから広げる。

順番は今までのLOCKを使う。

**TMOS PIR  
→ ENV-Pro  
→ Touch  
→ IMU  
→ Mic**

全部、

**Device固有値  
→ Raw Perception  
→ Integration  
→ Nerve Input  
→ Semantic Neuron**

で統一。

追加するたびに共存試験をする。

---

## Step 11｜対人認識

既にXMindで決めた、

- Kim
- 既知の他人
- 未知の人
- 認識不能

を実装へ落とす。

流れも既決。

**PERSON_NEAR  
→ 注視  
→ IDENTIFYING  
→ 認識結果  
→ 対象別反応**

---

## Step 12｜対人Session

会話だけではなく、

**視線・表情・相槌・待ち・発話**

を同じ対人文脈として保持する。

反射割り込み後もSessionを消さず、条件が残っていれば復帰する。

---

## Step 13｜視覚・Pi5高次認知

ここで、

**M5側＝低次・反射寄り  
Pi5側＝重い認知**

をつなぐ。

Pi5から返すものも身体命令ではなく、

**Semantic Neuron**

にする。

Pi5停止時でもM5側の生命活動は継続。

---

## Step 14｜聴覚・会話

ここで初めて本格的な会話。

会話はHeartより上位ではない。

**身体・神経・Heart・Sessionの中に会話を入れる。**

---

## Step 15｜長期記憶・関係性

ここで未決課題5  
**「記憶の境界」**  
を決める。

過去経験が、

- Heart
- 行動
- 会話
- 対人反応

へ影響するようにする。

---

## Step 16｜異常時と存在評価

最後に残っている未決課題。

**6｜故障・切断時の人格**  
**7｜「そこにいる感じ」の評価方法**

を決める。

ここまで来て初めて、

**M5Stackちゃん v1.0**

の完成判定をする。

---

## 実装順LOCK

**素体をゼロ化  
→ 神経を1本通す  
→ 身体と生命感  
→ Heart  
→ 行動  
→ 記憶  
→ 高次認知  
→ 会話  
→ 関係性**

XMindで積み上げた設計を上位設計として維持し、そのまま実装ロードマップへ落とす。

未決事項は先回りして増やさず、そのStepで必要になった時点でのみ決める。
