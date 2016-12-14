import bluepy.bluepy.btle as btle
import pygame, os, random, traceback, time, sys
from datetime import datetime
from slackman import Slackman
import RPi.GPIO as GPIO
import logging

SOUNDS_DIRECTORY = "/home/pi/App/sounds/"
LOG_DIRECTORY = "/home/pi/App/logs/"
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
	logging.info("Playing sound: " + soundPath)
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
	logging.info("Battery for %s: " % deviceID, batteryLevel)
	slackman.batteryLevels[deviceID] = batteryLevel

def scanForBLEButton(scanner, timeout):
	foundButton = False
	scanEntries = scanner.scan(timeout)
	for scanEntry in scanEntries:
		entryIsButton = False
		for (adtype, desc, value) in scanEntry.getScanData():
			if int(adtype) == 9 and "HS_" in value: #Complete local name
				logging.info(str(scanEntry.addr), str(scanEntry.connectable), str(scanEntry.rssi))
				entryIsButton = True
				foundButton = True
		for (adtype, desc, value) in scanEntry.getScanData():
			logging.debug("%s %s = %s" % (adtype, desc, value))
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
			logging.warning("Not seen for " + str(maxSecondsDiff / 60) + " minutes :(")
			slackman.warning()
			NOTIFIED_LOST = True
		return False
	return True

def main():
	global slackman
	global playing
	global NOTIFIED_LOST
	global notifySlack
	logging.basicConfig(filename=LOG_DIRECTORY + '/dingers.log',level=logging.DEBUG, format='%(asctime)s %(message)s')

	logging.info("Welcome to ding dang!")
	logging.info(sys.argv, len(sys.argv))
	if len(sys.argv) > 1 and sys.argv[1] == "noslack":
		logging.info("no slack plz")
		notifySlack = False
	scanner = btle.Scanner()
	lastSeen = datetime.now()
	slackman = Slackman()
	slackman.start()
	logging.info("Setting up GPIO...")
	GPIO.setmode(GPIO.BOARD)
	inputPins = [36, 38, 40]
	for pin in inputPins:
		GPIO.setup(pin, GPIO.IN, GPIO.PUD_UP)
		GPIO.add_event_detect(pin, GPIO.FALLING)
	logging.info("Waiting for BLE button...")
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
					logging.warning("What was once lost is now found!")
					NOTIFIED_LOST=False
				
			checkLastSeen(lastSeen, LAST_SEEN_MAX)

			if playing and not pygame.mixer.music.get_busy():
				playing = False
				pygame.mixer.quit() #prevents static noise
			
		except KeyboardInterrupt:
			logging.info("Bye!")
			GPIO.cleanup()
			break
	slackman.alive = False
	pygame.mixer.quit()

if __name__ == "__main__":
	try:
		main()
	except:
		errorMsg = traceback.format_exc()
		logging.error("A bad happened! [%s]" %(datetime.now().isoformat()))
		logging.error(errorMsg)
		if slackman:
			slackman.error(errorMsg)


