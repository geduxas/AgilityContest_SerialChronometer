#!/bin/bash

die() {
  echo "$@"
  exit 1
}

image=`dirname $0`/agilitycontest_64x64.png

which yad >/dev/null 2>&1 || die "Yad tool is not installed. Abort"
puertos=`./SerialChronometer -f | grep dev | tr [:space:] [!]`
[ "Z$puertos" == "Z" ] && die "No serial port(s) available. Exit"

# leemos fichero de configuracion
console=1
ring=1
loglevel=3
verbose=1
ajax_server="127.0.0.1"
module="digican"
comm_port="/dev/ttyUSB0"
baud_rate="115200"

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
	--field="Puertos::CB" "$puertos" \
	--field="Velocidad::CB" "9600!19200!38400!57000!^115200" \
	--field="Modelo de cronometro::CB" "generic!digican" \
	--field="Conectar con Ag Contest::CB" "si!no" \
	--field="Ag Contest Server IPAddr:" "127.0.0.1" \
	--field="Ring::CB" "Ring 1!Ring 2!Ring 3!Ring 4" \
	--field=" :LBL" "space" \
	--field="Parametros de Configuracion:LBL" "config"\
	--field="Acivar consola::CB" "si!no" \
	--field="Nivel de depuracion::CB" "0: none!1: panic!2: alert!^3: error!4: notice!5: info!6: debug!7: trace!8: all" \
	--field="Depuracion en consola::CB" "si!no" \
	--field=" :LBL" "space" \
	--field="Guardar configuracion:CHK" "true" \
	"" "" "" "" "" "" "" "" "" "" "" "" "" "" "" ""`
[ $? -ne 0 ] && die "Operacion cancelada por el usuario"

echo $res
