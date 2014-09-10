#!/usr/bin/env python
#coding=utf-8

# Listen for play request, extract song data from repo, play music, send command to lighting
#
# code by Mengchen, Zhehao
# Aug 21, 2014
#


import sys
import time,datetime
import Queue

from ConfigParser import RawConfigParser
from random import randrange
from pyndn import Name, Data, Interest, ThreadsafeFace, Exclude,Blob, Sha256WithRsaSignature
from pyndn.security import KeyChain
from pyGetFile import GetFile
from pyndn.encoding import ProtobufTlv
from LightMessenger import LightMessenger

import trollius as asyncio
import logging
import thread, threading
import pygame
import trollius as asyncio
from trollius import Return, From

logging.getLogger('trollius').addHandler(logging.StreamHandler())

class MusicPlayer:
	def __init__(self):
		self._queue = Queue.Queue()
		self._isPlaying = False
		self._lightThread = threading.Thread()
		self._lightThread.daemon = True
		self.config = RawConfigParser()
		self.config.read('config.cfg')
		self.s = LightMessenger(self.config)
		#pygame is a tool to play music
		pygame.init()
		pygame.mixer.init()
		
	def send_lighting(self, musicName):

		txt = open(musicName + str("-os.txt"), "r")
		osDur = [0.0]
		freq = []
		#read the onset point from file
		data = [float(line.split()[0])for line in txt]
		for i in data:
			osDur.append(i)
		txt.close()
		#read frequency data from file	
		txt = open(musicName + str("-freq.txt"), "r")
		data = [float(line.split()[1])for line in txt]
		for j in data:
			freq.append(j)
		#calculate the average frequency and map it to a RGB value
		avefq = int(sum(freq)/len(freq)) 
		print "music:", musicName,"avefq:",avefq
		txt.close()		
		r = (150-avefq)*1.2
		g = avefq/3
		b = (avefq-100)*1.2
		startingColors = [int((150+r)),int((100+g)), int((100+b))]
		for i in range(0,3):
			if startingColors[i]<0 :
				startingColors[i] = 0
			elif  startingColors[i]>255:
				startingColors[i] = 255
	
		if not self._lightThread.isAlive():
			self.s = LightMessenger(self.config)
			#start to send lighting command, in the mean time, start to load the music
			self._lightThread = threading.Thread(target = self.s.start, args = (osDur, startingColors,))
			self._lightThread.daemon = True
			self._lightThread.start()
			print "lightThread started"
		else:
			print "lightThread is still alive"
		
		return

	def unix_time_now(self):
		epoch = datetime.datetime.utcfromtimestamp(0)
		delta = datetime.datetime.utcnow() - epoch
		return delta.total_seconds() * 1000.0
		
	def play_music(self, musicName, preEmptive):
		
		#the ordered song will be stored at a queue and wait for playing
		if not preEmptive:
			self._queue.put(musicName)
		
			if not self._isPlaying:
				self._isPlaying = True
				while not self._queue.empty():
					name = self._queue.get()
					print "**** play: ",name,"****"
					
					self.send_lighting(name)
					#start to load the music
					print "~~~start to load......",datetime.datetime.now()
					
					pygame.mixer.music.load(name + "-music.mp3")
					print "~~~start to play the music",datetime.datetime.now()
					print self.unix_time_now()
					#start to play the music
					pygame.mixer.music.play()
					while pygame.mixer.music.get_busy():
					   try:
						time.sleep(0.2)
					   except KeyboardInterrupt:
						os._exit(1)
		
					
					
				self._isPlaying = False
		return
		
class SongHandler:

	def __init__(self):

		self._device = "PC1"
		self._playPrefix = Name("/ndn/edu/ucla/remap/music/play")
		self.prefix = self._playPrefix.append(self._device)		
		self._face = None
		self._loop = None
	
		self._keyChain = KeyChain()
		self._certificateName = self._keyChain.getDefaultCertificateName()

		self._repoCommandPrefix = "/example/repo/1"

		self.mp = MusicPlayer()
		
	def start(self):
		self._loop = asyncio.get_event_loop()
		self._face = ThreadsafeFace(self._loop,"")
		self._face.setCommandSigningInfo(self._keyChain, self._certificateName)
		self._face.registerPrefix(self.prefix, self.onPlayingCommand, self.onRegisterFailed)
	
		try:
			self._loop.run_forever()
		except KeyboardInterrupt:
			sys.exit()
		finally:
			self.stop()

	def stop(self):
		self._loop.close()	
		self._face.shutdown()
		sys.exit(1)

	def getOnset(self, musicName):
		otxt = musicName + str("-o")
		print otxt
		#get onset data file, it will then run getFreq after finishing download
		g = GetFile(self._repoCommandPrefix, otxt, self._face, self.getFreq, musicName)
		g.oStream = open(musicName + str("-os.txt"), 'wb')
		g.start()


	def getFreq(self, musicName):
		ftxt = musicName + str("-f")
		#get freq file, it will run play_music after finishing download
		print musicName, " get Freq called"
		g = GetFile(self._repoCommandPrefix, ftxt, self._face, self.mp.play_music, musicName, False)
		g.oStream = open(musicName + str("-freq.txt"),'wb')
		g.start()


	def signData(self, data):
		data.setSignature(Sha256WithRsaSignature())
	
	def onPlayingCommand(self, prefix, interest, transport, prefixId):
		
		interestName = Name(interest.getName())
		commandComponent = interest.getName().get(self.prefix.size())
		if commandComponent == Name.Component("stop"):
			#TODO: stop the process of playing music and lighting simultaneously
			pass
		if commandComponent == Name.Component("play"):
			pass
		else:
			songName = commandComponent.toEscapedString()
	    
		songList = []
		songList = songName.split('%2C')
	    
		for i in songList:
			fmusic = i + str("-music.mp3") 
			print "started getting music file", i
			#get mp3 file, it will run getOnset after finish downloading mp3 file
			g = GetFile(self._repoCommandPrefix, i, self._face, self.getOnset, i)
			
			g.oStream = open(fmusic,'wb')
			g.start()
			
			#self._getFiles.append(g)
			
		d = Data(interest.getName())
		d.setContent("start to play: " + songName + "\n")
		encodedData = d.wireEncode()
		transport.send(encodedData.toBuffer())	

	def onRegisterFailed(self, prefix):
		self.log.error("Could not register " + prefix.toUri())
		self.stop()

if __name__ == '__main__':
	logging.basicConfig()
	sh = SongHandler()
	sh.start()
    















	
