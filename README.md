# PongoOS oscaroff module

DeviceTreeからoscar関連のデバイスを無効化するPongoOS module

## 実行

動作環境
- Docker (Ubuntu 20.04コンテナ)

Windows:
```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\build.ps1
```

実行に成功すると、次の場所に実行ファイルが生成されます。

```text
.\out\oscaroff
```

## GitHub Actions

`main` へのpush、Pull Request、またはActionsの手動実行でビルドされます。
成功したワークフローの `oscaroff` artifact から実行ファイルを取得できます。
