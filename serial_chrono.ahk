#NoEnv  ; Recommended for performance and compatibility with future AutoHotkey releases.
; #Warn  ; Enable warnings to assist with detecting common errors.
SendMode Input  ; Recommended for new scripts due to its superior speed and reliability.
SetWorkingDir %A_ScriptDir%  ; Ensures a consistent starting directory.

; Creamos array de claves a modificar en archivo ini

ClaveArray := [ring,console,loglevel,verbose,ajax_server,module,comm_port,baud_rate,fire_browser]
ValorArray := ["1","1","5","0","none","generic","COM3","57600","1"]

; Creamos array para leer linea a linea el archivo ini
Entrada := [] 

; vamos leyendo el archivo y copiandolo al array:
Loop, Read, %A_ScriptDir%\serial_chrono.ini      ;  %A_ScriptDir%\serial_chrono.ini ; leemos linea a linea.
{
    Entrada.Push(A_LoopReadLine) ; vamos copiando en elementos sucesivos en el array.
}

; recorremos el array para leer los valores de cada linea del archivo
for indice, linea in Entrada 
{
    ;eliminamos los espacios,si los hubiera
    StringReplace , linea, linea, %A_Space%,,All  
    
    ;descomponemos la linea en elementos
    elementosDeLinea := StrSplit(linea,"=")
    
    ;verificamos la presencia de los campos que nos interesan y asignamos valor al elemento del array correspondiente
    switch elementosDeLinea[1]
    {
        case "ring"        :  ValorArray[1] := elementosDeLinea[2]
        case "console"     :  ValorArray[2] := elementosDeLinea[2]
        case "loglevel"    :  ValorArray[3] := elementosDeLinea[2]
        case "verbose"     :  ValorArray[4] := elementosDeLinea[2]
        case "ajax_server" :  ValorArray[5] := elementosDeLinea[2]
        case "module"      :  ValorArray[6] := elementosDeLinea[2]        
        case "comm_port"   :  ValorArray[7] := elementosDeLinea[2]
        case "baud_rate"   :  ValorArray[8] := elementosDeLinea[2]
        case "fire_browser":  ValorArray[9] := elementosDeLinea[2]
    }
}

;iniciamos una cadena para ir guardando todos los puertos com disponibles
PuertoString := ""

;ciclamos buscando puertos com y los vamos añadiendo a la cadena
Loop, HKLM, HARDWARE\DEVICEMAP\SERIALCOMM\ 
{
    RegRead, COM_result
    Loop, HKLM, SYSTEM\CurrentControlSet\Enum, 1, 1 
	{
        if (A_LoopRegName = "FriendlyName") 
		{
            RegRead, Outputvar
            IfInString,Outputvar,%COM_result% 
			{
                if ( PuertoString = "" )
                {
                    PuertoString = %COM_result%
                } else {
                    PuertoString = %PuertoString%|%COM_result%
                }
            }
        }
    }
}

;Creamos el interfaz de usuario

 Gui, Add, Picture, x5 y5, agilitycontest_64x64.png

 Gui, Add, Text,     w180 x75 y30 , Parámetros de Usuario
 Gui, Add, Text,     w130 x75 y60 , Puertos:
 Gui, Add, Text,     w130 x75 y90 , Velocidad:
 Gui, Add, Text,     w130 x75 y120, Modelo de cronómetro:
 Gui, Add, Text,     w130 x75 y150, Conectar con Ag Contest:
 Gui, Add, Text,     w130 x75 y180, Ag Contest IP Addr:
 Gui, Add, Text,     w130 x75 y210, Ring:
;Gui, Add, Text,     w130 x75 y240
 Gui, Add, Text,     w180 x75 y270, Parámetros de Depuración
 Gui, Add, Checkbox, w130 x75 y300 vConsola, Activar consola
 Gui, Add, Text,     w130 x75 y330, Nivel de depuración:
 Gui, Add, Checkbox, w130 x75 y360 vVerbo, Depuración en consola
 Gui, Add, Checkbox, w130 x75 y390 vExplorador , Iniciar consola web
 Gui, Add, Checkbox, w130 x75 y420 vGuardar , Guardar configuración

;Gui, Add, Text,         w120 x200 y25                              
 Gui, Add, DropDownList, w120 x200 y55  vPuerto,    %PuertoString%                                         
 Gui, Add, DropDownList, w120 x200 y85  vVelocidad, 9600|19200|38400|57600|74880|115200
 Gui, Add, DropDownList, w120 x200 y115 vTipo,      generic|digican                                    
 Gui, Add, DropDownList, w120 x200 y145 vServidor gServidor,  No|Local|Manual|Automático                                      
 Gui, Add, Edit,         w120 x200 y175 vdirIP                                                  
 Gui, Add, DropDownList, w120 x200 y205 vRing ,     1|2|3|4|5|6|7|8                                                             
;Gui, Add, Text,         w120 x200 y235 
;Gui, Add, Text,         w120 x200 y265 
;Gui, Add, DropDownList, w120 x200 y295 , No|Si
 Gui, Add, DropDownList, w120 x200 y325 vLevel ,    0: none|1: panic|2: alert|3: error|4: notice|5: info|6: debug|7: trace|8: all
;Gui, Add, DropDownList, w120 x200 y355 , No|Si

 Gui, Add, Button, w80 x150 y450        , Cancel ; Se ejecutara la etiqueta ButtonCancel (si existe) al presionar este boton.
 Gui, Add, Button, w80 x+12 y450 default, OK     ; Se ejecutara la etiqueta ButtonOK (si existe) al presionar este boton.

;asignamos valores a las variables del interfaz de usuario según lo leido del archivo ini
Ring      := ValorArray[1]
Consola   := ValorArray[2]
Level     := ValorArray[3] + 1      ;sumamos uno a level ya que las opciones de seleccion van de 1 a 9 y no de 0 a 8
Verbo     := ValorArray[4]
dirIP     := ValorArray[5] 
Tipo      := ValorArray[6]         
Puerto    := ValorArray[7] 
Velocidad := ValorArray[8] 
Explorador:= ValorArray[9] 

;hacemos que se muestre en cada control el valor asignado
GuiControl, Choose, Ring, %Ring%
GuiControl, , Consola, %Consola%
GuiControl, Choose, Level, %Level%
GuiControl, , Verbo, %Verbo%
GuiControl, ChooseString, Tipo, %Tipo%
GuiControl, ChooseString, Puerto, %Puerto%
GuiControl, ChooseString, Velocidad, %Velocidad%
GuiControl, , Explorador, %Explorador%
GuiControl, , dirIP, %dirIP%
GuiControl, Disable, dirIP

;en funcion de la direccion IP leida, mostramos el valor correspondiente en la casilla de seleccion del servidor
;y activamos si es el caso la casilla para escribir la direccion IP a mano
switch dirIP
{
    case "none"      : GuiControl, Choose, Servidor, 1
    case "localhost" : GuiControl, Choose, Servidor, 2
    case "0.0.0.0"   : GuiControl, Choose, Servidor, 4
    Default          : GuiControl, Choose, Servidor, 3
                       GuiControl, Enable, dirIP
}

;mostramos el interfaz al usuario y esperamos a que haga algo (return)
Gui, Show, center, AgilityContest SerialChronometer Interface.
return  

;en caso de que se modifique la seleccion del tipo de servidor modificamos la casilla de la direccion IP y la habilitamos o no
Servidor:
    Gui, Submit, NoHide
    GuiControl, Disable, dirIP
    switch Servidor
    {
        case "No"        : GuiControl, , dirIP, none
        case "Local"     : GuiControl, , dirIP, localhost
        case "Automático": GuiControl, , dirIP, 0.0.0.0
        case "Manual"    : GuiControl, , dirIP, 
                           GuiControl, Enable, dirIP
    }
Return

;si el usuario cancela o cierra la ventana salimos de la aplicacion
GuiClose:
    ButtonCancel:
    ExitApp

;Si el usuario presiona OK, recogemos los valores seleccionados y cerramos la ventana
ButtonOK:
    GuiControl, +AltSubmit, Consola
    GuiControl, +AltSubmit, Level
    GuiControl, +AltSubmit, Verbo
    Gui, Submit
    Gui, Destroy


;como level va de 1 a 9, le restamos uno para que vaya de 0 a 8
Level := Level - 1

;si estaba activada la opcion guardar
if (Guardar) 
{
    ;eliminamos el archivo existente
    FileDelete, %A_ScriptDir%\serial_chrono.ini

    ;recorremos la variable entrada modificando solo las lineas con los campos que nos interesan
    for indice, linea in Entrada 
    {        
        ;descomponemos la linea en elementos
        elementosDeLinea := StrSplit(linea," = ")
        pruebas := elementosDeLinea[1]
        ;verificamos la presencia de los campos que nos interesan
        switch elementosDeLinea[1]
        {
            case "ring"         :FileAppend, ring = %Ring%`n, *%A_ScriptDir%\serial_chrono.ini
            case "console"     : FileAppend, console = %Consola%`n, *%A_ScriptDir%\serial_chrono.ini, 
            case "loglevel"    : FileAppend, loglevel = %Level%`n, *%A_ScriptDir%\serial_chrono.ini
            case "verbose"     : FileAppend, verbose = %Verbo%`n, *%A_ScriptDir%\serial_chrono.ini
            case "ajax_server" : FileAppend, ajax_server = %dirIP%`n, *%A_ScriptDir%\serial_chrono.ini
            case "module"      : FileAppend, module = %Tipo%`n, *%A_ScriptDir%\serial_chrono.ini
            case "comm_port"   : FileAppend, comm_port = %Puerto%`n, *%A_ScriptDir%\serial_chrono.ini
            case "baud_rate"   : FileAppend, baud_rate = %Velocidad%`n, *%A_ScriptDir%\serial_chrono.ini
            case "fire_browser": FileAppend, fire_browser = %Explorador%`n, *%A_ScriptDir%\serial_chrono.ini
            default            : FileAppend, %linea%`n, *%A_ScriptDir%\serial_chrono.ini
        }
    }
}

;Preparamos los parametros para la linea de ejecucion del programa

if (Consola = 1) 
{
    Consola := " -c"
}

if (Verbo = 1) 
{
    Verbo := " -v"
} else {
    Verbo := " -q"
}

if (dirIP = "0.0.0.0")
{
    dirIP := "find"
}
    
;Ejecutamos el programa con los parametros indicados
Run % "SerialChronometer.exe -m " Tipo " -b " Velocidad " -d " Puerto " -s " dirIP " -r " Ring " -D " Level Consola Verbo  

ExitApp