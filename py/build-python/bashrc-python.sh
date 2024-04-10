
python_install_path=/vice2/python

nopy() {
    local np
    np="$( echo $PATH | tr ':' '\n' | grep -v $python_install_path | tr '\n' ':' | sed -E -e 's/:+$//' )"
    export PATH="$np"
}

py39() {
    nopy
    export PATH=$python_install_path/3.9/bin:$PATH
}

py310() {
    nopy
    export PATH=$python_install_path/3.10/bin:$PATH
}

py311() {
    nopy
    export PATH=$python_install_path/3.11/bin:$PATH
}
