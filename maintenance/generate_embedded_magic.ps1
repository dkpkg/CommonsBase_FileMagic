<#
.SYNOPSIS
Generates the checked-in embedded magic database sources.

.DESCRIPTION
This repository embeds the file(1) magic database directly into file.exe
instead of shipping a sibling bin/magic file.

The generated outputs are:

- assets/src/filemagic_embedded_magic.h
- assets/src/filemagic_embedded_magic.c

They are checked into the repository on purpose. Keeping the generated C blob in
source control makes the package build deterministic and keeps the dk values
files simple: the values forms only need to fetch and compile checked-in assets.
They do not need to regenerate the blob during object builds.

The generator starts with `usr.bin/file/magdir` from the OpenBSD 7.8
`src.tar.gz` bundle, then overlays selected `magic/Magdir/*` entries from the
upstream file(1) `FILE5_47.zip` listed in a checked-in asset file. It
concatenates files in the same order used by the upstream build:

1. Header
2. Localstuff
3. OpenBSD
4. All magdir entries whose basename starts with [0-9a-z], sorted by name

Regenerate the checked-in outputs whenever the bundled OpenBSD source tarball,
the bundled FILE5_47.zip, or the checked-in overlay entry list changes, or
whenever you intentionally change the embedded-database format.

.PARAMETER Tarball
Path to the OpenBSD src.tar.gz archive. Relative paths are resolved from the
repository root. Defaults to target/filemagic-src-local/src.tar.gz, which is the
local bundle-validation output used during development.

.PARAMETER FileZip
Path to the upstream FILE5_47.zip archive. Relative paths are resolved from the
repository root. Defaults to target/filemagic-src-local/FILE5_47.zip, which is
the local bundle-validation output used during development.

.PARAMETER EntryList
Path to the checked-in text file that lists the FILE5_47 Magdir basenames to
overlay onto the OpenBSD magdir.

.PARAMETER OutputDir
Directory that receives the generated .c and .h files. Relative paths are
resolved from the repository root.

.EXAMPLE
powershell -ExecutionPolicy Bypass -File maintenance\generate_embedded_magic.ps1

Uses the locally validated bundle tarball and refreshes the checked-in sources.

.EXAMPLE
powershell -ExecutionPolicy Bypass -File maintenance\generate_embedded_magic.ps1 `
  -FileZip build\downloads\FILE5_47.zip

Regenerates the checked-in sources from an explicit FILE5_47.zip path.
#>
[CmdletBinding()]
param(
    [string]$Tarball = 'target\filemagic-src-local\src.tar.gz',
    [string]$FileZip = 'target\filemagic-src-local\FILE5_47.zip',
    [string]$EntryList = 'assets\magdir\file547_platform_entries.txt',
    [string]$OutputDir = 'assets\src'
)

$ErrorActionPreference = 'Stop'

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent $scriptDir

function Resolve-RepoPath {
    param([string]$Path)

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return $Path
    }

    return Join-Path $repoRoot $Path
}

$tarballPath = Resolve-RepoPath $Tarball
$fileZipPath = Resolve-RepoPath $FileZip
$entryListPath = Resolve-RepoPath $EntryList
$outputPath = Resolve-RepoPath $OutputDir
$tempRoot = Join-Path $env:TEMP 'CommonsBase_FileMagic-generate_embedded_magic'
$extractRoot = Join-Path $tempRoot 'extract'
$openBsdRoot = Join-Path $extractRoot 'usr.bin\file'
$openBsdMagdirPath = Join-Path $openBsdRoot 'magdir'
$magicRoot = Join-Path $extractRoot 'file-FILE5_47\magic'
$file547MagdirPath = Join-Path $magicRoot 'Magdir'
$headerPath = Join-Path $outputPath 'filemagic_embedded_magic.h'
$sourcePath = Join-Path $outputPath 'filemagic_embedded_magic.c'
$orderedNames = @('Header', 'Localstuff', 'OpenBSD')

if (-not (Test-Path $tarballPath)) {
    throw "Tarball not found: $tarballPath"
}

if (-not (Test-Path $fileZipPath)) {
    throw "FILE5_47.zip not found: $fileZipPath"
}

if (-not (Test-Path $entryListPath)) {
    throw "Entry list not found: $entryListPath"
}

if (-not (Get-Command tar -ErrorAction SilentlyContinue)) {
    throw 'The tar executable was not found on PATH.'
}

if (Test-Path $tempRoot) {
    Remove-Item -Recurse -Force $tempRoot
}

New-Item -ItemType Directory -Path $extractRoot -Force | Out-Null
New-Item -ItemType Directory -Path $outputPath -Force | Out-Null

& tar -xf $tarballPath -C $extractRoot 'usr.bin/file/magdir'
& tar -xf $fileZipPath -C $extractRoot 'file-FILE5_47/magic'

if (-not (Test-Path $openBsdMagdirPath)) {
    throw "The archive did not contain usr.bin/file/magdir: $tarballPath"
}

if (-not (Test-Path $magicRoot) -or -not (Test-Path $file547MagdirPath)) {
    throw "The archive did not contain file-FILE5_47/magic: $fileZipPath"
}

$openBsdSortedNames =
    Get-ChildItem $openBsdMagdirPath -File |
    # PowerShell -match is case-insensitive, but the upstream build only takes
    # lowercase/decimal magdir entries here.
    Where-Object { $_.Name -cmatch '^[0-9a-z]' } |
    Sort-Object Name |
    Select-Object -ExpandProperty Name

$overlayNames =
    Get-Content $entryListPath |
    ForEach-Object { $_.Split('#', 2)[0].Trim() } |
    Where-Object { $_ -ne '' } |
    Select-Object -Unique

foreach ($name in $overlayNames) {
    $overlayPath = Join-Path $file547MagdirPath $name
    if (-not (Test-Path $overlayPath)) {
        throw "The FILE5_47 archive did not contain overlay entry: $name"
    }
}

$allNames = $orderedNames + (
    @($openBsdSortedNames + $overlayNames) |
    Sort-Object -Unique
)
$builder = New-Object System.Text.StringBuilder

foreach ($name in $allNames) {
    $sourceFile =
        if ($name -in $orderedNames) {
            Join-Path $openBsdMagdirPath $name
        } elseif ($name -in $overlayNames) {
            Join-Path $file547MagdirPath $name
        } else {
            Join-Path $openBsdMagdirPath $name
        }

    if (-not (Test-Path $sourceFile)) {
        throw "Missing magdir source file: $name"
    }

    $null = $builder.Append(
        [System.IO.File]::ReadAllText(
            $sourceFile,
            [System.Text.Encoding]::UTF8
        )
    )
}

$bytes = [System.Text.Encoding]::UTF8.GetBytes($builder.ToString())
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)

$headerLines = @(
    '/* Generated by maintenance/generate_embedded_magic.ps1. */',
    '#ifndef FILEMAGIC_EMBEDDED_MAGIC_H',
    '#define FILEMAGIC_EMBEDDED_MAGIC_H',
    '',
    '#include <stddef.h>',
    '',
    'extern const unsigned char filemagic_embedded_magic[];',
    'extern const size_t filemagic_embedded_magic_size;',
    '',
    '#endif'
)

$sourceLines = New-Object System.Collections.Generic.List[string]
$sourceLines.Add('/* Generated by maintenance/generate_embedded_magic.ps1. */')
$sourceLines.Add('#include "filemagic_embedded_magic.h"')
$sourceLines.Add('')
$sourceLines.Add('const unsigned char filemagic_embedded_magic[] = {')

for ($index = 0; $index -lt $bytes.Length; $index += 12) {
    $chunk =
        $bytes[$index..([Math]::Min($index + 11, $bytes.Length - 1))] |
        ForEach-Object { '0x{0:x2}' -f $_ }
    $suffix = if ($index + 12 -lt $bytes.Length) { ',' } else { '' }
    $sourceLines.Add('    ' + ($chunk -join ', ') + $suffix)
}

$sourceLines.Add('};')
$sourceLines.Add("const size_t filemagic_embedded_magic_size = $($bytes.Length);")

[System.IO.File]::WriteAllText(
    $headerPath,
    (($headerLines -join "`n") + "`n"),
    $utf8NoBom
)
[System.IO.File]::WriteAllText(
    $sourcePath,
    (($sourceLines -join "`n") + "`n"),
    $utf8NoBom
)

Get-Item $headerPath, $sourcePath |
    Select-Object FullName, Length,
        @{Name = 'Sha256'; Expression = {
            (Get-FileHash $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        }}
