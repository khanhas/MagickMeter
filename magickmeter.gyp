{
    "variables": {
        "im_86_path": "C:/Program Files (x86)/ImageMagick-7.0.8-Q16",
        "im_64_path": "C:/Program Files/ImageMagick-7.0.8-Q16",
        "rainmeter_plugin_folder": "%%appdata%%/Rainmeter/Plugins"
    },
    "targets": [
        {
            "target_name": "magickmeter",
            "type": "shared_library",
            "sources": [
                    "<!@(psm listsource)",
            ],
            "include_dirs": [
                "./src",
                "./API",
            ],
            "libraries": ["Shlwapi.lib"],
            "defines": ["UNICODE", ],
            "msvs_settings": {
                "VCCLCompilerTool": {
                    "RuntimeLibrary": 2,
                    "FavorSizeOrSpeed": 2,
                    "Optimization": 2,
                },
            },
            "configurations": {
                "Release": {
                    "msvs_configuration_platform": "Win32",
                    "include_dirs": ["<(im_86_path)/include", ],
                    "msvs_settings": {
                        "VCLinkerTool": {
                            "AdditionalDependencies": [
                                "<(im_86_path)/lib/CORE_RL_Magick++_.lib",
                                "<(im_86_path)/lib/CORE_RL_MagickCore_.lib",
                                "<(im_86_path)/lib/CORE_RL_MagickWand_.lib",
                                "./API/x32/Rainmeter.lib",
                            ],
                        },
                    },
                    "msvs_configuration_attributes": {
                        "OutputDirectory": "./build/x86",
                    },
                },
                'Release_x64': {
                    "msvs_configuration_platform": "x64",
                    "include_dirs": ["<(im_64_path)/include", ],
                    "msvs_settings": {
                        "VCLinkerTool": {
                            "AdditionalDependencies": [
                                "<(im_64_path)/lib/CORE_RL_Magick++_.lib",
                                "<(im_64_path)/lib/CORE_RL_MagickCore_.lib",
                                "<(im_64_path)/lib/CORE_RL_MagickWand_.lib",
                                "./API/x64/Rainmeter.lib",
                            ],
                        },
                    },
                    "msvs_configuration_attributes": {
                        "OutputDirectory": "./build/x64",
                    },
                },
            }
        },
    ],
}
