#define V0
#define V1
; #define V2
; #define DEBUGGING
; uncomment the above to enable light serial output,
; and/or full debugging data



; constants and variable assignment
	; ADC values at 10 bits appear to be roughly V * 48
	 ; 12.4V = 595 -> 95
         ; 13.0V = 625 -> 125
         ; determined experimentally from bandit: 117 + 156
	SYMBOL def_low_water = 100        
	SYMBOL def_high_water = 170      
	EEPROM 0, (def_low_water, def_high_water)
	EEPROM 10, (0, 0)        ; clear the location for new values

	SYMBOL low_water = b12
	SYMBOL high_water = b13

	SYMBOL tmp_low= b14
	SYMBOL tmp_high = b15
	SYMBOL low_timer = 6        ; cycles
	SYMBOL high_timer = 12     ; cycles
	SYMBOL pause_interval = 250 ; ms
	SYMBOL flash_slow = 3
	SYMBOL flash_med = 2
	SYMBOL flash_fast = 1
	SYMBOL flash_rate = b4
	SYMBOL Vin = b0
	SYMBOL avg = b5
	SYMBOL low_ctr = b2
	SYMBOL high_ctr = b11

	SYMBOL last_state = b3
	SYMBOL button_var = b7
	SYMBOL button_delay = b8
	SYMBOL button_flashes = b16
	SYMBOL button_flash_rate = b17

	SYMBOL ADC = C.1
	SYMBOL LED = C.2

	SYMBOL PUSHBUTTON = C.3
	SYMBOL RELAY = C.4
	



; task 0: the flasher
start0:
	high LED
	let w12 = flash_rate * 125
	pause w12
	low LED
	if flash_rate = flash_slow then
		sleep 1
	else
		pause w12

	endif
	goto start0


; task 1: the button interface
start1:
	button_init: 
		pullup on

		let button_var = 0
		let button_delay = 0
		#ifdef V2
		sertxd("reset", cr, lf)
		#endif
		; resume the flasher & controller (if it was suspended)
		restart 0
		restart 3
		
	; config state 0 -- waiting for input
	cfg_0: 
		button PUSHBUTTON, 0, 255, 1, button_var, 1, cfg_start
		pause 10
		goto cfg_0

	cfg_reset:
		let button_flashes = 20
		let button_flash_rate = 25
		gosub flash
		#ifdef V2
		sertxd("reset requested", cr, lf)
		#endif

		write 10, def_low_water
		write 11, def_high_water
		pause 1000
		goto button_init	
		
	cfg_start:
		 ; stop the flasher
		suspend 0
		low LED
		
		; stop the main control task & fire up accesories
		suspend 3
		high RELAY

		let button_flashes = 1
		let button_flash_rate = 50
		gosub flash

		pause 1000

		gosub get_adc
		
		let tmp_low= Vin
		#ifdef V1
		sertxd("low water: ", #tmp_low, cr, lf)
		#endif

		let button_flashes = 1
		let button_flash_rate = 50
		gosub flash
			
		pause 1000
		; check for reset condition ?
		let button_var = 0
		button PUSHBUTTON, 0, 100, 100, button_var, 1, cfg_reset
	
		#ifdef V1
		sertxd("wait for press...", cr, lf)
		#endif
		let button_flashes = 1
		let button_flash_rate = 50
		gosub flash
		low RELAY
		
		; wait up to 60s for cfg_high press
		let button_delay = 0 ; actually a delay counter
		let button_var = 0
		cfg_low_loop:
			button PUSHBUTTON, 0, 255, 1, button_var, 1, cfg_high
			pause 300
			
			; if it's been lil (~ 60s) while since the first press, bail out
			inc button_delay
			if button_delay > 200 then
				goto button_init
			endif
			goto cfg_low_loop
			
		
	cfg_high:
		let button_flashes = 1
		let button_flash_rate = 50
		gosub flash

		low RELAY
		pause 1000
		gosub get_adc
		let tmp_high = Vin
		#ifdef V1
		sertxd("high water: ", #tmp_high, cr, lf)
		#endif
		if tmp_high > tmp_low then
			let button_flashes = 3
			let button_flash_rate = 50
			gosub flash
			write 10, tmp_low
			write 11, tmp_high

			let low_water = tmp_low
			let high_water = tmp_high
			#ifdef V1
			sertxd("saving", cr, lf)
			#endif
		else
			let button_flashes = 4
			let button_flash_rate = 250
			gosub flash		
		endif
		
		pause 5000
		goto button_init
		
	goto start1


	
; task 3: the main loop
start3:
; initialization
        gosub init
        let low_ctr = 0
	let high_ctr = 0
        let flash_rate = flash_fast
        
; states: init, testing, high/on, middle (low < Vin < high)
       
; default state 
init_state:
	low RELAY
	gosub get_adc
	#ifdef V1
		sertxd("Vin: ", #Vin, " avg: ", #avg, cr, lf)
	#endif
	#ifdef DEBUGGING
		debug
	#endif
	; FALTHROUGH to testing_state

; Vin < high
testing_state:
	gosub get_adc
	if Vin >= high_water then
		let flash_rate = flash_med
		let low_ctr = 0
		if high_ctr > high_timer then
			goto high_state
		endif
		inc high_ctr
	elseif Vin <= low_water then
		let flash_rate = flash_fast
		let high_ctr = 0
		if low_ctr > low_timer then
			low RELAY
		else
			inc low_ctr
		endif
	else
		let flash_rate = flash_med
	endif
	pause pause_interval
	goto testing_state
	
; Vin >= high
high_state:
	high RELAY
	let flash_rate = flash_slow
	gosub get_adc
	if Vin < high_water then
		goto testing_state
	endif
	pause pause_interval
	goto high_state
	
	end


init:
	

	read 10, low_water
	read 11, high_water
	if low_water = 0 or high_water = 0 then

		read 0, low_water
		read 1, high_water
	else 
		#ifdef V0
		sertxd("Read from flash: low =", #low_water, ", high=", #high_water, cr, lf)
		#endif
	endif
	return


get_adc:
	readadc10 ADC, w0
	#ifdef V2
	calibadc10 w11
	sertxd("ADC10: ", #w0, ", 1.024V = ", #w11, cr, lf)
	#endif
	if w0 > 750 then
		let Vin = 250
	elseif w0  < 500 then
        	let Vin = 0
	else
		let Vin = w0 - 500
	endif
	#ifdef V1
		sertxd("Vin: ", #Vin, cr, lf)
	#endif
	return

	
flash:
	for w12 = 1 to button_flashes
		high LED
		pause button_flash_rate
		pause button_flash_rate
		low LED
		if w12 < button_flashes then
			pause button_flash_rate
			pause button_flash_rate
		endif
	next w12
	return
