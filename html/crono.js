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
            lastID= parseInt(data.ID);
            // if any event, parse it
            if (data.Command) {
                setTimeout(function(){ parseCommand(data.Command);},0);
            }
        },
        error: function(){
            console.log('Error in readData');
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
    c_llamada.stop();
    c_reconocimiento.stop();
    var crono=$('#cronoauto');
    if(crono.Chrono('started')) crono.Chrono('stop',1+start_timestamp);
    crono.Chrono( 'reset');
    $('#Clock').val("00:00");
    if(clockDisplay && (!clockDisplay.closed)) {
        clockDisplay.document.getElementById("Clock").value="00:00";
    }
    // do not update FTR, will be done in main program
    // also turn number should not be affected by reset
    if (local) writeData("reset");
}

function start_run(val,local) {
    if (local) start_timestamp = Date.now()-alive_timestamp; // miliseconds since webapp started
    else start_timestamp = 1+val;

    var crono=$('#cronoauto');
    // call crono functions
    c_llamada.stop();
    c_reconocimiento.stop();
    if (crono.Chrono('started')) crono.Chrono('stop',1+start_timestamp);
    crono.Chrono('reset');
    crono.Chrono('start',start_timestamp);

    // if locally generated, send to main loop
    if (local) writeData("start 0");
}

function int_run(elapsed,local) {
    var crono=$('#cronoauto');
    var int_timestamp= Date.now()-alive_timestamp; // miliseconds since webapp started
    if (local) elapsed = int_timestamp-start_timestamp;
    else elapsed += 1; // add 1 to elapsed to bypass chono handling fof '0'
    // call crono funtions
    if ( crono.Chrono('started')) { // si crono no esta activo, ignorar
        crono.Chrono('pause',start_timestamp+elapsed);
        setTimeout(function(){crono.Chrono('resume');},5000);
    }
    if (local) writeData("int "+elapsed);
}

function stop_run(elapsed,local) {
    var crono=$('#cronoauto');
    var end_timestamp = Date.now() - alive_timestamp;
    if (local) elapsed = end_timestamp - start_timestamp;
    else elapsed +=1; // add 1 to elapsed to bypass chono handling fof '0'
    if (crono.Chrono('started')) crono.Chrono('stop',start_timestamp+elapsed);
    if(local) writeData("stop " + elapsed);
}

function reconocimiento(seconds,local) {
    $('#LastTime').html("Last time: "+toFixedT(last_time/1000,2));
    if (local) seconds = 60 * parseInt($('#WalkTime').val());
    if (c_reconocimiento.started()) c_reconocimiento.stop();
    if (c_llamada.started()) c_llamada.stop();
    c_reconocimiento.reset(seconds);
    if (seconds!==0)c_reconocimiento.start();
    if(local) writeData("walk "+seconds);
}

function llamada(seconds,local) {
    $('#LastTime').html("Last time: "+toFixedT(last_time/1000,2));
    if (local) seconds=15;
    if (c_reconocimiento.started()) c_reconocimiento.stop();
    if (c_llamada.started()) c_llamada.stop();
    c_llamada.reset(seconds);
    if (seconds!==0)c_llamada.start();
    if (local) writeData("down "+seconds);
}
