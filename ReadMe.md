# Development Environment

1. Install prerequisites
    ```
    sudo apt update 

    sudo apt install build-essential libboost-all-dev bison flex texinfo wget git
    ```
1. Install SDCC 4.1.0:
    ```
    wget -O /tmp/sdcc-src-4.1.0.tar.bz2 https://sourceforge.net/projects/sdcc/files/sdcc/4.1.0/sdcc-src-4.1.0.tar.bz2
    # alternatively, wget -O /tmp/sdcc-src-4.1.0.tar.bz2 https://part-time-nerds.org/files/IskraDeltaPartner/Tools/sdcc/sdcc-src-4.1.0.tar.bz2
    
    cd /tmp

    tar -xvjf sdcc-src-4.1.0.tar.bz2
    
    cd /tmp/sdcc

    ./configure --disable-pic14-port --disable-pic16-port

    make
    
    sudo make install

    cd /tmp; rm -rf sdcc*
    ```
1. Install CPM TOOLS:
    ```
    sudo apt install cpmtools
    ```
1. Install HXCFE:
    ```
    sudo git clone https://github.com/mgrcar/HxCFloppyEmulator.git /opt/HxCFloppyEmulator

    cd /opt/HxCFloppyEmulator/build; sudo make HxCFloppyEmulator_cmdline
    ```
    Create an execution script in `/bin` (`/bin/hxcfe`):
    ```
    #/bin/sh
    cd /opt/HxCFloppyEmulator/build
    exec ./hxcfe "$@"
    ```
    Finally:
    ```
    sudo chmod +x /bin/hxcfe
    ```