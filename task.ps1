$msbuild = "E:\Program Files (x86)\Microsoft Visual Studio\MSBuild\15.0\Bin\MSBuild.exe"
Write-Host "Current version: " -n -f ([ConsoleColor]::Green)
Write-Host (Get-Content ".\skinDefinition.json" | ConvertFrom-Json).version

function Build
{
    param(
        [string]$target = "both"
    )

    $config = "/p:Configuration=Release"
    $x64 = "/p:Platform=x64"
    $x86 = "/p:Platform=x86"

    switch($target)
    {
        "both" {
            &$msbuild $config $x64
            &$msbuild $config $x86
        }
        "64" { &$msbuild $config $x64 }
        "86" { &$msbuild $config $x86 }
    }
}

function Dist
{
    param (
        [Parameter(Mandatory = $true)][int16]$major,
        [Parameter(Mandatory = $true)][int16]$minor,
        [Parameter(Mandatory = $true)][int16]$patch
    )
    Remove-Item -Recurse .\obj, .\bin, .\dist -ErrorAction SilentlyContinue

    $ver = "$($major).$($minor).$($patch)"
    BumpVersion $ver
    Build

    New-Item -ItemType directory .\dist -ErrorAction SilentlyContinue

    Compress-Archive -Path .\bin\x64, .\bin\x86 -DestinationPath ".\dist\magickmeter_$($ver)_x64_x86_dll.zip"

    &".\SkinPackager.exe" .\skinDefinition.json
}

function BumpVersion
{
    param (
        [Parameter(Mandatory = $true)][string]$ver
    )

    (Get-Content ".\src\version.h") -replace "PLUGIN_VERSION `"[\d\.]*`"", "PLUGIN_VERSION `"$($ver).0`"" |
        Set-Content ".\src\version.h"

    (Get-Content ".\task.ps1") -replace "version = `"[\d\.]*`"", "version = `"$($ver)`"" |
        Set-Content ".\task.ps1"

    $verComma = $ver -replace "\.", ","
    (Get-Content ".\src\Magickmeter.rc") -replace "FILEVERSION [\d,]*", "FILEVERSION $verComma,0" |
        Set-Content ".\src\Magickmeter.rc"

    $skinDef = Get-Content ".\skinDefinition.json" | ConvertFrom-Json
    $skinDef.version = $ver
    $skinDef.output = "./dist/magickmeter_$($ver).rmskin"
    $skinDef | ConvertTo-Json | Set-Content ".\skinDefinition.json" -Encoding UTF8
}

function ChangeImageMagickVersion {
    param(
        [Parameter(Mandatory = $true)][string]$name
    )
    (Get-Content ".\ImageMagick.props") -replace "ImageMagick\-.*\-Q16", $name |
        Set-Content ".\ImageMagick.props"
}
