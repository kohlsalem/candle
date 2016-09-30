#Fire 3 LED RGB Candles with a ESP8266WiFi

Last Christmas i bought a triplet of 3 Led candle with a cheap remote. So, why not include them in my (Homematic) home automation?

Just switching them on or off did not do the job - on "power on" they came back with some strange color.

So, some more serious surgery was needed. I removed the controller and replaced it by a NodeMCU.

On Data sheet the device has plenty of PWM enabled.

So how to animate a candle? Search found me [this](https://github.com/timpear/NeoCandle) pretty nice example of a neo candle.

I wasn't aware of multi threading on ESP - so how to let 3 candles flicker independently?

In this sketch i tried to turn the algorithm of flickering inside out :-). It gets pretty unreadable, however, for my it produces a fairly nice candle simulation.

steering gets dome per WLAN.

* /?s=0 - off
* /?s=100 - on
* /fire?s=60 - fire animation for 10 seconds
* /flicker?s=10 - flicker 10 seconds
* /flutter?s=5 - flutter 5 seconds
* /signal - make some crazy color dance
* /colour?r=40&g=55b=100 - guess

if you want to copy this - have fun :-)


in my actual implementation i had a Problem: my candles hat common + and seperate -. In order to run them on GPIO's i needed 9 NPN Transistor as describen [here](http://c-kolb.bplaced.net/projekte/elektronik/grundlagen/transistor/transistor_npn_pnp.php)


On the image:
![Image of Candles]http://i.imgur.com/bX745KQ.jpg) you can see the candles on aplate with the extra electronics and the lua.


Michael
