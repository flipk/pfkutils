#!/bin/bash

vars="DBUS_SESSION_BUS_ADDRESS XDG_CURRENT_DESKTOP XDG_GREETER_DATA_DIR XDG_RUNTIME_DIR XDG_SEAT XDG_SEAT_PATH XDG_SESSION_DESKTOP XDG_SESSION_ID XDG_SESSION_PATH XDG_SESSION_TYPE XDG_VTNR"

(
    echo '#!/bin/bash'
    echo ''

    for v in $vars ; do
	eval val=\${$v}
	echo export $v=\'$val\'
    done

    echo ''
    echo 'exec google-chrome'
) > ~/bin/start-chrome-in-vnc.sh
chmod 755 ~/bin/start-chrome-in-vnc.sh

exit 0
