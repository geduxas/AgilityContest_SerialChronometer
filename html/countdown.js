
/*
 * A downcounter in seconds
 * from: http://stackoverflow.com/questions/1191865/code-for-a-simple-javascript-countdown-timer
 * usage:
 * var myCounter = new Countdown({  
 *   seconds:5,  // number of seconds to count down
 *   onUpdateStatus: function(sec){console.log(tenths of seconds);}, // callback for each second
 *   onCounterEnd: function(){ alert('counter ended!');} // final action
 * });
 * myCounter.reset(secs);
 * myCounter.start();
 */
function Countdown(options) {
	var paused=false;
	var timer=null;
	var instance = this;
	var seconds = options.seconds || 15;
	var count = 0;
	var updateStatus = options.onUpdateStatus || function () {};
	var counterEnd = options.onCounterEnd || function () {};
	var onstart = options.onStart || function () {};
	var onstop = options.onStop || function () {};

	function decrementCounter() {
		if (count <= 0) {
			counterEnd();
			instance.stop();
		} else {
			updateStatus(count);
			if (!paused) count=count - 0.5; // very dirty trick
		}
	}

	this.start = function () {
		onstart();
		if (timer!==null) clearInterval(timer);
		paused=false;
		count = seconds*10; // count tenths of seconds
		timer = setInterval(decrementCounter, 50);
	};

	this.stop = function () {
		onstop();
		if (timer!==null) clearInterval(timer);
		paused=false;
		count=0;
		updateStatus(count);
	};

	// get/set start count. DO NOT STOP
	this.reset = function (secs) {
		if (typeof(secs) === 'undefined') return seconds;
		var s=parseInt(secs);
		if (s>0) seconds=s;
		return seconds;
	};

	// get/set current count DO NOT STOP
	this.val = function(secs) {
		if (typeof(secs) !== 'undefined') count=secs*10;
		return count;
	};
	// very dirty pause and resume
	this.pause = function() { if (count>0) paused=true; };
	this.resume= function() { if (count>0) paused=false; };
	this.paused = function() { return paused; }
	// get running status
	this.started = function() {
		return (count>0); //true if started
	}
}

