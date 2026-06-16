# Reproductor App

Reproductor de música en GTK3 + GStreamer con temas oscuros, letras LRC sincronizadas, portadas de álbum y soporte para múltiples formatos.

![Captura](/img/screenshot.png)

## Características

- **5 temas oscuros**: Azul, Púrpura, Verde, Rojo y Carbón
- **Múltiples formatos**: MP3, FLAC, WAV, OGG, M4A, Opus, WMA, AAC
- **Letras LRC**: Sincronizadas con la reproducción, línea actual resaltada
- **Portadas de álbum**: Extraídas automáticamente de los archivos
- **Playlist**: Selección de carpetas con escaneo recursivo
- **Persistencia**: Tema, volumen y carpetas guardados en `player-theme.txt`
- **Volumen exponencial**: Curva `pow(valor, 2.0)` para control preciso
- **Botón GitHub**: Abre el repositorio en el navegador

## Requisitos

- **GTK 3.24+**
- **GStreamer 1.0** con plugins: base, good, bad, ugly
- **GCC** (Linux) o **MSYS2 MINGW64** (Windows)

## Instalación (Linux)

```bash
git clone https://github.com/nestoree/reproductor_mp3.git
cd reproductor_mp3
chmod +x install.sh
./install.sh
```

Para instalar en un prefijo personalizado:

```bash
./install.sh --prefix=$HOME/.local
```

Para desinstalar:

```bash
./install.sh --uninstall
```

El instalador detecta automáticamente apt/dnf/pacman, instala dependencias y compila.

## Windows

Descarga la carpeta `windows/` en tu PC Windows.

### Instalación automática (recomendado)

Solo haz doble clic en **`setup.bat`** — automáticamente:

1. Instala MSYS2 (si no está presente)
2. Actualiza paquetes MSYS2
3. Instala GCC, GTK3, GStreamer, gst-libav y librsvg
4. Compila `player.c` → `reproductor.exe`
5. Crea `run.bat` para lanzamientos futuros
6. Crea un acceso directo en el escritorio con el icono `reproductor.ico`
7. Ejecuta el reproductor

No requiere instalación manual de nada. Si aparece UAC, haz clic en **Sí**.

### Lanzamiento manual

Una vez instalado, usa `run.bat` o ejecuta directamente `reproductor.exe`.

## Uso

```bash
reproductor
```

1. Haz clic en **📁 Abrir Carpeta** en la barra superior
2. Selecciona una o más carpetas con música
3. Haz clic en una canción de la playlist para reproducir
4. Usa 📜 **Letras** para ver la letra sincronizada (si hay un archivo `.lrc` junto al audio)
5. Cambia de tema con el combo-box en la barra superior

## Archivos del proyecto

| Archivo | Descripción |
|---|---|
| `player.c` | Código fuente Linux (GTK3 + GStreamer) |
| `install.sh` | Instalador Linux |
| `reproductor.desktop` | Entrada de escritorio |
| `reproductor.svg` | Icono de la app (SVG) |
| `reproductor.ico` | Icono de la app (Windows) |
| `windows/` | Código y scripts para Windows |
| `windows/player.c` | Fuente C compilado nativamente con MSYS2 MINGW64 |
| `windows/setup.ps1` | Instalador automático PowerShell |
| `windows/setup.bat` | Entrada de doble clic → `setup.ps1` |

## Notas

- Las letras LRC deben estar en el mismo directorio que el audio con el mismo nombre (ej. `cancion.mp3` → `cancion.lrc`)
- El archivo de configuración `player-theme.txt` se crea en el directorio de trabajo
- Los botones de control muestran símbolos Unicode ⏮ ▶ ⏸ ⏹ ⏭
