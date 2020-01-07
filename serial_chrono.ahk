#NoEnv  ; Recommended for performance and compatibility with future AutoHotkey releases.
; #Warn  ; Enable warnings to assist with detecting common errors.
SendMode Input  ; Recommended for new scripts due to its superior speed and reliability.
SetWorkingDir %A_ScriptDir%  ; Ensures a consistent starting directory.

COMPort := Object()
COMPortName := Object()
Num:=0

Puertos := "---|"
Descripciones := "---"

;Crear cadena con todos los puertos serie disponibles
Loop, HKLM, HARDWARE\DEVICEMAP\SERIALCOMM\ 
{
    RegRead, OutputVar
    COMPort%Num%:=OutputVar
    COM_result:=COMPort%Num%
    Num+=1
    Loop, HKLM, SYSTEM\CurrentControlSet\Enum, 1, 1 
	{
        if (A_LoopRegName = "FriendlyName") 
		{
            RegRead, Outputvar
            IfInString,Outputvar,%COM_result% 
			{
				Puertos = %Puertos%|%COM_result%
				Descripciones = %Descripciones%|%Outputvar%
            }
        }
    }
}

PuertosArray = StrSplit(Puertos,"|")
DescArray := StrSplit(Descripciones,"|")

;Dialogo de usuario
Gui, Add, Text, w80 x20 y60 , Puerto:
Gui, Add, Text, w80 x20 y90 , Velocidad:
Gui, Add, Text, w80 x20 y120, Tipo:
Gui, Add, Text, w80 x20 y150, Ag. Contest:
Gui, Add, Text, w80 x20 y180, Ring:
Gui, Add, Text, w80 x20 y270, Consola:
Gui, Add, Text, w80 x20 y300, Debug:
Gui, Add, Text, w80 x20 y330, Verbose:

Gui, Add, Text,         w200 x110 y25 , Parametros de Usuario
Gui, Add, DropDownList, w100 x110 y55 vPuerto gPuerto , %Puertos% 
Gui, Add, DropDownList, w100 x110 y85 vVelocidad, 9600|19200|38400|57600||74880|115200
Gui, Add, DropDownList, w100 x110 y115 vTipo , Estándar||digican
Gui, Add, DropDownList, w100 x110 y145 vContest , No||Local|Encontrar
Gui, Add, DropDownList, w100 x110 y175 vRing , 1||2|3|4|5|6|7|8
Gui, Add, Text,         w200 x110 y235, Parametros de Depuración
Gui, Add, DropDownList, w100 x110 y265 vConsola , No||Si
Gui, Add, DropDownList, w100 x110 y295 vLevel , 0||1|2|3|4|5|6|7
Gui, Add, DropDownList, w100 x110 y325 vVerbo , No||Si

Gui, Add, Text, w200 x220 y60 vDescrip , ---
Gui, Add, Text, w200 x220 y90, en baudios.
Gui, Add, Text, w200 x220 y120, Biblioteca de comunicación
Gui, Add, Text, w200 x220 y150, Comunicación con Agility Contest
Gui, Add, Text, w200 x220 y180, Numero de ring
Gui, Add, Text, w200 x220 y270, Usar consola de depuracion
Gui, Add, Text, w200 x220 y300, Nivel de depuración
Gui, Add, Text, w200 x220 y330, Mostrar Log en consola


Gui, Add, Button, w100 x110 y390 default, OK  ; The label ButtonOK (if it exists) will be run when the button is pressed.
Gui, Add, Button, w100 x+12 y390, Cancel  ; The label ButtonCancel (if it exists) will be run when the button is pressed.
Gui, Show, center, Seleccione configuración.
return  ; End of auto-execute section. The script is idle until the user does something.

Puerto:
Gui, Submit, NoHide
nombrePuerto := Puerto
GuiControl, +AltSubmit, Puerto
GuiControl, +AltSubmit, Tipo
GuiControl, +AltSubmit, Contest
GuiControl, +AltSubmit, Consola
GuiControl, +AltSubmit, Verbo
Gui, Submit, NoHide
GuiControl, , Descrip, % DescArray[Puerto]
Return

GuiClose:
ButtonCancel:
ExitApp

ButtonOK:
Gui, Submit
Gui, Destroy

Tipos := StrSplit("generic|digican","|")
Servidor := StrSplit("none|localhost|find","|")
Consolas := StrSplit(" | -c","|")
Verbos := StrSplit(" -q| -v","|")

; SerialChronometer.exe -m generic -d COM3 -b 57600 -D 5 -c -v
Run % "SerialChronometer.exe -m " Tipos[Tipo] " -d " nombrePuerto " -b " Velocidad " -s " Servidor[Contest] " -r " Ring  " -D " Level Consolas[Consola] Verbos[Verbo]

ExitApp
