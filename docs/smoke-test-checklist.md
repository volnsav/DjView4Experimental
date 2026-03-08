# DjView Smoke Test Checklist

Use this checklist after dependency updates, renderer changes, or CI/build script changes.

## 1. Startup

- App starts without crash from clean build artifacts.
- App starts with existing user settings (no regressions in startup behavior).
- "About" dialog opens and shows expected app/library build info.
  - DjVuLibre version matches the bundled `libdjvulibre.dll` (currently `3.5.30`).
  - DjView version matches the release tag (currently `4.16`).

## 2. Document Open

- Open a single-page DjVu file.
- Open a multi-page DjVu file.
- Open a DjVu file with text layer (if available).
- Open via drag-and-drop (standalone mode).

## 3. Navigation And View

- Next/Prev/First/Last page actions work.
- Zoom in/out and fit-width/fit-page work.
- Rotate left/right works.
- Thumbnails/sidebar updates correctly when changing pages.
- Search panel works for at least one known term in text layer documents.

## 4. Rendering

- Raster mode works (no corruption/artifacts).
- OpenGL mode works (if enabled and available).
- Renderer switch behavior is stable across restart (per settings/env).
- Scroll and rapid zoom do not produce visual corruption.

## 5. Selection And Clipboard

- Rectangle selection works.
- Copy image from selected region works.
- Copy text from selected region works (for text-layer documents).

## 6. Export/Print

- Export to at least one image format (PNG/TIFF).
- Export to PDF/PS path works (if feature enabled in build).
- Print dialog opens and preview/print path does not crash.

### 6.1 PDF Export — Page Routing Logic

The PDF exporter (`QDjViewPdfTextExporter`) classifies each DjVu page
into one of three deferred types during `doPage()`, then writes them
all in `doFinal()` after building a shared JBIG2 symbol dictionary.

```
DjVu page
│
├─ COMPOUND + RENDER_COLOR
│   ├─ BG render OK + mask render OK
│   │   → PAGE_COMPOUND (deferred)
│   │     • BG layer:    JPEG  (DCTDecode, color or grayscale)
│   │     • Text mask:   JBIG2 (shared symbol dictionary across pages)
│   │     • Fill mask:   JBIG2 generic (separate, with /Decode [1 0])
│   │
│   └─ BG or mask render failed → fall-through to JPEG
│
├─ BITONAL (or forceJbig2 flag)
│   ├─ renderBlackToPix1bpp OK
│   │   → PAGE_BITONAL (deferred)
│   │     • Pure JBIG2 (shared symbol dictionary), no JPEG
│   │
│   └─ PIX render failed → fall-through to JPEG
│
└─ PHOTO / any fallback
    → PAGE_PHOTO_JPEG
      • Pure JPEG (DCTDecode, color or grayscale), no JBIG2
```

**Settings tab controls:**

| Control | Values | Default | Affects |
|---------|--------|---------|---------|
| Max resolution | 72, 100, 150, 200, 300, 600 dpi | 300 | Render resolution cap for all page types |
| JPEG quality | 1–100 | 75 | DCTDecode quality for BG and photo pages |
| BG downscale | 1–12× | 3× | BG layer resolution = render ÷ factor |
| Force grayscale | on/off | off | 1-channel JPEG for all images |
| JBIG2 threshold | 50–100% | 85% | Symbol similarity for glyph merging |
| Text layer | on/off | on | Invisible text overlay for search/copy |

## 7. Session/Preferences

- Preference dialog opens and saves changes.
- Key preferences persist after restart.
- Optional: tab/session restore works (if feature enabled).

## 8. Plugin/Embedded Mode (if used)

- Embedded/plugin launch path still opens document.
- Basic navigation and zoom in embedded mode work.

## 9. Regression Sanity

- No unexpected warnings/errors in runtime log for normal flow.
- App exits cleanly after opening/closing several files.

## 10. Windows-Specific: Runtime Dependencies

When testing a freshly extracted/installed Windows build:

- App starts without "DLL not found" errors.
- All required DLLs present next to `djview.exe` (see `README.md §1.4`):
  - `libdjvulibre.dll`, `libcrypto-3-x64.dll`, `libssl-3-x64.dll`
  - `Qt6Core.dll`, `Qt6Gui.dll`, `Qt6Widgets.dll`, `Qt6Network.dll`
  - `Qt6OpenGL.dll`, `Qt6OpenGLWidgets.dll`, `Qt6PrintSupport.dll`
  - `platforms\qwindows.dll`, `styles\qmodernwindowsstyle.dll`
- Translation files present in `share\djvu\djview4\` (`djview_uk.qm`, etc.).
- UI appears in correct language when system locale is non-English.

## Test Notes Template

- Build: `<branch/commit + config/platform>`
- OS/Arch: `<example: Windows x64 / Linux x64>`
- Renderer tested: `<Raster/OpenGL>`
- Files used: `<sample set>`
- Result: `<pass/fail>`
- Issues found: `<short list>`
