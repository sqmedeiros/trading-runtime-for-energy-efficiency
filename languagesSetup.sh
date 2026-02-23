sudo apt-get update
pip install lazyme
sudo apt-get install make
sudo apt-get install zip unzip
sudo mkdir -p /opt/src
sudo apt-get update
sudo apt install cmake
sudo apt install curl #added
sudo apt-get install libffi-dev #added (Python regeverse complement)


## Compilers ##
   #C - v12.2.0 [NOTE: this will take about 3h to install]
    sudo apt install build-essential htop gcc-multilib zlib1g-dev 
    sudo apt-get install libgmp-dev libmpfr-dev libmpc-dev 
    sudo apt-get install libapr1-dev
    sudo apt-get install libpcre2-dev
    #wget http://mirror.linux-ia64.org/gnu/gcc/releases/gcc-12.2.0/gcc-12.2.0.tar.gz
    wget https://gcc.gnu.org/pub/gcc/releases/gcc-12.2.0/gcc-12.2.0.tar.gz
    tar -xvf gcc-12.2.0.tar.gz 
    cd gcc-12.2.0/ 
    ./configure --enable-shared --enable-linker-build-id --libexecdir=$HOME/gcc/usr/lib --without-included-gettext --enable-threads=posix --libdir=$HOME/gcc/usr/lib --enable-nls --disable-bootstrap --enable-clocale=gnu --enable-libstdcxx-debug --enable-libstdcxx-time=yes --with-default-libstdcxx-abi=new --enable-gnu-unique-object --disable-vtable-verify --enable-plugin --enable-default-pie --with-system-zlib --enable-libphobos-checking=release --with-target-system-zlib=auto --enable-objc-gc=auto --enable-multiarch --disable-werror --enable-cet --with-arch-32=i686 --with-abi=m64 --with-multilib-list=m32,m64,mx32 --enable-multilib --with-tune=generic --enable-checking=release --build=x86_64-linux-gnu --host=x86_64-linux-gnu --target=x86_64-linux-gnu --with-build-config=bootstrap-lto-lean --enable-link-serialization=2 --with-gmp --with-mpfr --with-mpc 
    make install 
    cd ../
    rm -rf gcc-12.2.0 gcc-12.2.0.tar.gz
        # K-nucl
        cd ./Languages/C/k-nucleotide || { echo "Directory not found"; exit 1; }
    
        # Clone the klib repository if not already cloned
        if [ ! -d "klib" ]; then
            git clone https://github.com/attractivechaos/klib.git
            echo "klib repository cloned."
        else
            echo "klib repository already exists."
        fi
        cd ../../../ 


    #C++ - v12.2.0
    sudo apt-get install libboost-all-dev
    sudo apt-get install libtbb-dev
    

    #Python - v3.11.1
    (cd /usr/src && 
    sudo wget https://www.python.org/ftp/python/3.11.1/Python-3.11.1.tgz  &&
    tar -xzf Python-3.11.1.tgz &&
    cd Python-3.11.1 &&
    sudo ./configure --enable-optimizations &&
    sudo make altinstall &&
    curl -sS https://bootstrap.pypa.io/get-pip.py | python3.11 &&
    pip3.11 install gmpy2)

    
    #Java - v20.0.2
    wget https://download.oracle.com/java/20/archive/jdk-20.0.2_linux-x64_bin.deb
    sudo dpkg -i jdk-20.0.2_linux-x64_bin.deb
    sudo apt-get install -f
    sudo update-alternatives --install /usr/bin/java java /usr/lib/jvm/jdk-20/bin/java 1
    sudo update-alternatives --install /usr/bin/javac javac /usr/lib/jvm/jdk-20/bin/javac 1
    sudo update-alternatives --install /usr/bin/jar jar /usr/lib/jvm/jdk-20/bin/jar 1
    rm jdk-20.0.2_linux-x64_bin.deb
    #-----------------Download .jar from https://jar-download.com/artifacts/it.unimi.dsi/fastutil/8.3.1/source-code
    cd Languages/Java
    unzip jar_files.zip
    sudo mkdir -p /opt/src/java-libs
    sudo mv fastutil-8.3.1.jar /opt/src/java-libs/fastutil-8.3.1.jar
    cd ../../

