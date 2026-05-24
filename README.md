# CommonsBase_FileMagic

`CommonsBase_FileMagic` packages the OpenBSD `file` utility sources from the
OpenBSD 7.8 `src.tar.gz` release as a dk package, with the embedded magic
database overlaid by selected upstream `file` `FILE5_47.zip` Magdir entries.

The main package targets are:

- `CommonsBase_FileMagic.File@7.8.50407`

The output for every supported ABI is:

- `bin/file.exe`

The magic database is embedded directly into `file.exe`. The checked-in
generated sources live at `assets/src/filemagic_embedded_magic.[ch]` and are
refreshed with `maintenance/generate_embedded_magic.ps1`, which starts from the
bundled OpenBSD magdir and overlays the FILE5_47 entries listed in
`assets/magdir/file547_platform_entries.txt`.
