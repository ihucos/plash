
set -eu

infile="$1"; shift
outfile="$1"; shift
name="$1"; shift

tmp=$(mktemp -d)

mkdir -pm 755 "$tmp/root/opt/$name/bin" "$tmp/root/opt/$name/rootfs/dev"

cp /usr/local/bin/glaze "$tmp/root/opt/$name/glaze" # XXXXXXXX

tar -xpf  "$infile" -C "$tmp/root/opt/$name/rootfs" --exclude ./dev --exclude /dev
chmod 755 "$tmp/root/opt/$name/rootfs"
 
cd "$tmp/root/opt/$name/rootfs"
find \
./usr/local/bin \
./usr/bin \
./bin \
./usr/local/sbin \
./usr/sbin \
./sbin 2> /dev/null \
| xargs -L 1 basename | sort | uniq \
| xargs -I{} ln -s "../glaze" "$tmp/root/opt/$name/bin/{}"

cd "$tmp/root"
mkdir -p 755 ./usr/local/bin/
for var in "$@"
do
    ln -s "/opt/$name/glaze" "./usr/local/bin/$var"
done

cd "$tmp/root"
tar -czf "$outfile" .
