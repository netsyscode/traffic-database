mkdir build
mkdir data
mkdir data/source
mkdir data/index
mkdir data/output

dependencies=("make" "clang" "libpcap-dev" "python3-pip" "nodejs" "npm")

for package in "${dependencies[@]}"; do
    if dpkg -s "$package" >/dev/null 2>&1; then
        echo "$package is already installed."
    else
        apt install "$package"
        if [ $? -eq 0 ]; then
            echo "Successfully installed $package."
        else
            echo "Failed to install $package."
            exit 1
        fi
    fi
done

pip3 install flask
pip3 install dpkt

make