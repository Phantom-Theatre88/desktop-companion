# LOCK 55｜Pi5 高次認知系の独立実装

更新日: 2026-09-19

## 決定

Raspberry Pi 5 側を、CoreS3とは独立した **高次認知系** として並行実装する。

今回のPi5実装対象は次の2系統とする。

1. **高次Vision**
   - Raspberry Pi Camera Module 3 Wide
   - 人物検出
   - 顔検出
   - Kim / Unknown 個人識別
2. **LLM**
   - 会話・状況理解
   - 意味解釈
   - 応答生成
   - 将来のASR / TTS / 高度Memoryとの接続点

## 最重要境界

Pi5やLLMをGhostそのものにはしない。

**Ghost / Heart / Time / Relationship / Behavior の主体はCoreS3側に残す。**

Pi5は「考える補助脳」であり、CoreS3へ渡すのは **Semantic Neuron相当の意味イベントだけ** とする。

今回、**CoreS3側およびGhost側のコードは変更しない。**

Pi5停止・カメラ停止・LLM停止・ネットワーク断が起きても、CoreS3側の生命ループが単独で継続する構造を維持する。

## Vision境界

Camera Module 3 Wideの画像はPi5内だけで扱う。

CoreS3へ以下を送らない。

- Raw frame
- JPEG / PNG等の画像
- 顔画像
- 顔特徴量 / embedding
- detector固有の生出力

認識処理はPi5内で、

**Camera → Person Detector → Face Detector → Identity → Semantic Event**

の順で完結させる。

初期の人物識別対象は **Kim / Unknown の2値のみ** とする。

「顔が見えた」ことと「Kimと認識した」ことを混同しない。

## LLM境界

LLMは高次認知の一器官として扱う。

LLMへGhostの主導権を渡さない。

LLMは以下を直接変更しない。

- Heart値
- Relationship値
- CoreS3 Memory
- Behavior状態
- サーボ角度
- Face描画
- Touch / IMU / Sensor Driver

LLMからCoreS3へ出す場合も、必ずPi5側で意味イベントへ変換・検証してから送る。

LLMの自然文を、そのままCoreS3の命令として実行しない。

## 初期Semantic Event

Pi5側の初期意味イベントは、少なくとも次を扱える構造にする。

### Vision

- PERSON_PRESENT
- PERSON_LOST
- FACE_PRESENT
- FACE_LOST
- KIM_RECOGNIZED
- UNKNOWN_PERSON
- IDENTITY_LOST

### Cognition / LLM

- USER_SPOKE
- USER_INTENT
- CONVERSATION_STARTED
- CONVERSATION_ENDED
- LLM_RESPONSE_READY

イベント名はPi5内部で固定し、モデルやライブラリ固有名をCoreS3へ漏らさない。

## イベント原則

- 同じ認識結果を毎フレーム送らない
- 状態変化を中心に送る
- debounce / cooldown / hysteresisをPi5側に持つ
- confidenceは判断補助のメタ情報として扱う
- 閾値は一箇所で管理し、各モジュールへ散らさない
- Unknownを無理にKimへ寄せない
- 認識できない状態は異常ではなく正式な状態として扱う
- カメラやLLMのエラーを人物・感情イベントとして偽装しない

## 実装順

Pi5はCoreS3側の現在工程と並行して、以下の順で独立実装する。

1. Pi5 OS / Python実行基盤
2. Camera Module 3 Wide 単独撮像
3. Person Detector
4. Face Detector
5. Kim / Unknown Identity
6. Semantic Event正規化
7. Pi5内イベントログで単独検証
8. LLM Service
9. Context Builder
10. LLM出力 → Semantic Event変換
11. CoreS3とのTransport境界
12. 実機統合確認

**1〜10まではCoreS3へ接続しなくても完成・検証できる構造** とする。

## 完成条件

Pi5単独で、

- Camera Module 3 Wideから人物を検出できる
- 顔の有無を判定できる
- Kim / Unknownを区別できる
- Raw画像を外へ出さず意味イベントへ変換できる
- LLMが独立Serviceとして起動できる
- LLM出力を直接命令にせず意味イベントへ変換できる
- CoreS3未接続でもログ上で一連の高次認知経路を検証できる

ことを、Pi5側第一段階の完成とする。

## 最終構造

```text
Camera Module 3 Wide
        │
        ▼
      Vision
 Person → Face → Identity
        │
        ▼
 Semantic Event ─────────┐
                          │
Text / ASR → Context → LLM│
                    │     │
                    ▼     │
              Semantic Event
                          │
                          ▼
                   Transport
                          │
                          ▼
                     CoreS3
                 Semantic Neuron
                          │
                          ▼
                       Ghost
```

Pi5は「見る・理解する・考える」を高度化する。

CoreS3は「生きる主体」であり続ける。
