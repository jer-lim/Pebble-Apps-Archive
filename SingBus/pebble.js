Pebble.addEventListener("appmessage", function(e){
	console.log("[PHONE] Message received from Pebble: " + JSON.stringify(e.payload));

	if(e.payload.pb_requestBusStops !== undefined){
		if(navigator.geolocation){
			navigator.geolocation.getCurrentPosition(getLocation,locationError);
		}else{
			console.log("[PHONE] Failed to get location");
		}
	}else if(e.payload.pb_requestBusTimings !== undefined){
		var busStopId = e.payload.pb_requestBusTimings;
		getBusStopTimings(busStopId, 'js_busTimings');
	}else if(e.payload.pb_requestBusTimingsUpdate !== undefined){
		var busStopId = e.payload.pb_requestBusTimingsUpdate;
		getBusStopTimings(busStopId, 'js_busTimingsUpdate');
	}else if(e.payload.pb_toggleFavourite !== undefined){
		var busStopStr = e.payload.pb_toggleFavourite;
		toggleFavourite(busStopStr);
	}else if(e.payload.pb_showFavourites !== undefined){
		showFavourites();
	}
});

Pebble.addEventListener("ready", function(e){
	console.log("[PHONE] App loaded and ready");
	appReady();
});

function appReady(){

	if(localStorage.getItem("favourites") === null){
		console.log("[PHONE] Favourites not found, creating favourites");
		localStorage.setItem("favourites", "{}");
	}

	var obj = {'js_appReady': '69'};

	console.log("[PHONE] Sending message - App Ready");
	Pebble.sendAppMessage(obj,
		function(e){console.log("[PHONE] Message sent to Pebble - App Ready");},
		function(e){console.log("[PHONE] Failed to send to Pebble - App Ready, retrying"); appReady();});
}

function getLocation(location){
	console.log("[PHONE] Found Location: "+location.coords.latitude+", "+location.coords.longitude);
	getNearestBusStops(location.coords.latitude, location.coords.longitude);
}

function getNearestBusStops(lat, lng){
	var req=new XMLHttpRequest();
	req.open('GET',"http://transportapis.jerl.im/bus/stops/nearest/lat/" + lat + "/lng/" + lng, true);

	req.onload=function(e){
		if(req.readyState==4 && req.status==200){

			var sendToWatch = "";
			var i = 0;

			var response=JSON.parse(req.responseText);
			response.stops.forEach(function(stop, index, thisArray){
				if(i < 6) sendToWatch += stop.id + "|" + stop.name + "|" + stop.id + " " + stop.dist + "m,";
				i++;
			});
			sendToWatch = sendToWatch.slice(0, -1);

			var obj = {'js_busStops': sendToWatch};

			//console.log("[PHONE] Sending message - " + sendToWatch);
			Pebble.sendAppMessage(obj,
				function(e){console.log("[PHONE] Message sent to Pebble - Bus Stops");},
				function(e){console.log("[PHONE] Failed to send to Pebble - Bus Stops");});


		}
	};
	
	req.send();
}

function getBusStopTimings(busStopId, returnKey){
	var req=new XMLHttpRequest();
	req.open('GET',"http://transportapis.jerl.im/bus/stops/" + busStopId + "/timings", true);

	req.onload=function(e){
		if(req.readyState==4 && req.status==200){

			var sendToWatch = "";

			var response=JSON.parse(req.responseText);
			for(var key in response.buses){
				var bus = response.buses[key];

				if(bus[0] !== null){
					var d = new Date(bus[0].arrivalTime);
					var minutesToArrival = Math.floor((d.getTime() - Date.now())/1000/60);
					if(minutesToArrival <= 0) minutesToArrival = "Arr";
					else if(minutesToArrival == 1) minutesToArrival = "1 min";
					else minutesToArrival += " mins";
					var busLoad = 0;
					if(bus[0].busLoad == "Seats Available") busLoad = 1;
					else if(bus[0].busLoad == "Standing Available") busLoad = 2;
					else busLoad = 3;
					sendToWatch += " " + key + "|" + minutesToArrival + " |" + busLoad + ",";
				}else{
					sendToWatch += " " + key + "|Not Op. |3,";
				}
			}
			sendToWatch = sendToWatch.slice(0, -1);

			var obj = {};
			obj[returnKey] = sendToWatch;

			//console.log("[PHONE] Sending message - " + sendToWatch);
			Pebble.sendAppMessage(obj,
				function(e){console.log("[PHONE] Message sent to Pebble - Bus Timings");},
				function(e){console.log("[PHONE] Failed to send to Pebble - Bus Timings");});


		}
	};
	
	req.send();
}

function toggleFavourite(busStopStr){
	var favourites = JSON.parse(localStorage.getItem("favourites"));

	var bs = busStopStr.split(",");

	var add = true;
	for(var id in favourites){
		if(id == bs[0]) add = false;
	}
	if(add){
		favourites[bs[0]] = bs[1];
		console.log("[PHONE] New favourite added");
	}else{
		delete favourites[bs[0]];
		console.log("[PHONE] Favourite removed");
	}

	localStorage.setItem("favourites", JSON.stringify(favourites));
	console.log(localStorage.getItem("favourites"));
}

function showFavourites(){
	var sendToWatch = "";

	var favourites = JSON.parse(localStorage.getItem("favourites"));
	for(var id in favourites){
		var name = favourites[id];

		sendToWatch += id + "|" + name + "|" + id + ",";
	}

	sendToWatch = sendToWatch.slice(0, -1);

	var obj = {'js_busStops': sendToWatch};
	Pebble.sendAppMessage(obj,
		function(e){console.log("[PHONE] Message sent to Pebble - Bus Stops");},
		function(e){console.log("[PHONE] Failed to send to Pebble - Bus Stops");});
}

function locationError(error){
	var result;
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