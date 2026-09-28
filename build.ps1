$ErrorActionPreference = "Stop"

$OutDir = Join-Path $PWD "out"
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

# Remove a stale artifact so a previous successful build can never be
# mistaken for the result of a failed build.
$OutputFile = Join-Path $OutDir "oscaroff"
if (Test-Path $OutputFile) {
    Remove-Item -Force $OutputFile
}

Write-Host "== Building Docker image =="
docker build --platform linux/amd64 -t pongo-oscaroff-builder .
if ($LASTEXITCODE -ne 0) {
    throw "docker build failed with exit code $LASTEXITCODE"
}

Write-Host ""
Write-Host "== Building oscaroff module =="
docker run --rm `
  --platform linux/amd64 `
  -v "${OutDir}:/out" `
  pongo-oscaroff-builder

if ($LASTEXITCODE -ne 0) {
    throw "docker run/module build failed with exit code $LASTEXITCODE"
}

if (-not (Test-Path $OutputFile -PathType Leaf)) {
    throw "Build command returned success, but $OutputFile does not exist"
}

if ((Get-Item $OutputFile).Length -eq 0) {
    throw "Output file exists but is empty: $OutputFile"
}

Write-Host ""
Write-Host "SUCCESS"
Write-Host "Built: $OutputFile"
