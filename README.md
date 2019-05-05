# AgilityContest_SerialChronometer

AgilityContest plugin for for chronometers with serial port interface
Allow connect to AgilityContest Event bus any chronometer that uses serial port and complaint with these protocol

Provides:
- Command line invocation
- Web interface to show status. Can be used as auxiliary chronometer display
- monitoring, test and logging

**Invocation:**

ac_chrono[.exe] <options><br/>

Where options are:<br/>
    -d comport || --device=com_port .     Communication port to attach to<br/>
    -p tcpport || --port=tcp_port .   Where to listen for web interface. Default 8080<br/>
    -s ip || --server=ip_address .  Location of AgilityContest server. Default "localhost"<br/>
    -r ring || --ring=ring_number . Tell server which ring to attach chrono. Default "1"<br/>
    -l level || --debuglog=level .  Set debug/logging level. Defaults to "error"<br/>
    -f file || --logfile=filename . Set log file. Defaults to "stderr"<br/>
    -b baud || --baud=baudrate .    Set baudrate for comm port. Defaults 9600<br/>
    -t || --test .  Test mode. Don't try to connect server, just check comm port<br/>
    -f || --find .  Show available ports<br/>

**API Command description**
- Is up to the user select serial port baudrate. 
- Upper/Lower case is ignored
- Extra whitespaces and non-ascii chars are also ignored
- Every command are text based, and ends with 0x0D 0x0A sequence ("windows/dos newline")
- Time stamps are given in milisegundos
- Important: time deltas are _trunk'd_ to cents of second; NOT round'd, as KCC directives say

**Messages from Chronometer to Computer:**

* START [timestamp] < newline > (required) 

    Chronometer starts. Timestamp mark is optional. when ommited zero (0) is assumed
* INT timestamp < newline >

    (optional) Intermediate course run timestamp. Time shown is timestamp - start, trunk'd to cents of seconds
* STOP timestamp < newline > (required)

    End of course run. Time shown is timestamp - start, trunk'd to cents of seconds
* FAIL < newline > (optional)

    Sensor failure. Sent every second whilst error remains
* OK < newline > (optional - required whenever fail is implemented )

    Chronometer is ready. Sensor error is over

**Messages from Computer to Chronometer:**

* MSG message [ seconds ] < newline > ( optional )
    Show message on chronometer display . "seconds" is optional and indicates seconds that message is to be shown

**Bi-Directional messages:**

Can be sent either for the chronometer or the Computer
Chronometer can ignore these commands, but honoring RESET is recommended

* DOWN [seconds] < newline >

    Start CountDown when competitor receives ack to run. defaults to 15 seconds
* FAULT + < newline >
* FAULT - < newline >
* FAULT number < newline >

    Increase / Decrease / Set fault counter
* REFUSAL + < newline >
* REFUSAL - < newline >
* REFUSAL numero < newline >

    Increase / Decrease / Set refusal counter. If up to the user set ELIM flag after 3 refusals
* ELIM < newline >
* ELIM + < newline >

    Set eliminated mark
* ELIM - < newline >

    Clear eliminated mark
* RESET < newline > 

    Clears chronometer status, setting zero fault/refusal/countdown counters. Stop and clears chronometer