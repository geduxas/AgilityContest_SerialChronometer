// MSG: timeout text of message
function showMessage(msg) {
    var a=msg.explode(" ",2);
    $('#MessageBox').html("<br/>"+a[1]).css("display","inline-block");
    setTimeout(function(){$('#MessageBox').css("display","none");},1000*parseInt(a[0]))
}

function parseCommand(command) {
    lastID +=1;
    cmd=command.toLowerCase();
    console.log("Command "+lastID+" received: '"+command+"'");
    // fault, refusal and eliminated commands are ignored here
    if (cmd.indexOf("reset")===0) {
        return c_reset(false);
    }
    if (cmd.indexOf("start ")===0) {
        return start_run(parseInt(command.substring(6)),false);
    }
    if (cmd.indexOf("int ")===0) {
        return int_run(parseInt(command.substring(4)),false);
    }
    if (cmd.indexOf("stop ")===0) {
        return stop_run(parseInt(command.substring(5)),false);
    }
    if (cmd.indexOf("msg ")===0) {
        showMessage(command.substring(4));
        return;
    }
    if (cmd.indexOf("walk ")===0) {
        return reconocimiento(parseInt(command.substring(5)),false);
    }
    if (cmd.indexOf("down ")===0) {
        return llamada(parseInt(command.substring(5)),false);
    }
    if (cmd.indexOf("turn ")===0) {
        $('#Turno').val(command.substring(5));
        return;
    }
    if (cmd.indexOf("fail")===0) {
        c_error(true);
        return;
    }
    if (cmd.indexOf("ok")===0) {
        c_error(false);
        return;
    }
    if (cmd.indexOf("bright ")===0) {
        return brillo(parseInt(command.substring(7)),false);
    }
    console.log("unknown command received: '"+command+"'");
}

function readData() {
    $.ajax({
        url: '/readData',
        dataType: 'json',
        method: 'GET',
        data: { 'ID': lastID },
        success: function(data){
            $('#Faltas').val(data.F);
            $('#Rehuses').val(data.R);
            $('#Eliminado').val(data.E);
            $('#Turno').val(data.D);
            if (data.ID==="0") $('#Ring').val(data.Ring);
            if(clockDisplay && (!clockDisplay.closed)) {
                clockDisplay.document.getElementById("Faltas").value=data.F;
                clockDisplay.document.getElementById("Rehuses").value=data.R;
                clockDisplay.document.getElementById("Eliminado").value=data.E;
                clockDisplay.document.getElementById("Turno").value=data.D;
                if (data.ID==="0") clockDisplay.document.getElementById("Ring").value=data.Ring;
            }
            lastID= parseInt(data.ID);
            // if any event, parse it
            if (data.Command) {
                setTimeout(function(){ parseCommand(data.Command);},0);
            }
            // and relaunch request
            setTimeout(readData,500);
        },
        error: function(){
            console.log('Error in readData');
            setTimeout(readData,2000);
        }
    });
    return false;
}

function writeData(msg) {
    $.ajax({
        type: 'GET',
        url: "/writeData",
        dataType: 'json',
        data: { Command : msg }
    }).done(function(data) {
        // alert(JSON.stringify(data));
    }).fail(function(data) {
        console.log("Error in writeData");
    });
}

/** enable/disable buttons according action */
function handle_buttons(button) {
    var resetBtn=$("#ResetBtn"); // always enabled :-)
    var startBtn=$("#StartBtn");
    var intBtn=$("#IntBtn");
    var stopBtn=$("#StopBtn");
    var downBtn=$("#SalidaBtn");
    var walkBtn=$("#ReconocimientoBtn");
    switch(button) {
        case "reset":
        case "stop":
            startBtn.attr("disabled",false);
            intBtn.attr("disabled",true);
            stopBtn.attr("disabled",true);
            downBtn.attr("disabled",false);
            walkBtn.attr("disabled",false);
            break;
        case "start":
        case "int":
            startBtn.attr("disabled",true);
            intBtn.attr("disabled",false);
            stopBtn.attr("disabled",false);
            downBtn.attr("disabled",true);
            walkBtn.attr("disabled",true);
            break;
        case "down":
            startBtn.attr("disabled",false);
            intBtn.attr("disabled",true);
            stopBtn.attr("disabled",true);
            downBtn.attr("disabled",false);
            walkBtn.attr("disabled",true);
            break;
        case "walk":
            startBtn.attr("disabled",true);
            intBtn.attr("disabled",true);
            stopBtn.attr("disabled",true);
            downBtn.attr("disabled",true);
            walkBtn.attr("disabled",false);
            break;
        default:
            console.log("handle_buttons() invalid button: "+button);
    }
}

function falta(inc) {
    var f= inc+parseInt( $('#Faltas').val() ) ;
    if (f<0) f=0;
    writeData("fault "+f);
}

function rehuse(inc) {
    var r= inc+parseInt( $('#Rehuses').val() ) ;
    if(r<0) r=0;
    writeData("refusal "+r);
}

function eliminado(inc) {
    var e= inc+parseInt( $('#Eliminado').val() ) ;
    if (e<0) e=0;
    if (e>1) e=1;
    writeData("elim "+e);
}

function c_error(state) {
    $('#MessageBox').html("<br/>Sensor(s) Error").css("display",(state)?"inline-block":"none");
}

function c_reset(local) {
    $('#LastTime').html("Last time: "+toFixedT(last_time/1000,2));
    $('#Faltas').val(0);
    $('#Rehuses').val(0);
    $('#Eliminado').val(0);
    start_local=false;
    if(clockDisplay && (!clockDisplay.closed)) {
        clockDisplay.document.getElementById("Faltas").value=0;
        clockDisplay.document.getElementById("Rehuses").value=0;
        clockDisplay.document.getElementById("Eliminado").value=0;
        // clock y turno no se actualizan con reset, sino con reload
    }
    c_llamada.stop();
    c_reconocimiento.stop();
    var crono=$('#cronoauto');
    if(crono.Chrono('started')) crono.Chrono('stop');
    crono.Chrono( 'reset');
    $('#Clock').val("00:00");
    if(clockDisplay && (!clockDisplay.closed)) {
        clockDisplay.document.getElementById("Clock").value="00:00";
    }
    // update buttons status
    handle_buttons("reset");
    // do not update FTR, will be done in main program
    // also turn number should not be affected by reset
    if (local) writeData("reset");
}

function start_run(val,local) { // provided val is zero  when local=true
    // evaluate timestamps
    local_ts = Date.now()-alive_timestamp; // miliseconds since webapp started
    remote_ts = (local)?local_ts : val; // 0:local; else provided
    var start_ts=remote_ts;

    // call crono functions
    var crono=$('#cronoauto');
    c_llamada.stop();
    c_reconocimiento.stop();
    if (crono.Chrono('started')) crono.Chrono('stop',start_ts);
    crono.Chrono('reset');
    crono.Chrono('start',start_ts+1);  // use value+1 to bypass zero chrono behaviour

    // update buttons status
    handle_buttons("start");
    // if locally generated, send to main loop
    if (local) writeData("start "+start_ts);
}

function int_run(val,local) { // provided val is zero  when local=true
    // evaluate timestamps
    var local_int = Date.now()-alive_timestamp; // miliseconds since webapp started
    var remote_int = val; // 0:local; else provided
    var int_ts=0;
    if (local) { // stop command is local
        if (local_ts===remote_ts) int_ts= local_int; // start:local stop:local
        else int_ts = remote_ts + local_int-local_ts; // start:remote stop:local
    } else { // stop command is remote
        // on start:local and stop remote, assume that remote uses our provided start
        // as there are no way to deduce their own start --> use remote_data
        // on start:remote and stop remote, --> use also remote data
        int_ts = remote_int; // start:remote stop:remote
    }

    // call crono funtions
    var crono=$('#cronoauto');
    if ( crono.Chrono('started')) { // si crono no esta activo, ignorar
        crono.Chrono('pause',int_ts+1); // use value+1 to bypass zero chrono behaviour
        setTimeout(function(){crono.Chrono('resume');},5000);
    }
    // update buttons status
    handle_buttons("int");
    // if locally generated, send to main loop
    if (local) writeData("int "+int_ts);
}

function stop_run(val,local) { // provided val is zero  when local=true
    // evaluate timestamps
    var local_stop = Date.now()-alive_timestamp; // miliseconds since webapp started
    var remote_stop = val; // 0:local; else provided
    var end_ts=0;
    if (local) { // stop command is local
        if (local_ts===remote_ts) end_ts= local_stop; // start:local stop:local
        else end_ts = remote_ts + local_stop-local_ts; // start:remote stop:local
    } else { // stop command is remote
        // on start:local and stop remote, assume that remote uses our provided start
        // as there are no way to deduce their own start --> use remote_data
        // on start:remote and stop remote, --> use also remote data
        end_ts = remote_stop; // start:remote stop:remote
    }
    // call crono funtions
    var crono=$('#cronoauto');
    if (crono.Chrono('started')) crono.Chrono('stop',end_ts+1); // use value+1 to bypass zero chrono behaviour
    // update buttons status
    handle_buttons("stop");
    // if locally generated, send to main loop
    if(local) writeData("stop " + end_ts);
}

function reconocimiento(seconds,local) {
    $('#LastTime').html("Last time: "+toFixedT(last_time/1000,2));
    if (local) seconds = 60 * parseInt($('#WalkTime').val());
    if (c_reconocimiento.started()) c_reconocimiento.stop();
    if (c_llamada.started()) c_llamada.stop();
    c_reconocimiento.reset(seconds);
    if (seconds!==0)c_reconocimiento.start();
    // update buttons status
    handle_buttons("walk");
    // if locally generated, send to main loop
    if(local) writeData("walk "+seconds);
}

function brillo(level,local) {
    if (local) level = parseInt($('#Bright').val());
    $('#Bright').val(level); // store new level
    $('#Clock').css("color",bright_levels[level]);
    if(clockDisplay && (!clockDisplay.closed)) {
        $(clockDisplay.document.getElementById("Clock")).css("color",bright_levels[level]);
    }
}

function llamada(seconds,local) {
    $('#LastTime').html("Last time: "+toFixedT(last_time/1000,2));
    if (local) seconds=15;
    if (c_reconocimiento.started()) c_reconocimiento.stop();
    if (c_llamada.started()) c_llamada.stop();
    c_llamada.reset(seconds);
    if (seconds!==0)c_llamada.start();
    // update buttons status
    handle_buttons("down");
    // if locally generated, send to main loop
    if (local) writeData("down "+seconds);
}
