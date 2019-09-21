Desert Island Duel
------------------

A 1-4 player multplayer online game (with bots), Desert Island Duel pits four mighty pirates against each other in their search for treasure. Follow your map, dig in the sand and reap the booty! Sound simple? Don't count on it! Not only are there 3 other pirates awaiting to thwart your every move there are skeletons at night (thanks to the full day/night cycle), booby traps, bees, bad weather and more!



##### How to play
Each player starts on one corner of the island, near their boat. Using the map (press M to enlarge) dig at the marked points to uncover treasure chests, coins, or ammo. On uncovering a chest grab it and return it to your boat before another player takes it! Of course the pirating doesn't stop there - if you see somebody else's boat unattended, grab their treasure for yourself, quick!

Besides other players the island contains several hazards - during the day players may be chased by swarms of bees which can be deterred by jumping in the sea or placing down a decoy. Bees will also hide when it rains or thunders. During the night the skeletal remains of previous treasure hunters will emerge and continue their never ending quest for treasure! The skeletons can also be distracted with decoys, but fear the light too, which damages them, so make sure to carry a lantern with you! Be warned, these skeletons may make off with any unattended treasure lying around... If you kill a skeleton it might drop a coin or two.

Barrels frequently wash ashore and can contain health (in the form of food), coins or ammo, items such as flares or decoys, or may even be booby trapped with gunpowder so break them open with care! Sometimes a barrel may merely contain a harmless sea-dweller.

###### Items

 * Flares. These look like rockets and are found in barrels. Launch one of these to call in a barrage of canon fire from your offshore galleon.
 * Decoys. The creepy looking decoys are actually your friend! Place one down to distract bees or skeletons that may otherwise attack you.
 * Spooky Skull. These haunted skulls will, when placed, put up a small shield which will curse any player who touches it, reversing their controls for a short amount of time. Place one by your boat for added security!


###### Weather
Weather comes and goes on the the island - while it's mostly sunny it can and will rain sometimes. Occasionally a full on storm will kick in, so watch out for lightning strikes, they tend to prove lethal...

###### Fighting
Every pirate has a trusty sword as well as a pistol. Pistols are powerful, have a long range, but have limited ammo, so keep an eye out for barrels which may contain more ammo. When fighting remember to use the strafe button so you can continue to face your opponent while dodging, weaving and delivering those mighty blows.

###### Default Controls
    W - Up  
    S - Down  
    A - Left  
    D - Right  

    LCtrl/Controller B - Pick up/drop  
    Space/Controller A - Use Item  
    LShift/Controller LB - Strafe  

    Q/Controller RB - Previous Item  
    E/Controller Y - Next Item  
    Z/Controller X - Show/Zoom Map  
    M/Controller Back - Show Map  
    Tab - Show Scoreboard  

    Esc/P/Pause/Controller Start - Show options Menu  

This assumes the default xinput style controller layout on Windows. Custom keybinds and controller input is coming Soon™

###### Hosting a Game
Click host on the main menu to host a new game. The host can shoose to update the 'seed' a random word or phrase which is used to change how the island is generated, it can be almost anything you like! Remember the seeds for your favourite islands as using the same seed will regenerate the same one each time (although not necessarily with the same treasure...). Hosting requires port 20715 to be open on the host computer, and any relevant NAT/port forwarding done to allow other players to join across the internet.

###### Joining a Game
To join a game, hit join from the main menu. When prompted enter a name for your character, and the IP address of the game you'd like to join. Port 20715 will need to be open for outgoing connections. Click join to attempt to join the host's lobby.

When in a lobby as either a host or a guest, make sure to check the 'Ready' button below your player avatar. The host cannot start the game until everyone is ready.

###### Console Commands
While the game is running there are a few console commands available. Open the console by pressing F1 and enter the following:

    set_storm 0,1,2 - sets weather to nice, rain or storm (host only)
    bots_enabled 0, 1 - enable server side bots (host only)
    spawn <item> - spawns the given item at the player’s position (host only)
        decoy_item
        decoy
        crab
        flare_item
        flare
        skull_item
        skull_shield
    seed - shows the current map seed

Note some of the above commands are only available to the current game host. For a list of available commands at run time type `help`.
