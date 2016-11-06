import bluepy.bluepy.btle as btle
import pygame, os, random, traceback, time, sys
from datetime import datetime
from slackman import Slackman
import RPi.GPIO as GPIO

SOUNDS_DIRECTORY = "/home/pi/App/sounds/"
LAST_SEEN_MAX = 31 * 60 #1 extra minute just to be sure
NOTIFIED_LOST = False
slackman = None #Can throw exceptions so we'll set it up later
playing = False
notifySlack = True

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

def handlerButton():
	global slackman
	#Notify the slackers
	if notifySlack:
		slackman.notify()
	#dont stop the music
	soundPath = getSoundPath(SOUNDS_DIRECTORY)
	playSound(soundPath)
	
	#Avoid repeated sounds by waiting
	time.sleep(5.0)
			
def handlerBattery(batteryLevel = -1, deviceID = "Ukjent dinger"):
	print("Battery for %s: " % deviceID, batteryLevel)
	slackman.batteryLevels[deviceID] = batteryLevel

def scanForBLEButton(scanner, timeout):
	foundButton = False
	scanEntries = scanner.scan(timeout)
	for scanEntry in scanEntries:
		entryIsButton = False
		for (adtype, desc, value) in scanEntry.getScanData():
			if int(adtype) == 9 and "HS_" in value: #Complete local name
				print(scanEntry.addr, scanEntry.connectable, scanEntry.rssi)
				entryIsButton = True
				foundButton = True
		for (adtype, desc, value) in scanEntry.getScanData():
			print("%s %s = %s" % (adtype, desc, value))
			if entryIsButton and int(adtype) == 22: #Service data
				data = int(value[0:2], 16)
				if data >= 128:
					handlerButton()
				battery = data & ~(1 << 7)
				deviceID = value[2:8]
				handlerBattery(battery, deviceID)
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
	global notifySlack
	print("Welcome to ding dang!")
	print(sys.argv, len(sys.argv))
	if len(sys.argv) > 1 and sys.argv[1] == "noslack":
		print("no slack plz")
		notifySlack = False
	scanner = btle.Scanner()
	lastSeen = datetime.now()
	slackman = Slackman()
	slackman.start()
	print("Setting up GPIO...")
	GPIO.setmode(GPIO.BOARD)
	inputPins = [36, 38, 40]
	for pin in inputPins:
		GPIO.setup(pin, GPIO.IN, GPIO.PUD_UP)
		GPIO.add_event_detect(pin, GPIO.FALLING)
	print("Waiting for BLE button...")
	while True:
		try:
			if slackman.is_manding():
				handlerButton()
			for pin in inputPins:
				if GPIO.event_detected(pin):
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
			GPIO.cleanup()
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


