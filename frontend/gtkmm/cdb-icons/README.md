Civilised systems will use the SVG icons in this folder. On macOS, the
homebrew installation of librsvg seriously messes up the @rpath in the
libpixbufloader_svg.dylib, as a result of which it becomes impossible
to use the SVG loader on that platform without seriously messing around
with extra symlinks, preloading or rpath changes and subsequent binary
resigning. 

So on macOS, it's PNG again...

Re-generate these PNG versions of the icons with `make update_icons`
(requires inkscape).
