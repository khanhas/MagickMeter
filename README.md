![Title](https://i.imgur.com/1i9SwIk.png) 

Basic usage:
```ini
[MagickMeter_1]
Measure = Plugin
Plugin = MagickMeter
Image = D:\homer.svg

[Image_1]
Meter = Image
MeasureName = MagickMeter_1
```

You can add more image into current canvas by appending Image2, Image3, Image*N* in plugin measure:

```ini
[MagickMeter_1]
Measure = Plugin
Plugin = MagickMeter
Image  = D:\homer.svg
Image2 = E:\kanna.png
Image3 = http://www.clker.com/cliparts/f/a/5/P/4/A/dark-green-marijuana-leaf-vector-format-md.png
```

True power of ImageMagick library is image manipulation. To use available effects and modifiers, you can add them right after its declaration:
```ini
Image = D:\homer.svg | Scale 200% | Implode -1
Image2 = Text EXAMPLE FOR#CRLF#GitHub | Size 120 | Shadow 80,10,20,0 ; FF5050
Image3 = E:\Weed420Meme.jpg | AdaptiveBlur 0,20
```

## Example skin:

![SexyPlayer](https://i.imgur.com/VggetzK.png)

![SlashCalendar](https://i.imgur.com/U71KcKm.png)
