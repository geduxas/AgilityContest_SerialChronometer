#!/bin/bash
tmpconf=/tmp/serial_chrono_$$.tmp
inifile=serial_chrono.ini

die() {
  echo "$@"
  exit 1
}

image=`dirname $0`/agilitycontest_64x64.png

which yad >/dev/null 2>&1 || die "Yad tool is not installed. Abort"
[ ! -x ./SerialChronometer ] && die "Cannot find SerialChronometer executable. Abort"
[ -f ./registration.info ] || die "Cannot find license file. Abort"

# buscamos puertos serie disponibles
puertos=`./SerialChronometer -f -D 0 -q | grep dev | tr [:space:] [!]`
[ "Z$puertos" == "Z" ] && die "No serial port(s) available. Exit"

# leemos fichero de configuracion
console="si!^no"
ring="Ring 1!Ring 2!Ring 3!Ring 4"
loglevel="0: None!1: Panic!2: Alert!3: Error!4: Notice!5: Info!6: Debug!7: Trace!8: All"
verbose="^si!no"
use_server="^si!no"
ajax_server="127.0.0.1"
module="generic!^digican!canometroweb"
comm_ipaddr="192.168.2.1"
comm_port="$puertos"
baud_rate="9600!19200!38400!57600!115200"
if [ -f $inifile ]; then
  sed -e 's/[ \t]//g' serial_chrono.ini | \
  awk -v puertos="$puertos" \
      -v ipaddr="$comm_ipaddr" \
      -v ring="$ring" \
      -v loglevel="$loglevel" \
      -v baud_rate="$baud_rate" \
      -F'=' '
  BEGIN {
        printf("#!/bin/bash\n");
      }
  /^console/ {
        printf("console=\"%s\"\n",($2==1)?"^si!no":"si!^no");
      }
  /^ring/ {
        gsub("Ring "$2,"^Ring "$2,ring);
        printf("ring=\"%s\"\n",ring);
      }
  /^loglevel/ {
        gsub($2": ","^"$2": ",loglevel);
        printf("loglevel=\"%s\"\n",loglevel);
      }
  /^verbose/ {
        printf("verbose=\"%s\"\n",($2==1)?"^si!no":"si!^no");
      }
  /^ajax_server/ {
        printf("ajax_server=\"%s\"\n",$2);
        printf("use_server=\"%s\"\n",($2!="none")?"^si!no":"si!^no");
      }
  /^module/ {
        printf("module=\"%s\"\n",($2=="generic")?"^generic!digican!canometroweb":"generic!^digican!canometroweb");
      }
  /^comm_ipaddr/ {
        printf("comm_ipaddr=\"%s\"\n",ipaddr);
      }
  /^comm_port/ {
        gsub($2,"^"$2,puertos);
        printf("comm_port=\"%s\"\n",puertos);
      }
  /^baud_rate/ {
        gsub($2,"^"$2,baud_rate);
        printf("baud_rate=\"%s\"\n",baud_rate);
      }
  ' > $tmpconf  && source $tmpconf
   # rm -f $tmpconf
  else
    echo "Cannot locate configuration file. Using defaults"
fi

# componemos y mostramos la ventana de dialogo del lanzador
res=`yad \
  --title="AgilityContest SerialChronometer interface" \
	--geometry="350x550" \
	--image="$image" \
	--dialog-sep \
	--borders=10 \
	--form \
	--separator="," \
	--field=" :LBL" "space"\
	--field="Parametros de Usuario:LBL" "user"\
	--field="IPAddr (Canometro):" "$comm_ipaddr" \
	--field="Puertos::CB" "$comm_port" \
	--field="Velocidad::CB" "$baud_rate" \
	--field="Modelo de cronometro::CB" "$module" \
	--field="Conectar con Ag Contest::CB" "$use_server" \
	--field="Ag Contest Server IPAddr:" "$ajax_server" \
	--field="Ring::CB" "$ring" \
	--field=" :LBL" "space" \
	--field="Parametros de Configuracion:LBL" "config"\
	--field="Activar consola::CB" "$console" \
	--field="Nivel de depuracion::CB" "$loglevel" \
	--field="Depuracion en consola::CB" "$verbose" \
	--field=" :LBL" "space" \
	--field="Guardar configuracion:CHK" "true" \
	"" "" "" "" "" "" "" "" "" "" "" "" "" "" "" "" ""`
[ $? -ne 0 ] && die "Operacion cancelada por el usuario"

# leemos el resultado de la ventana de dialogo
IFS=, read spare1 spare2 ipaddr port baud mod server addr ring spare3 spare4 cons loglvl clog spare5 save <<< $res

#componemos la linea de comandos
IPADDR="$addr"
[ "Z$server" == "Zno" ] && IPADDR="none"
RING=$(echo "$ring" | sed -e 's/Ring //g')
CONS=""
I_CONS="0"
[ "Z$cons" == "Zsi" ] && CONS="-c"
[ "Z$cons" == "Zsi" ] && I_CONS="1"
LVL=$(echo "$loglvl" | sed -e 's/: .*//g')
CLOG="-q"
I_CLOG="0"
[ "Z$clog" == "Zsi" ] && CLOG="-v"
[ "Z$clog" == "Zsi" ] && I_CLOG="1"

CMD="./SerialChronometer -m $mod -d $port -i $ipaddr -b $baud -s $IPADDR -r $RING $CONS -D $LVL $CLOG"

# si nos lo piden, guardamos datos en fichero de configuracion
if [ "Z$save" = "ZTRUE" ]; then
  # FASE 1: comprobar si el fichero existe; si no crearlo
  if [ ! -f $inifile ]; then
    # file does not exist. create default one
    cat << __EOF > $inifile
[Global]
# app will listen for incomming UDP module messages at local_port+ring
local_port = 8880
ring = $RING
# set to 1 to enable user console
console = $I_CONS

[Debug]
#logfile = c:\\windows\\temp\\serial_chrono.log
logfile = /home/jantonio/tmp/serial_chrono.log
# 0:none 1:panic 2:alert 3:error 4:notice 5:info 6:debug 7:trace 8:all
loglevel = $LVL
# set to 1 to copy log messages to console
verbose = $I_CLOG

[Server]
# location of AgilityContest server
# IPv4 Address (prefered) or FQDN host name ( not recomended if no DNS available)
# "none": do not connect to server
# "find" or "0.0.0.0" try to locate server in local network
ajax_server = $IPADDR
client_name = serial_chrono

[Serial]
#serial module to be used. "generic","digican",canometroweb ...
module = $mod
comm_ipaddr = $com_ipaddr
comm_port = $port
baud_rate = $baud

[Web]
# if set to non-zero app will listen at web_port+ring for html console
web_port = 8080
__EOF
  fi
  # FASE 2: editar los parametros del fichero de configuracion dejandolo como debe
  sed -i $inifile \
    -e "s|^ring =.*|ring = $RING|g" \
    -e "s|^console =.*|console = $I_CONS|g" \
    -e "s|^loglevel =.*|loglevel = $LVL|g" \
    -e "s|^verbose =.*|verbose = $I_CLOG|g" \
    -e "s|^ajax_server =.*|ajax_server = $IPADDR|g" \
    -e "s|^module =.*|module = $mod|g" \
    -e "s|^comm_ipaddr =.*|comm_ipaddr = $port|g" \
    -e "s|^comm_port =.*|comm_port = $port|g" \
    -e "s|^baud_rate =.*|baud_rate = $baud|g"
fi

echo "Executing $CMD"
exec $CMD

