# Rainova
Proyecto completo Rainova. Regadera inteligente con agua de lluvia.

## Qué incluye este repo

- `firmware/Rainova/Rainova.ino` — firmware para **ESP8266 NodeMCU (ESP-12E)**.
- `firmware/Rainova/web.h` — dashboard web embebido (generado, no editar a mano).
- `web/index.html`, `web/style.css`, `web/app.js` — fuentes de la app.
- `tools/embed_web.ps1` — genera `firmware/web.h` a partir de `/web`.
- `README.md` — este archivo.

## Cómo se usa (feria / demo)

1. Alimentar el NodeMCU por USB (cable de datos) y subir `firmware/Rainova/Rainova.ino`.
   - Placa: **NodeMCU 1.0 (ESP-12E Module)**.
   - URL de placas: `http://arduino.esp8266.com/stable/package_esp8266com_index.json`.
   - Driver si no aparece puerto: **CP210x / Silicon Labs**.
   - Librerías: `ESP8266WiFi`, `ESP8266WebServer`, `Wire`, `LiquidCrystal_I2C`, `EEPROM` (todas vienen con el core excepto `LiquidCrystal_I2C`).
2. El ESP crea el WiFi **`Rainova-ESP`** (abierto) con IP **`192.168.4.1`**.
3. En el celular: conectarse a `Rainova-ESP` y abrir **`http://192.168.4.1`**.
   - La app abre con forma de teléfono en escritorio y a pantalla completa en móvil.
   - Al abrir, el celular le envía su hora al ESP para calcular “¿ya regó hoy?”.
4. La LCD 16x2 sigue mostrando humedad, estado y mL totales.

## Pines

| Pieza | NodeMCU |
|---|---|
| Sensor humedad AOUT | A0 |
| Flotador | D5 |
| Relé IN | D6 |
| LCD SDA | D2 |
| LCD SCL | D1 |

Evitar D3, D4 y D8 (afectan el arranque). El sensor va a **3,3 V**: recalibrar seco/mojado desde la pestaña Control de la app.

## API

| Método | Ruta | Uso |
|---|---|---|
| GET | `/` | Dashboard PWA |
| GET | `/api/estado` | Estado en vivo (JSON) |
| GET | `/api/historial` | Muestras + eventos (JSON) |
| GET | `/exportar.csv` | Descarga CSV |
| POST | `/api/hora` | `{"epoch":...}` |
| POST | `/api/accion` | `{"accion":"regar"|"pausa"|"reanudar"|"reset"}` |
| POST | `/api/calibracion` | `{"seco":782,"mojado":325,"ml":15.0,"pin":1234}` (opcional `nuevoPin`) |

## PIN de calibración

- La calibración del sensor está protegida con **PIN de 4 dígitos**. El PIN inicial es **`1234`** y se puede cambiar desde la pestaña **Control** (campo “Nuevo PIN”).
- Queda guardado en EEPROM, así que sobrevive a reinicios y reflashes del firmware.
- Después de **5 intentos fallidos** se bloquea el endpoint durante 30 segundos.
- Es una protección local de la demo (red `Rainova-ESP`), no criptografía.

## Registros y hora

- Al abrir la app, el celular envía su hora al ESP. Los eventos y muestras ocurridos antes de esa sincronización se **rellenan con su hora real** al recibirla, así “Eventos recientes” siempre muestra `HH:MM`.
- Todo riego manual queda registrado como **`Riego manual XX mL`** con su hora, suma `manualHoy` y cuenta en “¿Ya regó hoy?”.
- Si se solicita un riego manual mientras hay otro ciclo en curso, se ejecuta automáticamente al terminar el ciclo actual.

## Regenerar el dashboard embebido

Después de editar `web/*`:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\embed_web.ps1
```

Eso actualiza `firmware/Rainova/web.h`. Luego volver a subir el firmware.

## Notas de diseño

- La lógica de riego **no depende del WiFi**: si el celular se desconecta, el sistema sigue regando solo.
- El relé se apaga dejando el pin en `INPUT` (necesario para relés de 5 V con señal de 3,3 V).
- La bomba va con fuente propia y separada; no unir su negativo al NodeMCU.
- Cada fin de pulso se reinicia la LCD para recuperarla si el ruido la desordena.
- Referencia de ahorro: 350 mL (riego manual habitual). Se muestra frente al último riego del sistema.
- Paleta tomada de la imagen de la planta: crema, verde oliva y violeta floral.
- La interfaz usa **un único scroll**: cabecera y navegación quedan fijas dentro del marco del teléfono.

## Estado del repo

Los cambios están commiteados y pusheados a `main` en `https://github.com/lienath/Rainova.git`.
