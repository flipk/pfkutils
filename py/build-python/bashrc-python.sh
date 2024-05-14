
python_install_path=/vice2/python

_nopy() {
    local prev_FS="$IFS"
    IFS=":"
    local pathparts=($PATH)
    local np=()
    for p in "${pathparts[@]}" ; do
        if [[ "${p}" != "" ]] ; then
            if [[ "${p}" = "${p#$python_install_path/}" ]] ;then
               np+=("$p")
            fi
        fi
    done
    export PATH="${np[*]}"
    IFS="$prev_FS"
}

nopy() {
    _nopy
    which python3
}

py39() {
    _nopy
    export PATH=$python_install_path/3.9/bin:$PATH
    which python3
}

py311() {
    _nopy
    export PATH=$python_install_path/3.11/bin:$PATH
    which python3
}
