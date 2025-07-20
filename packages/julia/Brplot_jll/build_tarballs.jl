# Note that this script can accept some limited command-line arguments, run
# `julia build_tarballs.jl --help` to see a usage message.
using BinaryBuilder, Pkg

name = "Brplot"
version = v"0.0.4"

# Collection of sources required to complete build
sources = [
    ArchiveSource("https://github.com/branc116/brplot/releases/download/v$(version)/brplot-$(version).tar.gz", "416c06c1480408e54fb96bf46fa5f055d615b327ac25885ffa30f2a5118aaed2")
]

# Bash recipe for building across all platforms
script = raw"""
cd $WORKSPACE/srcdir

echo "Target = '$target'"
AMALGAM="brplot-0.0.4/include/brplot.c" 
LIBBRPLOT="$libdir/libbrplot.$dlext"
FLAGS="-O3 -o $LIBBRPLOT -shared -fpie -fPIC -DBRPLOT_IMPLEMENTATION $AMALGAM"
MAC_FLAGS="-ObjC $FLAGS -lm -framework Foundation -framework CoreServices -framework CoreGraphics -framework AppKit -framework IOKit" 

mkdir $libdir

case $target in
  x86_64-w64-mingw32)
    ${CC} $FLAGS -lm -lgdi32
    ;;

  aarch64-apple-darwin20)
    ${CC} $MAC_FLAGS
    ;;

  x86_64-apple-darwin14)
    ${CC} $MAC_FLAGS
    ;;

  *)
    ${CC} $FLAGS -lm
    ;;
esac

mkdir $prefix/share
mkdir $prefix/share/licenses
mkdir $prefix/share/licenses/Brplot
cp brplot-0.0.4/share/licenses/brplot/LICENSE $prefix/share/licenses/Brplot/LICENSE
"""

# These are the platforms we will build for by default, unless further
# platforms are passed in on the command line
platforms = [
    Platform("armv7l", "linux"; call_abi = "eabihf", libc = "glibc"),
    Platform("aarch64", "freebsd"; ),
    Platform("powerpc64le", "linux"; libc = "glibc"),
    Platform("aarch64", "macos"; ),
    Platform("aarch64", "linux"; libc = "glibc"),
    Platform("armv6l", "linux"; call_abi = "eabihf", libc = "glibc"),
    Platform("x86_64", "macos"; ),
    Platform("x86_64", "freebsd"; ),
    Platform("x86_64", "linux"; libc = "glibc"),
    Platform("x86_64", "windows"; )
]


# The products that we will ensure are always built
products = [
    LibraryProduct("libbrplot", :libbrplot)
]

# Dependencies that must be installed before this package can be built
dependencies = Dependency[
]

# Build the tarballs, and possibly a `build.jl` as well.
build_tarballs(ARGS, name, version, sources, script, platforms, products, dependencies; julia_compat="1.6", preferred_gcc_version = v"14.2.0")
