Regardless of the existent of port or pin. W55FA92 GPIO driver reserves 0x20 
nodes for each port starting from PORT A

The mapping between GPIO pin and GPIO node is shown below. 


PORT NAME[PIN] = GPIO [NODE]	

PORTA[ 0]      = gpio[ 0x00]
PORTA[ 1]      = gpio[ 0x01]	  
               :
PORTA[31]      = gpio[ 0x1F]
PORTB[ 0]      = gpio[ 0x20]
               :
PORTB[31]      = gpio[ 0x3F]
               :
               :
               :
               :
PORTH[ 0]      = gpio[ 0xC0]
               :
               :
PORTH[31]      = gpio[ 0xDF]

The GPIO driver does _not_ touch register MFSEL, and assumes the pin user
application trys to control is configured as GPIO pin. In other words, the 
driver use the same pin must be unselect in kernel configuration.

Please make sure "menuconfig->device drivers->GPIO support->/sys/class/gpio"
and "menuconfig->device drivers->GPIO support->W55FA92 GPIO support" are enabled
in kernel configuration. 


