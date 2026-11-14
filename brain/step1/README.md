# AKARI Brain Step 1｜正本確認ガード（最小試作）

**状態：Pythonソース・テストをGitHubに保存。統合運用・オンライン実行は未検証。**

## 目的

LLMの会話記憶だけに依存せず、AKARI計画に適用される既存のGitHub正式LOCKを実ファイルから確認し、旧M5Stack／Pi5の技術前提をAKARIの最終ハードウェアへ誤って転用しないための**読み取り専用の最小試作**。

上位LOCK：`docs/sacred/LOCK_DESKTOP_AI_THREE_PILLARS_IMPLEMENTATION_2026-11-09.md`、`docs/sacred/LOCK_AKARI_BRAIN_OPENCLAW_LANGGRAPH_ARCHITECTURE_2026-11-14.md`。

## 実行

Python標準ライブラリのみ。ネットワーク接続が必要。

```bash
cd brain/step1
python3 -m unittest -v test_canon_guard
python3 canon_guard.py
```

失敗時は非ゼロ終了し、`blocked`を返す。GitHubの正本は変更しない。

## できること

- 指定した4件のAKARI上位聖典をGitHubから取得する
- `docs/sacred/`内のAKARI関連ファイル一覧を照合し、未レビューの新規LOCKがあれば停止する
- 現行Brain技術構成とOpenClaw＋LangGraph第一候補の文面上の成立条件を確認する
- AKARIの最終ハードウェアとOpenClaw／LangGraphの最終製品採用が**未決定**であることを返す
- 取得不能・欠落・重要な記述変化があれば自動的な結論を出さない

## まだできないこと

- 任意の新しい聖典・LOCKの意味的な優先判定。未知の新規LOCKでは**停止**して人の再確認を求める
- 聖典の全文意味解析、矛盾の自動解消、変更承認、差分監査、正本の更新
- Dropbox、LangGraph、OpenClaw、Ghost、Characterとの接続
- ジョブの中断・再開、外部送信制御、長期記憶の永続化
- 公開／非公開リポジトリ両方への接続保証（認証・実環境試験は別工程）

**この段階は「正本参照の小さなガード」であり、Brain Step 1の長期記憶・プロジェクト継続全体が完成したという意味ではない。**

## 次の検証

1. GitHubから実際に4文書を取得してCLIが`source_assertions_verified`を返すこと
2. 未レビューの新規AKARI LOCKの検出テスト
3. 検出時の人間によるレビューと現行ソース一覧更新の運用
4. 文書の版・出典・SHA・時刻・Git履歴確認を追加して、意味上の衝突を明示する

既存LOCKは変更していない。
