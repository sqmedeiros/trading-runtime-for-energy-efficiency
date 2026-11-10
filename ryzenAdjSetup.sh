# Install ryzen_smu driver

sudo apt install pkg-config lm-sensors scc cmake dkms git build-essential linux-headers-$(uname -r)
git clone https://github.com/amkillam/ryzen_smu # Active fork of the original module
(cd ryzen_smu/ && sudo make dkms-install)

cd ..


# Install RyzenAdj
sudo apt install cmake libpci-dev
git clone https://github.com/FlyGoat/RyzenAdj.git
cd RyzenAdj
rm -r win32
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
if [ -d ~/.local/bin ]; then ln -s $(readlink -f ryzenadj) ~/.local/bin/ryzenadj && echo "symlinked to ~/.local/bin/ryzenadj"; fi
if [ -d ~/.bin ]; then ln -s $(readlink -f ryzenadj) ~/.bin/ryzenadj && echo "symlinked to ~/.bin/ryzenadj"; fi

sudo cp lib/ryzenadj.h /usr/local/include/
sudo cp build/libryzenadj.so /usr/local/lib
sudo ldconfig

cd ..
