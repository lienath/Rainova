$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$html = [System.IO.File]::ReadAllText((Join-Path $root 'web\index.html'))
$css  = [System.IO.File]::ReadAllText((Join-Path $root 'web\style.css'))
$js   = [System.IO.File]::ReadAllText((Join-Path $root 'web\app.js'))

$html = $html.Replace('<!--STYLE-->', "<style>`n$css`n</style>")
$html = $html.Replace('<!--SCRIPT-->', "<script>`n$js`n</script>")

if ($html.Contains(')rawliteral"')) { throw 'El contenido contiene el delimitador )rawliteral" ' }

$out = 'const char INDEX_HTML[] PROGMEM = R"rawliteral(' + "`n" + $html + "`n" + ')rawliteral";' + "`n"
$dest = Join-Path $root 'firmware\Rainova\web.h'
[System.IO.File]::WriteAllText($dest, $out, [System.Text.UTF8Encoding]::new($false))
'Generado ' + $dest + ' (' + $out.Length + ' bytes)'
