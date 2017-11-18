# MagickMeter  

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

# Image Types  
## **File**
  You can use:  
  - A local file path, an URL  
  Supported formats:  
  ```
  AAI, ART, ARW, AVI, AVS, BPG, BMP, BMP2, BMP3, BRF, CALS, CGM, CIN, CMYK, CMYKA, CR2, CRW, CUR, CUT, DCM, DCR, DCX, DDS, DIB, DJVU, DNG, DOT, DPX, EMF, EPDF, EPI, EPS, EPS2, EPS3, EPSF, EPSI, EPT, EXR, FAX, FIG, FITS, FPX, GIF, GPLT, GRAY, HDR, HPGL, HRZ, HTML, ICO, INFO, INLINE, ISOBRL, ISOBRL6, JBIG, JNG, JP2, JPT, J2C, J2K, JPEG, JXR, JSON, MAN, MAT, MIFF, MONO, MNG, M2V, MPEG, MPC, MPR, MRW, MSL, MTV, MVG, NEF, ORF, OTB, P7, PALM, PAM, CLIPBOARD, PBM, PCD, PCDS, PCL, PCX, PDB, PDF, PEF, PES, PFA, PFB, PFM, PGM, PICON, PICT, PIX, PNG, PNG8, PNG00, PNG24, PNG32, PNG48, PNG64, PNM, PPM, PS, PS2, PS3, PSB, PSD, PTIF, PWP, RAD, RAF, RGB, RGBA, RGF, RLA, RLE, SCT, SFW, SGI, SHTML, SID, MrSID, SPARSE, SUN, SVG, TEXT, TGA, TIFF, TIM, TTF, TXT, UBRL, UBRL6, UIL, UYVY, VICAR, VIFF, WBMP, WDP, WEBP, WMF, WPG, X, XBM, XCF, XPM, XWD, X3F, YCbCr, YCbCrA, YUV
```
  - `Screenshot`, `Screenshot@1`, `Screenshot@2`, `Screenshot@N`  
    If you have multiple monitors, add `@1`, `@2`, `@N` to specify which one you want to capture screenshot.
    
  - `Image`, `Image2`, `ImageN`  
    Clone a defined image. *N* has to be smaller than current Image index.
    
```ini
Image = C:\Pictures\turtle_think.png`
Image2 = http://www.tuxpaint.org/stamps/stamps/animals/birds/duck.png`
Image3 = Screenshot
Image4 = Image2
```

### Modifers:
  #### `Canvas` *Default:* 600,600
  Define intial image size. You have to define this at beginning as first modifier or it will be ignored.  
  If your image file is a SVG, remember to set this setting to initially render vector at desired size, instead of using `Scale` or `Resize` effect.  
  
`Image = D:\image.png | Canvas 800,400`   
`Image = E:\picture.png | Canvas #CurrentConfigWidth#,(#CurrentConfigHeight# * 2)`
  
## **Text**
  Add a string to current canvas with various text modifers.  
  
`Image = Text I am A Red String | Color 255,0,0 | Face Times New Roman | Size 120`
  
### Modifers:  
  
#### `Text`
  String.  
  
  `Text Example String`  
    
#### `Canvas` *Default:* 600,600
  Define image region. Any pixel exceed this region is cutoff.  
  
`Canvas 800,400`  
`Canvas #CurrentConfigWidth#,(#CurrentConfigHeight# * 2)`  
    
#### `Anchor` *Default:* LeftTop
  Unlike Rainmeter String meter, define anchor of text to change both text *alignment* and *original position* to 
   - `LeftTop` (or `Left`),  
   - `LeftCenter`,  
   - `LeftBottom`,  
   - `RightTop` (or `Right`),  
   - `RightCenter`,  
   - `RightBottom`,  
   - `CenterTop` (or `Center`),  
   - `CenterCenter`,  
   - `CenterBottom`  
of **canvas**.
  
  Eg: 
```ini
Image = D:\kannaWow.png | Canvas 400,400
Image2 = Text One | Canvas 400,400 | Size 20 | Anchor Left
Image3 = Text Two | Canvas 400,400 | Size 20 | Anchor LeftCenter
Image4 = Text Three | Canvas 400,400 | Size 20 | Anchor LeftBottom
Image5 = Text Four | Canvas 400,400 | Size 20 | Anchor Center
Image6 = Text Five | Canvas 400,400 | Size 20 | Anchor CenterCenter
Image7 = Text Six | Canvas 400,400 | Size 20 | Anchor CenterBottom
Image8 = Text Seven | Canvas 400,400 | Size 20 | Anchor RightTop
Image9 = Text Eight | Canvas 400,400 | Size 20 | Anchor RightCenter
Image10 = Text Nine | Canvas 400,400 | Size 20 | Anchor RightBottom
```

![TextAnchorExample](https://i.imgur.com/L3eSlm9.png)

#### `Face` *Default:* Arial
  Use a font for your string. It can be:
  - Installed font name. Eg: `Face Comic Sans Ms`  
  - Font file in YourSkinFolder\\@Resources\Fonts folder. Eg: `Face @Avantgarde.ttf`  
  - Direct link to font file. Not recommend for publishing skin. Eg: `Face F:\FontCollection\Elegant Lux Mager.otf`  
    
#### `Size` *Default:* 30
  Text size.  
  
`Size 80`  
`Size (20*#Scale#)`  
  
#### `Color` *Default:* 0,0,0,255
  Text color.  
  
`Color 180,0,50`  
`Color 65,255,30,(255 * #Time# / 60)`  
`Color 5050ff`  

#### `Weight` *Default:* 400
  Choose font weight, only available for installed fonts. Valid values:
  - `100` - Thin (Hairline)
  - `200` - Extra Light (Ultra Light)
  - `300` - Light
  - `400` - Regular (Normal)
  - `500` - Medium
  - `600` - Semi Bold (Demi Bold)
  - `700` - Bold
  - `800` - Extra Bold (Ultra Bold)
  - `900` - Black (Heavy)
  - `950` - Extra Black (Ultra Black)  
  
  Eg: `Weight 600`

#### `Stretch` *Default:* 1
  Choose font stretch, only available for installed fonts. Valid values:
  - `1` - Normal Stretch
  - `2` - Ultra Condensed Stretch
  - `3` - Extra Condensed Stretch
  - `4` - Condensed Stretch
  - `5` - Semi Condensed Stretch
  - `6` - Semi Expanded Stretch
  - `7` - Expanded Stretch
  - `8` - Extra Expanded Stretch
  - `9` - Ultra Expanded Stretch  

  Eg: `Stretch 4`
  
#### `Style` *Default:* Normal
  Choose font style. Valid values:
  - `Normal`
  - `Italic`
  - `Olique`  
  
  Eg: `Style Italic`

#### `AntiAlias` *Default:* 1
  Set to `0` to turn off edge smoothing.
  

## **Ellipse** and **Rectangle**
  Just like Shape meter.  
  Parameters: (**bold** = required)  
  Ellipse **X**, **Y**, **RadiusX**, RadiusY  
  Rectangle **X**, **Y**, **Width**, **Height**, CornerRadiusX, CornerRadiusY  

  ```ini
  Image = Ellipse 300,300,50 | Color 50,50,50
  Image2 = Rectangle 10,10,600,200,20 | StrokeWidth 5 | StrokeColor FF5050
  ```
  
### Modifiers:
#### `Canvas` *Default:* 600,600
  Define image region. Any pixel exceed this region is cutoff.  
  
`Ellipse 50,50,20 | Canvas 80,80`  
`Rectangle 30,60,50,100 | Canvas [CalcSizeX],([CalcSizeY] / 3)`
  
#### `Color` *Default:* 0,0,0,1
  Fill shape color.  
  
  `Color 67492450`

#### `StrokeWidth` *Default:* 0
  Define stroke width.  
  
  `StrokeWidth 10`

#### `StrokeColor` *Default:* 0,0,0,0
  Define stroke color.  
  
  `StrokeColor 255,0,0`

## **Combine**
 Composite images with many blend method.  
 You can compose as many images as, as many times as you want in one sequence of Combine and still be able to add effects after that.  
 Parameters:  
 Combine *ParentImageName* | CombineMethod *ChildImageName* | CombineMethod *ChildImageName* | ...  
 
 **Note:**  
 Although it looks like Shape Combine, they does not work exactly the same. All modifiers and effects in *ParentImageName* and *ChildImageName* do not share and inherit, they are applied first and their final images are combined with chosen method.
 
 ```ini
Image = D:\homer.svg
Image2 = D:\maiChan.jpg
Image3 = Combine Image2 | HardLight Image
Image4 = Combine Image2 | Luminize Image | Move 600,0
Image5 = Combine Image2 | Bumpmap Image | Move 1200,0
```

![DemoComposite](https://i.imgur.com/ooUII7L.png)

### Combine methods:
#### `Minus`
#### `Bumpmap`
#### `Mask`
#### `Union`
#### `Xor`
#### `Blend`
#### `Hue`
#### `ColorBurn`
#### `ColorDodge`
#### `Darken`
#### `Divide`
#### `Divide2`
#### `Exclusion`
#### `Hardlight`
#### `Hardmix`
#### `Incomp`
#### `Intensity`
#### `Lighten`
#### `LinearBurn`
#### `LinearDodge`
#### `LinearLight`
#### `Luminize`
#### `Mathematics`

## Effects:
  Add special effects like blur, flip, implode,... to image.
  
#### `AdaptiveBlur` *Parameters:* Radius, Sigma
Adaptive-blur image with specified blur factor.  
`Radius` parameter specifies the radius of the Gaussian, in pixels, not counting the center pixel.  
`Sigma` parameter specifies the standard deviation of the Laplacian, in pixels.  

#### `Blur` *Parameters:* Radius, Sigma
Blur image with specified blur factor.  
`Radius` parameter specifies the radius of the Gaussian, in pixels, not counting the center pixel.  
`Sigma` parameter specifies the standard deviation of the Laplacian, in pixels.  

#### `GaussianBlur` *Parameters:* Radius, Sigma
Gaussian blur image.  
The number of neighbor pixels to be included in the convolution mask is specified by `Radius`.  
The standard deviation of the gaussian bell curve is specified by `Sigma`.  

#### `RotationalBlur` *Parameter:* Angle
Rotational blur image.
`Angle` is in degree.

#### `MotionBlur` *Parameters:* Radius, Sigma, Angle
Motion blur image with specified blur factor
`Radius` parameter specifies the radius of the Gaussian, in pixels, not counting the center pixel.  
`Sigma` parameter specifies the standard deviation of the Laplacian, in pixels.  
`Angle` (in degree) parameter specifies the angle the object appears to be comming from (zero degrees is from the right).  

#### `Noise` *Parameters:* Density, Type
Add noise to image with specified noise type
Valid `Type` values:
- `1` - Uniform Noise
- `2` - Gaussian Noise
- `3` - Multiplicative Gaussian Noise
- `4` - Impulse Noise
- `5` - Laplacian Noise
- `6` - Poisson Noise
- `7` - Random Noise

#### `Shadow` *Parameters:* PercentAlpha, Sigma, OffsetX, OffsetY
Simulate an image shadow.  
`PercentAlpha` specified opaque of shadow. Valid values: `0` (totally transparent) to `100` (totally visible).   
`OffsetX`, `OffsetY` specified distance of shadow from image.

#### `Resize` *Parameters:* Width, Height
Resize image to specified size.

#### `AdaptiveResize` *Parameters:* Width, Height
This is shortcut function for a fast interpolative resize using mesh interpolation.  It works well for small resizes of less than +/- 50% of the original image size.  For larger resizing on images a full filtered and slower resize function should be used instead.

#### `Scale` *Parameters:* Width, Height
#### `Scale` *Parameter:* Percent%
Resize image by using simple ratio algorithm.  
You can either specify in 2 parameters `Width` and `Height` or 1 parameter `Percent` with percent sign.  
`Scale 1920,300`  
`Scale 10% | Scale 1000%`  

#### `Crop` *Parameters:* X, Y, W, H, Origin
Crops out and uses a defined part of the image.  
`Origin` is optional and can be set to one of the following:
- `1` - Top left (default)  
- `2` - Top right  
- `3` - Bottom right  
- `4` - Bottom left  
- `5` - Center  

Eg:  
`Crop -50,-30,100,60,5`
 Start at the Origin of 5 or "Center". Then move -50 pixels left (negative number is left, positive number is right) and -30 pixels up (negative number is up, positive number is down). Then capture 100 pixels of width, and 60 pixels of height, and that is the new image. This will crop and use 100 X 60 pixels of the center of the image.


#### `Implode` *Parameter:* Factor 
A special effect usually be used in deep fried memes.

#### `Spread` *Parameters:* Amount
Spread pixels randomly within image by specified amount.

#### `Swirl` *Parameters:* Angle
Image pixels are rotated by degrees.

#### `Equalize` *No parameter*
Histogram equalization.

#### `Enhance` *No parameter* 
Minimize noise.

#### `Despeckle` *No parameter* 
Reduce speckle noise.

#### `Transpose` *No parameter* 
Creates a horizontal mirror image by reflecting the pixels around the central y-axis while rotating them by 90 degrees.

#### `Transverse` *No parameter* 
Creates a vertical mirror image by reflecting the pixels around the central x-axis while rotating them by 270 degrees.
#### `Flip` *No parameter* 
Reflect each scanline in the vertical direction.

#### `Flop` *No parameter* 
Reflect each scanline in the horizontal direction.
