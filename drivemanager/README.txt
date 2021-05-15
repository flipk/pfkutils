
responsibilities:

- list all drives installed
  - view partitions
  - recognize SERIAL and MODEL


- should i update /etc/auto.master.d/auto.auto and
  reload autofs ?

- automount tickler
   cd /auto/MOUNTDIR
   while true ; do touch x ; sleep 30 ; rm x ; sleep 30 ; done



<with -d it only prints top-level devices, not partitions>

<with -J it prints in JSON format>

lsblk -b -o NAME,PATH,KNAME,PKNAME,FSTYPE,MOUNTPOINT,LABEL,UUID,HOTPLUG,MODEL,SERIAL,SIZE,TYPE,TRAN 



< all information in JSON format >

lsblk -abJO
