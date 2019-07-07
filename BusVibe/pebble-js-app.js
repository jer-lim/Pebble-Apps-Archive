var result;

Pebble.addEventListener("ready", function(e){
	console.log("[PHONE] App loaded and ready");
});

Pebble.addEventListener("appmessage",function(e){
	console.log("[JS] Received message from pebble: "+JSON.stringify(e.payload));
	
  /*Pebble.sendAppMessage(
    {
      "key3":25,
      "key4":"some string value"
    },
    function(e){
      console.log("[JS]Successsfully devliered message with transactionId="+e.data.transactionId);
    },
    function(e){
      console.log("[JS]FAILLED TO SEND THE transactionId="+e.data.transactionId+" Error message: "+e.error.message);
    }
  );*/
  
  //Pebble.showSimpleNotificationOnPebble("Notifi\ncation","blvlallalalal");
  
	if(e.payload.send_getBusStop!==undefined){
		if(navigator.geolocation){
			navigator.geolocation.getCurrentPosition(getLocation,locationError);
		}else{
			console.log("Failed to get location");
		}
	}
	
	if(e.payload.send_busNumber!==undefined){
		getBusTimings(e.payload.send_busNumber);
	}

	if(e.payload.send_cycleBusStop!==undefined){
		sendBusStop(e.payload.send_cycleBusStop);
	}

	if(e.payload.resend_lastMessage){
		resendLastMessage();
	}
});

function sendMessageToPebble(name,result){
	var obj={};
	obj[name]=result;
	localStorage.setItem("lastMessage",JSON.stringify(obj));
	Pebble.sendAppMessage(
	obj,
	function(e){
		console.log("[JS]Successsfully devliered message with transactionId="+e.data.transactionId);
	},
	function(e){
		console.log("[JS]FAILLED TO SEND THE transactionId="+e.data.transactionId+" Error message: "+e.error.message);
	});
}

function resendLastMessage(){
	var obj=JSON.parse(localStorage.getItem("lastMessage"));
	Pebble.sendAppMessage(
	obj,
	function(e){
		console.log("[JS]Successsfully devliered message with transactionId="+e.data.transactionId);
	},
	function(e){
		console.log("[JS]FAILLED TO SEND THE transactionId="+e.data.transactionId+" Error message: "+e.error.message);
	});
}

function getLocation(location){
	console.log("Got location: "+location.coords.latitude+","+location.coords.longitude);
	getNearestBusStop(location.coords.latitude,location.coords.longitude);
	//sendMessageToPebble(result);
  
}

function locationError(error){
	switch(error.code){
		case error.PERMISSION_DENIED:
			result="PERMISSION_DENIED";
			break;
		case error.POSITION_UNAVAILABLE:
			result="POSITION_UNAVAILABLE";
			break;
		case error.TIMEOUT:
			result="TIMEOUT";
			break;
		case error.UNKNOWN_ERROR:
			result="UNKNOWN_ERROR";
			break;
	}
	console.log(result);
}

function getNearestBusStop(lat,long){
	var req=new XMLHttpRequest();
	req.open('GET','http://transportapis.jerl.im/bus/stops/nearest/lat/' + lat + '/lng/' + long,true);
	
	req.onload=function(e){
		console.log("Received nearest bus stop list: readyState "+req.readyState+" status "+req.status);
		if(req.readyState==4 && req.status==200){
			//var response=JSON.parse(req.responseText);\
			var response=JSON.parse(req.responseText);
			localStorage.setItem("busStopNum",0);
			localStorage.setItem("busStops", JSON.stringify(response));
			result=response.stops[0].id+"|"+response.stops[0].name;
			sendMessageToPebble("receive_busStop",result);
		}
	};
	
	req.send();
}

function sendBusStop(num){
	var busStops=JSON.parse(localStorage.getItem("busStops"));
	localStorage.setItem("busStopNum",num);
	result=busStops.stops[num].id+"|"+busStops.stops[num].name;
	sendMessageToPebble("receive_busStop",result);
}

function getBusTimings(busNum){
	var req=new XMLHttpRequest();
	var busStops=JSON.parse(localStorage.getItem("busStops"));
	var busStopNum=localStorage.getItem("busStopNum");
	var busTiming;
	req.open('GET','http://transportapis.jerl.im/bus/stops/'+busStops.stops[busStopNum].id+'/timings',true);
	
	req.onload=function(e){
		if(req.readyState==4 && req.status==200){
			//var response=JSON.parse(req.responseText);
			var response=JSON.parse(req.responseText);
			if(!response.error){
				var d = new Date(response.buses[busNum][0].arrivalTime);
				busTiming = Math.floor((d.getTime() - Date.now())/1000/60);
				if(busTiming <= 0) busTiming = "0";
				else busTiming = "" + busTiming;
				//busTiming=response.buses[busNum][0].arrivalTime;
				console.log("Bus timing: "+busTiming);
			}else{
				busTiming="-1";
			}
			sendMessageToPebble("receive_busTime",busTiming);
		}
	};
	
	req.send();
}