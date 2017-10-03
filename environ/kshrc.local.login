
#
# shortcuts
#

data=/usr/test/bssdata
kdat=/usr/test/bssdata/work/knaack
edat=/usr/test/bssdata/work/exec
data2=/usr/test/bssdata2
kdat2=/usr/test/bssdata2/work/knaack
edat2=/usr/test/bssdata2/work/exec
pci1=/usr/test/bsspci1/knaack
nib=/usr/test/bssdata/work/artesyn-spider/

lp=/usr/prod/bss1/load_pointer
int=/usr/test/bssint1

torn=/usr/test/bssdata/tornado
torn3=/usr/test/bssdata2/tornado3
init=/usr/test/bssdata/work/knaack/initial

alias s3='screen -c $HOME/.screenrc.3'
alias testdir=$HOME/bin/testdir
alias testdir.='eval `testdir .`'
alias qq='qcomm -q'
alias ps1='/bin/ps -u $USER -o pid,tty,args'
alias ps2='/bin/ps -u $USER -o pid,tty,pri,vsz,osz,rss,comm'
alias fi=find_include
alias whom='/usr/test/bsstools/bin/whom'
alias ccs='cd $HOME;exec bin/ccs'

#FUNCTIONS: lsv sv svlp lbsub lbsol lbsolccs lbapp lbappccs lbsun lbcomp lbgsd zeroversion makeorig ccdiff ccdiff2 dis tman pr cr riw vdp vmp vpp vsp vm vsl vl 

lsv() {
	typeset prefix
	if [[ $# -eq 0 ]] ; then
		prefix=knaack_
	else
		prefix=$1
	fi
	ct lsview ${prefix}* | cut -c3- | awk '{print $1}'
}

cdv() {
    cd /view/$1/usr/vob/gsm
}

sv() {
    ct setview $1
}

svlp() {
    load=`print $1 | /bin/sed -e 's/\(.\)\(.\)\(.\)\(.\)/\1.\2.\3.\4./'`
    ct setview rel_$load
    cd .
}

lbsub() {
    SHELL=/bin/ksh
    export SHELL

    __verbose COMMAND: RUN_CCS=$RUN_CCS lbsub $*

    if [[ $# -eq 1 ]] ; then
        __verbose INVOKING: bsub -q $1 -Is $SHELL
        bsub -q $1 -Is $SHELL
    else
        queue=$1
        shift
        if [[ $1 = "-q" ]] ; then
            shift
            __verbose INVOKING: bsub -q $queue -o $HOME/00_lbsub_%J $*
            bsub -q $queue -o $HOME/00_lbsub_%J $*
        else
            __verbose INVOKING: bsub -q $queue -Is $*
            bsub -q $queue -Is $*
        fi
    fi
    __verbose bsub complete
}

lbsol() {
    lbsub comp251 $*
}


lbsolccs() {
    export RUN_CCS=1
    lbsub comp251 $*
}

lbapp() {
    lbsub gsdapp28 $*
}

lbappccs() {
    export RUN_CCS=1
    lbsub gsdapp28 $*
}

lbsun() {
    lbsub sunos413 $*
}

lbcomp() {
    lbsub compute $*
}

lbgsd() {
    lbsub gsdbuild_26 $*
}

zeroversion() {
    ct ls $1 | 
     sed \
        -e 's,^\(.*\) *Rule.*$,\1,'     \
        -e 's,^\(.*\) *from.*$,\1,'     \
        -e 's,^\(.*/\).*$,\1,'          \
        -e 's,$,0,'
}

makeorig() {
    for f in $* ; do
        zv=`zeroversion $f`
        rm -f $f.orig
        echo $zv
        cp $zv $f.orig
    done
}

ccdiff() {
    typeset file_arg
    typeset opts
    typeset ofile

    if [[ x$1 == x+g ]] ; then
        graphical=no
        shift
    else
        graphical=yes
    fi

    file_arg=$1
    shift
    opts=$1

    ofile=`zeroversion $file_arg`

    if [[ $graphical == no ]] ; then
        sd $file_arg $ofile
    else
        xsd $file_arg $ofile
    fi
}

ccdiff2() {
    typeset file_arg
    typeset opts
    typeset ofile

    if [[ x$1 == x+g ]] ; then
        graphical=no
        shift
    else
        graphical=yes
    fi

    file_arg=$1
    shift
    opts=$1

    ofile=`zeroversion $file_arg`

    if [[ $graphical == no ]] ; then
        sd2 $file_arg $ofile
    else
        xsd2 $file_arg $ofile
    fi
}

dis() {
    if [[ $# -ne 2 ]] ; then
        print usage: dis elf_file dis_file
    else
        objdumpppc --disassemble $1 > $2
    fi
}


tman() {
    typeset base manpath
    base=/usr/test/bssdata2/tornado3/pcore750.7
    manpath=$base/host/man:$base/host/sun4-solaris2/man:$base/target/man
    man -M $manpath $*
}

pr() {
    for pr in $* ; do 
        cctview pr $pr 2>&1 | less
    done
}

cr() {
    for cr in $* ; do
        cctview cr $cr 2>&1 | less
    done
}

riw() {
    typeset ipaddr
    if [[ -f $HOME/.riw ]] ; then
        ipaddr=`cat $HOME/.riw`
    else
        ipaddr=136.182.177.88
    fi
    if [[ ${PWD#/usr/vob/gsm} != $PWD ]] ; then
        ITSFS_SERVER_IPADDR=$ipaddr itsfsriw -nodirlinks $* &
    else
        ITSFS_SERVER_IPADDR=$ipaddr itsfsriw $* &
    fi
}

debugbuild=-G

_vm() {
    type="$1"
    shift
    if [[ x$td == x ]] ; then
        bssmake -T $type $debugbuild $*
    else
        if [ ! -f $td/bin/bssmake ] ; then
            bssmake -T $type $debugbuild -r $*
        else
            $td/bin/bssmake -T $type $debugbuild -r $*
        fi
    fi
}

_vl() {
    unset TMP
    unset TEMP
    unset TMPDIR
    type="$1"
    shift
    target=
    if [[ -f TARGET_OPTIONS ]] ; then
        target=`awk -F= '/^OUTPUT/ { print $2 }' TARGET_OPTIONS`
    fi

    if [[ x$td == x ]] ; then
        bsslink -T $type $debugbuild $target $*
    else
        if [ ! -f $td/bin/bsslink ] ; then
            bsslink -T $type $debugbuild -j $target $*
        else
            $td/bin/bsslink -T $type $debugbuild -r $target $*
        fi
    fi
}

alias vdp='_vm VXWORKS:PPC750:DIAB:DPROC:'
alias vmp='_vm VXWORKS:PPC750:DIAB:MPROC:'
alias vpp='_vm VXWORKS:PPC750:DIAB:PCU:'
alias vsp='_vm VXSIM:SPARC:GNU:VXSIM:'
alias  vm='_vm ""'
alias vsl='_vl VXSIM:SPARC:GNU:VXSIM:'
alias  vl='_vl VXWORKS:PPC750:DIAB::'


if [[ x$RUN_CCS = x1 ]] ; then
  if __noninteractive ; then
    __verbose not honoring RUN_CCS due to noninteractive shell
  else
      unset RUN_CCS
      unset GOOD_SHELL
      __verbose RUN_CCS being honored
      cd $HOME
      exec bin/ccs
      # NOTREACHED
  fi
fi

# always make sure my /home/$USER/bin is the first
# element of the path.
if [[ x${PATH#/home/${USER}/bin} == x$PATH ]] ; then
    __verbose fixing up busted path
    PATHhead=${PATH%:/home/${USER}*}
    PATHtail=${PATH#*${USER}/bin:*}
    PATH=/home/$USER/bin:$PATHhead:$PATHtail
    unset PATHhead
    unset PATHtail
    export PATH
fi
