# Pi5 High Cognition Architecture v0.1

更新日: 2026-09-19
対応LOCK: LOCK 55

## 1. 目的

Raspberry Pi 5単独で高次VisionとLLMを成立させる。

最初の実機ゴールは、

**Camera Module 3 Wide → 人物検出 → 顔検出 → Kim / Unknown認識 → Semantic Event**

と、

**Text入力 → Context → LLM → Semantic Event**

をPi5内で独立して通すこと。

CoreS3 / Ghostの変更は行わない。

## 2. ディレクトリ構成

```text
pi5/
├─ README.md
├─ pyproject.toml
├─ config/
│  └─ default.yaml
├─ app/
│  ├─ main.py
│  ├─ vision/
│  │  ├─ camera.py
│  │  ├─ person_detector.py
│  │  ├─ face_detector.py
│  │  ├─ identity.py
│  │  └─ vision_pipeline.py
│  ├─ cognition/
│  │  ├─ llm_service.py
│  │  ├─ context_builder.py
│  │  └─ cognition_pipeline.py
│  ├─ semantic/
│  │  ├─ event.py
│  │  ├─ event_types.py
│  │  └─ normalizer.py
│  ├─ transport/
│  │  ├─ publisher.py
│  │  └─ protocol.py
│  └─ common/
│     ├─ clock.py
│     └─ logging.py
├─ tests/
│  ├─ vision/
│  ├─ cognition/
│  └─ semantic/
├─ models/
│  └─ .gitkeep
└─ data/
   ├─ identities/
   │  └─ .gitkeep
   └─ runtime/
      └─ .gitkeep
```

## 3. レイヤ責務

### camera.py
Camera Module 3 Wideとの接続だけを担当する。

人物・顔・個人識別を判断しない。

### person_detector.py
画像中の人物候補を検出する。

Kimかどうかは判断しない。

### face_detector.py
人物候補または画像から顔候補を検出する。

個人識別は行わない。

### identity.py
顔候補からIdentityを判断する。

初期出力は以下だけ。

- KIM
- UNKNOWN
- NONE

### vision_pipeline.py
Vision各段を接続し、状態変化をSemantic Eventへ渡す。

### llm_service.py
LLM推論だけを担当する。

CoreS3の制御命令を生成する責務を持たない。

### context_builder.py
LLMへ渡す文脈を組み立てる。

将来、ASR / Memory / Vision semantic stateをここへ統合する。

Raw camera frameはLLM contextへ常時投入しない。

### cognition_pipeline.py
入力文脈 → LLM → 意味抽出を接続する。

### semantic/
Pi5内部の製品・モデル依存出力を、製品非依存の意味イベントへ正規化する。

### transport/
Semantic Eventだけを外部へ送る。

Vision / LLMの内部オブジェクトを直接送らない。

## 4. Semantic Event共通形

初期形は次を基準とする。

```json
{
  "version": 1,
  "source": "pi5.vision",
  "event": "KIM_RECOGNIZED",
  "entity": "kim",
  "confidence": 0.94,
  "timestamp_ms": 0
}
```

LLM系では、

```json
{
  "version": 1,
  "source": "pi5.cognition",
  "event": "USER_INTENT",
  "intent": "greeting",
  "confidence": 0.88,
  "timestamp_ms": 0
}
```

とする。

必須項目と任意項目は `semantic/event.py` で一元管理する。

## 5. Identityルール

初期はKim一人だけをKnown Identityとして登録する。

Unknown人物を個別に追跡・命名しない。

Kim登録データはPi5内に保持し、Gitへコミットしない。

顔画像・embeddingをCoreS3へ送らない。

認識閾値未満はUNKNOWNとする。

「たぶんKim」をKim確定として扱わない。

## 6. LLMルール

LLMは交換可能なServiceとする。

上位コードが特定モデル名・特定APIへ依存しない構造にする。

LLM入力はContext Builder経由に限定する。

LLM出力はCognition Pipelineで受け、必要な情報だけSemantic Eventへ変換する。

LLM自然文をそのままBehavior命令・サーボ命令・Heart更新へ接続しない。

LLM異常時はSemantic層へエラー状態を返し、高次認知だけを停止する。

## 7. Privacy / Dataルール

以下はGitへ入れない。

- Kimの顔画像
- 顔embedding
- 音声録音
- 会話ログの実データ
- API key
- token
- 個人Memory実データ
- モデル本体

設定例・schema・ダミーデータのみRepoに置く。

## 8. Fail-safe

Pi5プロセス停止時、CoreS3へ連続的な異常イベントを送らない。

再起動時は現在状態を再取得してから状態変化イベントを開始する。

前回起動時の「Kimがいる」状態を盲目的に復元しない。

Vision停止と「PERSON_LOST」を同一視しない。

LLM停止と「会話終了」を同一視しない。

## 9. テスト境界

各段を単独テスト可能にする。

- Cameraなしで保存テスト画像を使える
- Person Detector単独
- Face Detector単独
- Identity単独
- Semantic Normalizer単独
- LLM Service単独
- Cognition Pipeline単独
- Transportなしでstdout / logへ出せる

実機統合前にPi5内で完結した経路を確認する。

## 10. 今回触らない範囲

- CoreS3 firmware
- Ghost
- Heart
- Relationship
- CoreS3 Memory
- Behavior
- Face
- Neck
- Touch
- IMU
- ToF4M
- TMOS PIR
- ENV-Pro

Pi5側で必要になっても、これらのコードを変更して解決しない。

境界仕様として必要事項を記録し、CoreS3側変更は別工程で判断する。
