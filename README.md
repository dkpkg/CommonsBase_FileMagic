# CommonsBase_FileMagic

`CommonsBase_FileMagic` packages the OpenBSD `file` utility sources from the
OpenBSD 7.8 `src.tar.gz` release as a dk package.

The initial package line targets:

- `CommonsBase_FileMagic.File.Bundle@7.8.0`
- `CommonsBase_FileMagic.File@7.8.0`

The intended output contract for every supported ABI is:

- `bin/file.exe`

The implementation is modeled after `Y:\source\CommonsBase_GNU`, with bundle
acquisition split from platform-specific build forms and aggregate package
wiring.

The OpenBSD magic database is embedded directly into `file.exe`. The checked-in
generated sources live at `assets/src/filemagic_embedded_magic.[ch]` and are
refreshed with `maintenance/generate_embedded_magic.ps1`.
