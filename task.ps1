$msbuild = "C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe"

function configure {
    ./gyp/gyp.bat --depth=. .\magickmeter.gyp
}

function listSource {
    return Get-ChildItem -File -Path ".\src" |
        Where-Object {$_.Extension -match ".(cpp|h|rc)" } |
        Foreach-Object { $_.FullName.Replace("\", "/") }
}

function build() {
    # Build x64
    &$msbuild /p:Platform=x64;

    #Build x86
    &$msbuild /p:Platform=Win32
}

function dist() {
    $version = "0.5.0"
    Get-ChildItem -Recurse -File -Path ".\build" |
        Where-Object {$_.Extension -notmatch ".dll" } |
        Foreach-Object { Remove-Item $_.FullName }

    Compress-Archive -Path ".\build\*" ".\build\magickmeter-$($version)-dll-x64-x86.zip"
}

function bumpVersion() {
    param (
        [Parameter(Mandatory = $true)][int16]$major,
        [Parameter(Mandatory = $true)][int16]$minor,
        [Parameter(Mandatory = $true)][int16]$patch
    )

    $ver = "$($major).$($minor).$($patch)"

    (Get-Content ".\src\version.h") -replace "PLUGIN_VERSION `"[\d\.]*`"", "PLUGIN_VERSION `"$($ver).0`"" |
        Set-Content ".\src\version.h"

    (Get-Content ".\task.ps1") -replace "version = `"[\d\.]*`"", "version = `"$($ver)`"" |
        Set-Content ".\task.ps1"

    $ver2 = "$($major),$($minor),$($patch)"
    (Get-Content ".\src\Magickmeter.rc") -replace "FILEVERSION [\d,]*", "FILEVERSION $($ver2),0" |
        Set-Content ".\src\Magickmeter.rc"
}
