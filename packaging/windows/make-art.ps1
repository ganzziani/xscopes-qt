# Generates the two bitmaps the NSIS installer shows: the welcome page (164x314) and the
# header strip at the top of the other pages (150x57). Both are plain 24-bit BMPs, which is
# all NSIS understands. Run it only when the artwork changes; the results are committed.
#
#   powershell -ExecutionPolicy Bypass -File packaging\windows\make-art.ps1
#
# The logo comes from Bitmaps\GT-logo.png (dark panel, metallic "GT").

Add-Type -AssemblyName System.Drawing

$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)   # repository root
$logoPath = Join-Path $root 'Bitmaps\GT-logo.png'
$outDir = $PSScriptRoot
$logo = [System.Drawing.Image]::FromFile($logoPath)

# Colours from win-css.qss, so the installer looks like the application.
$panel = [System.Drawing.Color]::FromArgb(235, 236, 239)   # #EBECEF
$edge = [System.Drawing.Color]::FromArgb(200, 201, 208)    # #C8C9D0
$dark = [System.Drawing.Color]::FromArgb(40, 43, 54)       # text
$navy = [System.Drawing.Color]::FromArgb(30, 26, 58)       # logo background

function New-Bitmap([int]$width, [int]$height, [System.Drawing.Color]$back) {
    $bmp = New-Object System.Drawing.Bitmap($width, $height)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.Clear($back)
    $g.SmoothingMode = 'HighQuality'
    $g.InterpolationMode = 'HighQualityBicubic'
    $g.PixelOffsetMode = 'HighQuality'
    $g.TextRenderingHint = 'ClearTypeGridFit'
    return @($bmp, $g)
}

function Save-Bmp($bmp, [string]$path) {
    # Save as 24bpp BMP. The PNG-derived bitmap would otherwise keep its alpha channel and
    # NSIS rejects anything but 24-bit BMPs.
    $flat = New-Object System.Drawing.Bitmap($bmp.Width, $bmp.Height, [System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
    $fg = [System.Drawing.Graphics]::FromImage($flat)
    $fg.DrawImage($bmp, 0, 0, $bmp.Width, $bmp.Height)
    $fg.Dispose()
    $flat.Save($path, [System.Drawing.Imaging.ImageFormat]::Bmp)
    $flat.Dispose()
    Write-Host ("wrote {0} ({1}x{2})" -f $path, $bmp.Width, $bmp.Height)
}

# --- welcome page: 164x314 -------------------------------------------------------------
$welcome = New-Bitmap 164 314 $navy
$bmp = $welcome[0]; $g = $welcome[1]

$scaleW = 140
$scaleH = [int]($logo.Height * $scaleW / $logo.Width)
$g.DrawImage($logo, [int]((164 - $scaleW) / 2), 24, $scaleW, $scaleH)

$titleFont = New-Object System.Drawing.Font('Segoe UI Semibold', 16, [System.Drawing.FontStyle]::Regular, [System.Drawing.GraphicsUnit]::Point)
$subFont = New-Object System.Drawing.Font('Segoe UI', 9, [System.Drawing.FontStyle]::Regular, [System.Drawing.GraphicsUnit]::Point)
$smallFont = New-Object System.Drawing.Font('Segoe UI', 8, [System.Drawing.FontStyle]::Regular, [System.Drawing.GraphicsUnit]::Point)
$brushWhite = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::White)
$brushDim = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(150, 155, 175))
$centre = New-Object System.Drawing.StringFormat
$centre.Alignment = 'Center'
$centre.LineAlignment = 'Near'

$g.DrawString('XScopes', $titleFont, $brushWhite, (New-Object System.Drawing.RectangleF(0, 150, 164, 30)), $centre)
$g.DrawString('Interface', $titleFont, $brushWhite, (New-Object System.Drawing.RectangleF(0, 180, 164, 30)), $centre)
$g.DrawString('Oscilloscopes, logic analyzers', $subFont, $brushDim, (New-Object System.Drawing.RectangleF(0, 224, 164, 20)), $centre)
$g.DrawString('and the Oscilloscope Watch', $subFont, $brushDim, (New-Object System.Drawing.RectangleF(0, 240, 164, 20)), $centre)
$g.DrawString('Gabotronics', $smallFont, $brushDim, (New-Object System.Drawing.RectangleF(0, 278, 164, 16)), $centre)
$g.Dispose()
Save-Bmp $bmp (Join-Path $outDir 'welcome.bmp')

# --- header strip: 150x57 --------------------------------------------------------------
$header = New-Bitmap 150 57 $panel
$bmp = $header[0]; $g = $header[1]

$g.FillRectangle((New-Object System.Drawing.SolidBrush($panel)), 0, 0, 150, 57)
$g.FillRectangle((New-Object System.Drawing.SolidBrush($edge)), 0, 55, 150, 2)
# Logo only: NSIS draws the page title and subtitle to the right of this strip.
$hScaleH = 40
$hScaleW = [int]($logo.Width * $hScaleH / $logo.Height)
$g.DrawImage($logo, 8, [int]((55 - $hScaleH) / 2), $hScaleW, $hScaleH)
$g.Dispose()
Save-Bmp $bmp (Join-Path $outDir 'header.bmp')

$logo.Dispose()
