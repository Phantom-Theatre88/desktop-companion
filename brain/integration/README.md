# AKARI Brain Step 1｜日本語説明書・Mac検証手順

**現在地：実装コードは保存済み。OpenClawとLangGraphの実機連携は未検証。Step 1完成ではありません。**

## 何を作ったか

OpenClawが記憶から整理した目的・文脈をLangGraphへ委任し、LangGraphがローカルLLMで実行して、進捗・結果をOpenClawから照会できる構造です。

| 担当 | 役割 |
| --- | --- |
| OpenClaw | 対話・長期記憶・依頼理解 |
| LangGraph | 仕事の実行・状態管理・失敗時の再試行・標準チェックポイント |
| Ollama | 最初の試験用ローカルLLM |
| Ghost・NERVA・Character | 今回は未接続。既存LOCKを維持 |

OpenClawの記憶の**自動検索・自動書き戻しはまだ未実装**です。現在のプラグインではOpenClaw側が取得した文脈をcontextとして明示的に渡します。

## 収録ファイル

- service.py：LangGraphサービス（受付・実行・結果確認・再開）
- requirements.txt：Python依存ライブラリ
- openclaw-plugin/：OpenClaw連携の試作プラグイン
- tests/：静的テスト
- README.md：本書

## A｜Macでサービスを起動

前提：Python 3.11以上、Ollamaと利用可能なローカルモデル。初回のライブラリ導入にはインターネット接続が必要です。

ターミナル①で実行（GitHubのリポジトリから操作する場合）：

~~~bash
cd brain/integration
python3 --version
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install -r requirements.txt
ollama list
~~~

次の「モデル名」をollama listで表示された実在の名前に置き換えます。

~~~bash
export AKARI_OLLAMA_MODEL='モデル名'
export AKARI_API_TOKEN="$(python3 -c 'import secrets; print(secrets.token_hex(24))')"
export LANGGRAPH_STRICT_MSGPACK=true
uvicorn service:app --host 127.0.0.1 --port 8765
~~~

**期待結果：** Uvicornが127.0.0.1:8765で起動。ターミナル①は閉じません。

## B｜別ターミナルから仕事を送る

認証トークンを別のターミナルでも設定する必要があります。ターミナル①で次の値を表示し、ターミナル②に設定します。トークンはチャットやGitHubへ投稿しないでください。

~~~bash
printf '%s\n' "$AKARI_API_TOKEN"
~~~

ターミナル②：

~~~bash
export AKARI_API_TOKEN='ターミナル①で確認した値'
curl -sS -X POST http://127.0.0.1:8765/jobs \
  -H "X-AKARI-Token: $AKARI_API_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"job_id":"akari-test-001","goal":"この情報を日本語で3行に整理して","context":"8bit図鑑は既存の完成を優先。","constraints":["推測を事実と書かない"]}'
curl -sS -H "X-AKARI-Token: $AKARI_API_TOKEN" \
  http://127.0.0.1:8765/jobs/akari-test-001
~~~

- queued：受け付けただけ。成功ではありません
- running / verifying：処理中
- completed：空でない文章が得られた（内容の正しさまでは未検証）
- failed：モデル未設定や通信失敗など
- duplicate: true：同じジョブIDで再投稿されたとき

処理が終わらなければ、最後のcurlをもう一度実行してください。

## C｜再起動して状態を確認

ターミナル①をcontrol+Cで終了。同じディレクトリから、同じ環境変数を設定して再起動してください。保存先は .akari-local-data/jobs.sqlite です。その後、BのGETを再実行し、以前のjob_idの状態が見えるか確認します。

**重要：** 突然停止時の完全復旧や外部ファイル操作の二重実行防止は未検証です。現在のワーカーは文章生成のみで、ファイル書込みは行いません。

## D｜OpenClawプラグインを試す

LangGraphサービス単体が動いたら、次を実行：

~~~bash
cd openclaw-plugin
npm install
npm run build
~~~

使用中のOpenClawの公式手順に従ってローカルプラグインを登録し、サービスと同じAKARI_API_TOKENをOpenClawのプロセスに設定してください。

- akari_delegate：目的・文脈・制約を渡し、job_idを取得
- akari_job_status：作業状態と結果を取得
- akari_resume：保存済みの中断ジョブを再開

**OpenClawのSDKバージョンとの互換性、ビルド、実際の読み込みは未検証です。** バージョンが合わない場合、出たエラーを確認して修正します。

## E｜基本テスト

~~~bash
cd brain/integration
python3 -m unittest discover -s tests -v
python3 -m compileall -q service.py
~~~

これらは基本契約・構文の確認であり、実際の連携成功を保証するものではありません。

## 完成判定チェック

- [ ] サービス起動
- [ ] 仕事の登録、結果取得、失敗表示
- [ ] 同じIDの重複処理防止
- [ ] 再起動後の状態参照と中断再開
- [ ] OpenClawプラグインのビルド・読み込み
- [ ] OpenClawから委任・照会でき、仕事中も会話継続
- [ ] OpenClawの長期記憶を参照し、結果を記憶に戻せる

**現時点では実機による項目の合格は未確認。**

正式LOCKは変更していません。旧brain/step1/canon_guard.pyは正本参照の補助コードとして維持しています。
