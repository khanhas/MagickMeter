![Title](https://i.imgur.com/1i9SwIk.png) 

[![GitHub release](https://img.shields.io/github/release/khanhas/MagickMeter.svg?colorB=97CA00?label=version)](https://github.com/khanhas/MagickMeter/releases/latest) [![Github All Releases](https://img.shields.io/github/downloads/khanhas/MagickMeter/total.svg?colorB=97CA00)](https://github.com/khanhas/MagickMeter/releases)  

## How to install:
#### 1. Download ImageMagick: https://www.imagemagick.org/download/binaries/ImageMagick-7.0.7-15-Q16-x64-dll.exe  
You can download newer version if there's one, but always download **Q16 x64** version.  
#### 2. Install ImageMagick. You must check `Add application directory to your system path`:

![InstallStep](https://i.imgur.com/6TbBlTo.png)
  
Remaining are optional, you can check them if you know what they do.  
#### 3. [Download](https://github.com/khanhas/MagickMeter/releases) and install Example skins pack to install plugin   
#### Or [download](https://github.com/khanhas/MagickMeter/releases) plugin version corresponding to your system and manually copy plugin DLL to `%appdata%\Rainmeter\Plugins\`

## Basic usage:
```ini
[MagickMeter_1]
Measure = Plugin
Plugin = MagickMeter
Image = File D:\homer.svg

[Image_1]
Meter = Image
MeasureName = MagickMeter_1
```

You can add more image into current canvas by appending Image2, Image3, Image*N* in plugin measure:

```ini
[MagickMeter_1]
Measure = Plugin
Plugin = MagickMeter
Image  = File D:\homer.svg
Image2 = File E:\kanna.png
Image3 = File http://www.clker.com/cliparts/f/a/5/P/4/A/dark-green-marijuana-leaf-vector-format-md.png
```

True power of ImageMagick library is image manipulation. To use available effects and modifiers, you can add them right after its declaration:
```ini
Image = File D:\homer.svg | Scale 200% | Implode -1
Image2 = File Text EXAMPLE FOR#CRLF#GitHub | Size 120 | Shadow 80,10,20,0 ; FF5050
Image3 = File E:\Weed420Meme.jpg | AdaptiveBlur 0,20
```

Check out [Wiki](https://github.com/khanhas/MagickMeter/wiki) for more options and modifiers.

## Example skins:

![SexyPlayer](https://i.imgur.com/VggetzK.png)

![SexyClock](https://i.imgur.com/bSWW9eO.png)

![SlashCalendar](https://i.imgur.com/LRpTWO3.png)

![Fracture](https://i.imgur.com/dnCDZvQ.png)

![UnderClock](https://i.imgur.com/aTlATjV.png)

![BitcoinGraph](https://i.imgur.com/r17dnOq.png)
