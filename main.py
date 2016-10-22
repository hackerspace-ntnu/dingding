import bluepy.bluepy.btle as btle
import pygame, os, random, traceback, time
from datetime import datetime
from slackman import Slackman

BUTTON_MAC = "0e:aa:aa:aa:aa:aa"
BATTERY_MAC = "0e:bb:bb:bb:bb:bb"
BUTTON_SERVICE_UUID = "0000a000-0000-1000-8000-00805f9b34fb"
BATTERY_SERVICE_UUID = "0000180f-0000-1000-8000-00805f9b34fb"
SOUNDS_DIRECTORY = "/home/pi/App/sounds/"
LAST_SEEN_MAX = 31 * 60 #1 extra minute just to be sure
NOTIFIED_LOST = False
slackman = None #Can throw exceptions so we'll set it up later
playing = False

def getSoundPath(basePath):
	files = [f for f in os.listdir(basePath) if os.path.isfile(os.path.join(basePath, f))]
	mp3files = [f for f in files if ".mp3" in f]
	return basePath + random.choice(mp3files)
	
def playSound(soundPath):
	global playing
	playing = True
	pygame.mixer.init() #for playing audio later
	print("Playing sound: " + soundPath)
	pygame.mixer.music.load(soundPath)
	pygame.mixer.music.play()

def handlerButton(device=None):
	global slackman
	#Notify the slackers
	slackman.notify()
	#dont stop the music
	soundPath = getSoundPath(SOUNDS_DIRECTORY)
	playSound(soundPath)
	
	#connect and disconnect to ACK:
	for i in range(0, 10):
		try:
			button = btle.Peripheral(device.addr, device.addrType, device.iface)
			button.disconnect()
			break
		except BTLEException:
			pass
	
	#Avoid repeated sounds by waiting
	#No longer needed with new ding-code
	#time.sleep(4.5)
			
def handlerBattery(device):
	for i in range(0, 10):
		try:
			button = btle.Peripheral(device.addr, device.addrType, device.iface)
			buttonService = button.getServiceByUUID(BATTERY_SERVICE_UUID)
			for characteristic in buttonService.getCharacteristics():
				if characteristic.supportsRead(): #then fuckin read it!
					batteryLevel = int(characteristic.read().encode('hex'), 16)
					print("Battery: ", batteryLevel)
					slackman.batteryLevel = batteryLevel
			button.disconnect()
			break
		except BTLEException:
			pass

def scanForBLEButton(scanner, timeout):
	foundButton = False
	scanHandlers = {BUTTON_MAC: handlerButton,
				BATTERY_MAC: handlerBattery}
	devicesFound = scanner.scan(timeout)
	for device in devicesFound:
		if device.addr in scanHandlers and device.connectable:
			print(device.addr, device.connectable, device.rssi)
			scanHandlers[device.addr](device) #Run handler
			foundButton = True
	return foundButton
					
def checkLastSeen(lastSeen, maxSecondsDiff):
	global slackman
	global NOTIFIED_LOST
	diff = datetime.now() - lastSeen
	if diff.seconds > maxSecondsDiff:
		if not NOTIFIED_LOST:
			print("Not seen for " + str(maxSecondsDiff / 60) + " minutes :(")
			slackman.warning()
			NOTIFIED_LOST = True
		return False
	return True

def main():
	global slackman
	global playing
	global NOTIFIED_LOST
	print("Welcome to ding dang!")
	scanner = btle.Scanner()
	lastSeen = datetime.now()
	slackman = Slackman()
	slackman.start()
	print("Waiting for BLE button...")
	while True:
		try:
			if slackman.is_manding():
				handlerButton()
			if scanForBLEButton(scanner, 0.5):
				lastSeen = datetime.now()
				if NOTIFIED_LOST:
					slackman.back_to_life()
					print("What was once lost is now found!")
					NOTIFIED_LOST=False
				
			checkLastSeen(lastSeen, LAST_SEEN_MAX)

			if playing and not pygame.mixer.music.get_busy():
				playing = False
				pygame.mixer.quit() #prevents static noise
			
		except KeyboardInterrupt:
			print("Bye!")
			break
	slackman.alive = False
	pygame.mixer.quit()

if __name__ == "__main__":
	try:
		main()
	except:
		errorMsg = traceback.format_exc()
		print("A bad happened! [%s]" %(datetime.now().isoformat()))
		print(errorMsg)
		if slackman:
			slackman.error(errorMsg)


