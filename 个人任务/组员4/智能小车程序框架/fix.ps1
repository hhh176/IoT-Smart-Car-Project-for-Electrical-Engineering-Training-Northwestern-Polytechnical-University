$file = 'C:\Users\31605\Downloads\智能小车程序框架\智能小车程序框架\智能小车.c'
$enc = [System.Text.Encoding]::GetEncoding(936)
$c = $enc.GetString([System.IO.File]::ReadAllBytes($file))

# 1: Enable TR1/ET1
$c = $c.Replace('//TR1 = 1;', '  TR1 = 1;')
$c = $c.Replace('//ET1 = 1;', '  ET1 = 1;')

# 2: Fix TM1_Isr
$p = $c.IndexOf('void TM1_Isr() interrupt 3')
$n = $c.IndexOf('IC', $p)
$b = $c.Substring(0, $p)
$a = $c.Substring($n)
$isr = "void TM1_Isr() interrupt 3`r`n{`r`n    if(GM==0)`r`n    {`r`n        stop();`r`n    }`r`n    else`r`n    {`r`n        if(HWL==1 && HWZ==0 && HWR==1)       { qianjin(); }`r`n        else if(HWL==0 && HWZ==1 && HWR==1)  { zuozhuan(); }`r`n        else if(HWL==1 && HWZ==1 && HWR==0)  { youzhuan(); }`r`n        else if(HWL==0 && HWZ==0 && HWR==0)  { qianjin(); }`r`n    }`r`n}`r`n`r`n"
$c = $b + $isr + $a

# 3: Remove stop() from RFID
$c = $c.Replace('stop(); Speech(', 'Speech(')

# 4: Remove TL+line from main loop
$tl = $c.IndexOf('红绿灯检测')
$rf = $c.IndexOf('RFID站点报站')
if ($tl -gt 0 -and $rf -gt $tl) {
    $b4 = $c.Substring(0, $tl - 10)
    $a4 = $c.Substring($rf)
    $c = $b4 + $a4
}

# Fix comment formatting
$c = $c.Replace('///////////////RFID', '    //////////////////////////////////RFID')
$c = $c.Replace('    `r`n`r`n    //////////////////////////////////避障', "`r`n    //////////////////////////////////避障")

[System.IO.File]::WriteAllBytes($file, $enc.GetBytes($c))
Write-Output "Done"
