# Image Parser Lab

Лабораторная работа №2. Приложение для извлечения метаданных из графических файлов.

## Что делает

Сканирует указанную папку (включая подпапки) и извлекает основную информацию
о каждом графическом файле:

- имя файла;
- формат;
- размер изображения (в пикселях);
- глубина цвета;
- тип сжатия;
- разрешение (DPI);
- статус (OK / Повреждён / Неизвестный формат / Ошибка чтения).

Поддерживаемые форматы: **BMP, PNG, JPEG, GIF, TIFF, PCX**.

## Архитектура

```
src/
├── main.cpp                 — точка входа (консольная версия)
├── main_gui.cpp             — точка входа (GUI-версия, Qt)
├── BmpParser.cpp/.h         — ручной парсер BMP
├── PngParser.cpp/.h         — ручной парсер PNG
├── JpegParser.cpp/.h        — ручной парсер JPEG
├── GifParser.cpp/.h         — ручной парсер GIF
├── TiffParser.cpp/.h        — ручной парсер TIFF
├── PcxParser.cpp/.h         — ручной парсер PCX
├── ThreadPool.cpp/.h        — пул потоков для параллельной обработки
└── MainWindow.cpp/.h        — главное окно Qt (GUI)
```

Каждый парсер реализует статический метод `parse(const std::string&)`,
возвращающий структуру `ImageInfo`.

## Способ извлечения данных

**Все метаданные читаются вручную, побайтово**, через `std::ifstream`
в бинарном режиме:

```cpp
std::ifstream file(path, std::ios::binary);
```

**Сторонние библиотеки для чтения метаданных не используются.**
Каждый парсер реализует свою логику обхода структуры файла:

- **BMP** — читаем `BITMAPFILEHEADER` и `BITMAPINFOHEADER` через `#pragma pack(1)`.
- **PNG** — ищем сигнатуру `89 50 4E 47 0D 0A 1A 0A`, потом обходим чанки.
  Из `IHDR` берём ширину, высоту, глубину цвета. Из `pHYs` — DPI.
  Проверяем наличие `IEND` (иначе файл повреждён).
- **JPEG** — обходим поток маркеров `FF xx`. Из `SOF0`/`SOF2` берём размеры
  и глубину. Из `APP0/JFIF` — DPI. После `SOS` ищем `FFD9` (EOI),
  корректно пропуская энтропийно сжатые данные и `FF00` escape-байты.
- **GIF** — читаем заголовок `GIF87a`/`GIF89a` + Logical Screen Descriptor.
  Пропускаем Global Color Table. Обходим блоки (extensions, image descriptors).
  Проверяем наличие Trailer `0x3B`.
- **TIFF** — читаем header, поддерживаем оба byte order (`II` и `MM`).
  Обходим IFD-записи (по 12 байт каждая). Извлекаем теги
  `ImageWidth`, `ImageLength`, `BitsPerSample`, `Compression`, `XResolution`.
  Учитываем, что SHORT-значения хранятся прямо в поле `value` (2 байта),
  а большие массивы — по offset.
- **PCX** — читаем фиксированный 128-байтовый заголовок.
  Ширина = `Xmax - Xmin + 1`, высота = `Ymax - Ymin + 1`,
  глубина = `BitsPerPixel × NPlanes`. Проверяем EOI-маркер `0x0C 0x00`.

## Многопоточность

Обработка файлов распараллелена через **собственный `ThreadPool`**:

- `std::thread`, `std::mutex`, `std::condition_variable`;
- число потоков = `std::thread::hardware_concurrency()`;
- результаты собираются в общий `std::vector<ImageInfo>` под мьютексом.

Это даёт **многократное ускорение** при обработке больших папок (100 000+ файлов).

## GUI

GUI реализован на **Qt 6** (единственная внешняя библиотека):

- **QMainWindow** — главное окно;
- **QFileDialog** — диалог выбора папки;
- **QTableWidget** — таблица с метаданными (7 колонок);
- **QProgressBar** — индикатор прогресса;
- **QThread + signals/slots** — фоновый поток для сканирования,
  UI не блокируется во время работы.

**Qt используется только для GUI.** Все парсеры — на стандартной
библиотеке C++ (`<fstream>`, `<filesystem>`, `<thread>`, `<mutex>`).

## Сборка

### macOS (Qt-версия)

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt
cmake --build build
```

### Windows (.exe, консольная версия)

Кросс-компиляция с macOS через `mingw-w64`:

```bash
x86_64-w64-mingw32-g++ -std=c++17 -O3 -static \
    -I include \
    src/main.cpp \
    src/BmpParser.cpp \
    src/PngParser.cpp \
    src/JpegParser.cpp \
    src/GifParser.cpp \
    src/TiffParser.cpp \
    src/PcxParser.cpp \
    src/ThreadPool.cpp \
    -o ImageParserLab.exe \
    -static-libgcc -static-libstdc++
```

Флаг `-static` вшивает все библиотеки в `.exe`, поэтому он работает
на любой Windows-машине без установки дополнительных DLL.

## Тестирование

Протестировано на:
- **24 BMP-файлах** из архива «Для проверки Lab#2» — все OK.
- **4 PNG-файлах** — все OK.
- **3 JPEG-файлах** — все OK.
- **3 GIF-файлах** — все OK.
- **3 TIFF-файлах** — все OK.
- **3 PCX-файлах** — все OK.

Всего: **40 файлов** — 40 OK, 0 ошибок.

## Итоговые характеристики

- Поддержка **6 форматов**.
- **Многопоточная** обработка (до N ядер).
- **Кроссплатформенная** сборка (macOS + Windows).
- **GUI** с таблицей и прогресс-баром.
- **Детекция** битых файлов и неверных форматов.