Mu-lighting is an entertaining music playing system running over NDN. It has two features: music sharing among multiple devices and using the metadata of music to control lighting. Music Sharing allows the music player to stream music with little to no setup. If a storage device inside a network sets up the sharing function, any music player on that network can see and fetch data from it. Besides, the music player will control the lighting according to the frequency and onset data of a song.

To begin with, the storage devices will first insert all its music data into the NDN repository, (https://github.com/named-data/repo-ng). 
When the user wants to order song, an updated song list will be displayed and he/she can simply choose a song and a music player to play it. 
The network will appoint the chosen music player to fetch the requested music information from the repo. 
When music player starts to play the music, it will simultaneously send RGB command to control the lighting.



storage1.py and storage2.py will scan a certain folder(music-file and music-file2) and insert all its music files into the NDN repository. They call pyPutFile.py to interact with NDN repository.
musicPlayer.py is used to fetch data from NDN repo, play music and send command to lighting. It will call pyGetFile.py to fetch data from NDN repository.
controller.py can collect the song list, then user can choose a song and a music player to play it.

Mengchen


