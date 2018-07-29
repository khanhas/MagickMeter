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
