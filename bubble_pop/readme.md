#### Remake of Bubble Puzzle 97

One of the earliest PC games I remember playing as a teenager was Bubble Puzzle 97, a freeware puzzle-bobble style clone. I recently rediscovered the title which, unfortunately, doesn't work on modern computers as it was written for Windows 95 using directX 3. This repository attempts to recreate the original game, using the assets distributed with the original. This is purely for nostaligia's sake... while this source code is released under the apache license because it is written from scratch, the assets retain the original license (below) and MUST NOT be distributed for commericial purposes. Of course, as this is an open source project, you may feel free to replace all the resources (including the level designs!) with your own.

Oh and please don't hold me culpable for the ridiculous difficulty of some levels - I didn't make them! :D

#### Original readme

   __________________
    Bubble Puzzle 97                                      February, 1998
   __________________
   __________________

S +---------------------------------------------------------------------+
H | Put three or more bubbles of the same colour together, and they will|
O | drop off. All bubbles hanging below them will drop down, too. The   |
R | bubbles bounce off the sides, therefore you can "bank" a shot at a  |                    
T | target. A level is completed when all bubbles are gone. The game is |                 
! | over when a bubble is fixed too low (Red Area).                     |
 +----------------------------------------------------------------------+

  +---------------------------------------------------------------------+
K | Bringe drei oder mehr Kugeln der gleichen Farbe zusammen, damit sie |
U | platzen. Alle Kugeln, welche darunter hängen fallen herab und ver-  |
R | schwinden somit auch. Die Kugeln prallen von den Banden ab. Ein     |
Z | Level ist geschafft, wenn alle Kugeln fort sind. Das Spiel ist be-  |
! | endet, sobald sich eine Kugel zu tief (im roten Bereich) befindet.  |
  +---------------------------------------------------------------------+

ENGLISH
- introduction
- about the game
- levels
- highscore
- controls
- system requirements
- the "Conmeg Config Tool"
- warranty / licence / disclaimer
- troubleshooting
DEUTSCH
- Einleitung
- über das Spiel
- Level
- Highscore
- Steuerung
- Systemanforderungen
- Das "Conmeg Config Tool"
- Garantie und Lizensbestimmungen
- Fehlerbehebung

___________________________________________________________________________


     ENGLISH 
  °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°


__introduction__________________________

Welcome to Bubble Puzzle 97!
This is the latest version of Bubble Puzzle. 
You say, You've seen a game like this before ? Uh... 
You're joking, aren't you?
Perhaps, there could be some game that resembles Bubble--
which might be a little like Bubble Puzzle--but there
is only one original... and now you've got it.
Lucky You!

The game's main purpose was to test our routines,
developed to create bigger games. It is the first of 
three projects, and started as our introductory works for
the Windows 95/Direct-X platform. We thought 
this would make our former problems, inherent in a thousand 
different hardware configurations, obsolete; but 
we had to learn that there are many other problems
compromising this advantage. 

Nevertheless, Bubble comes out as freeware, too. You're
free to copy it, play it, give it to your friends, but please
mail us. Tell us how you like the game, who you are,
where you found our Bubble Puzzle, your blood type, 
the maiden name of your mother, what TV channel you prefer, 
or anything you want.
See--we will never be able to realize all our great ideas,
but we want to know you--our "customers"--and strive for
the things you want.

__about the game________________________

The goal of the game is quite simple. Put
three or more bubbles (balls) of the same colour 
together, and they will drop off the playing surface.
OK, heard that quite often? Kind of Tetris-like you think? Sure, 
it sounds simple, but read ahead--or play it--before you judge it.
You are in control of a 'cannon' that fires one of 
those bubbles into the playing field. The 
cannon can only be rotated to the left or to the 
right so that you have to aim carefully to hit the 
places you want to hit. As soon as the bubble collides,
the cannon will be reloaded with the next one. If your misses 
don't drive you nuts, the sound effects will!  You will 
have to admit, however, that the Bubble's facial expressions 
are adorable......

- A level is completed when no balls are left on the
  playing field.
- The game will be over when at least one bubble is 
  positioned in the red zone at the bottom of the 
  playing field.

There are some additional(and more important things)to know:

- The bubbles only stick to each other and to the top
  of the playing field. Therefore, you are able to play
  via the sides of the field. The bubble will bounce 
  from it like pool balls.
- If three or more bubbles fall down, all bubbles 
  hanging below them will drop down and disapear, too, 
  so You can remove many bubbles from the screen with a
  single, well aimed, shot.
- In most of the levels, in the course of time, the top 
  of the playing field drops down one one step. 
  This could put tome bubbles under the red area, 
  so that the game would be lost.
- In a few levels, the cannon moves from one side to the
  other. This movement cannot be influenced by the player.
  You just have to live with it.


__levels________________________________

With Bubble Puzzle 97 our new Levelsystem is introduced.
There are 50 default levels includes with it.
If you play through all of them (ha ha!), you will be 
confronted with our well-known random levels from the
DOS-version.
Levels are textfiles "*.ini", positioned in the "level"
directory. You can create your own levels using any text
editor. Save them in the level directory, and they will
be included and sorted by their stage number.
For further information look at the "empty.ini" level.

Normally you begin each game on the first level, but you
are also be able to play your "Personal Profile". If you
do so, Bubble Puzzle always saves the highest level you
reached and lets you begin each game right there.

To play a profile, just enter (or pick) its name in the 
starting dialogue. To play without any profile, leave the
profile field empty.

You can also play Bubble Puzzle 97 like Bubble Puzzle for
DOS. Just delete or rename the "level" directory.


__highscore_____________________________

The ten highest scores are saved in the high score record. 
If You reach a score which is worthy of being entered among the 
high scores, this is mentioned on the screen by a small number
above your score. If your game score is high, you will
be asked to enter your name to the high score table.

Next to your name, Bubble Puzzle 97 saves a code which
is influenced by the set of levels you played. If
you ever enter some new levels, delete some out of the
level directory, or change some existing levels, the code 
will change. Therefore you will be able to note whether another 
player got the highscore by playing a different (easier?) level
set and crow if you have beaten him.

To reset the high score table just delete the highscore.ini file from the
games directory.


__controls______________________________

The game can either be controlled by keyboard or by mouse.

_Function____    _keyboard____   _mouse____
rotate left      [cursor left]   move to the left
rotate right     [cursor right]  move to the right
fire bubble      [ctrl]          click on mouse button
pause game       [P]             ---
quit game        [Esc]           ---
name entry       [a]...[z]       ---


__system requirements___________________

You need at least a P-90/SuperVGA system with Windows95
and DirectX 3.0 installed.


__the 'Conmeg Config Tool'______________

This config tool creates a configuration file that is used
by all games from Conmeg. The file will be saved as
"c:\dxerrors.ini" with the hidden attribute set.
This configuration includes basic settings like language
and some special settings to improve the performance of our
games. 


__warranty / licence / disclaimer_______

Bubble Puzzle 97 is copyrighted 1997 by Conmeg Spielart. 
(Oliver Pape, Frank Sander, Stefan Spittank, Marc Störzel)

You are hereby granted permission to freely distribute and 
use this program for personal use and entertainment. You may
upload this program to electronic bulletin boards or the Internet.
You are specifically prohibited from selling or charging or
requesting any monetary amount for the distribution or use of
Bubble Puzzle 97 or accompanying documentation. It may not 
be used for any commercial purposes without the prior express
written permission of Conmeg Spielart. Bubble Puzzle 97 may 
not be distributed for use with any for-profit products 
(software, hardware, or other; including, but not limited to, 
commercial software, shareware, etc.) without the prior
express written permission of Conmeg Spielart. There is no 
warranty of any kind; this software is provided on an "AS-IS"
basis. The author of this software shall not be liable for
any damages whatsoever that may result, directly or indirectly,
from the use of this software or documentation. By owning a 
copy of and/or using Bubble Puzzle 97, you agree to all the 
above terms.


__troubleshooting_______________________

Bubble Puzzle controls the hardware totally via DirectX.
If any problems occur which look like hardware problems, 
check Your DirectX installation or look out for the newest 
version of Your DirectX drivers.

If there are any other problems, or problems you are not
sure are hardware-based, don't be afraid to contact us via email: conmeg@iname.com.

known bugs

- If you switch back from another application, the game will 
  crash.
- The game doesn't work on some WinNT-platforms.



______________________________________________________________




     DEUTSCH
  °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°


__Einleitung____________________________

Willkommen bei Bubble Puzzle 97!
Die ist die neuste Version des Spieles Bubble Puzzle.
Wie bitte? Du hast so ein Spiel schon mal gesehen ?
Ahem.... Vieleicht gibt es irgendwo ein Spiel, wessen
Prizip in irgendeiner Art an Bubble Puzzle erinnert,
aber es gibt nur ein Original ! (Hüstel...)

Das Spiel war hauptsächlich gedacht, um unsere Rou-
tinen zu testen, die eigentlich dafür gedacht sind, 
"größere" Spiele zu verwirklichen. Es ist das erste
von drei Projekten, welche unssere Arbeit für die
Windows 95 / DirectX Plattform einleiten. Wir dachten,
daß damit frühere Probleme von tausenden verschiedenen
Rechnerkonfigurationen wegfallen würden, aber wir 
mussten lernen, daß dafür andere Probleme hinzukamen,
welche diesen Vorteil wieder aufhoben.

Auf jeden Fall kommt Bubble Puzzle 97 als Letterware
zu Euch. Ihr könnt es frei kopieren, spielen, weiter-
reichen usw., aber bitte, bitte schreibt uns.
Sagt uns, wie Ihr das Spiel findet, wer Ihr seid, 
woher Ihr Bubble Puzzle bezogen habt, welche Kino-
filme Ihr mögt, oder was immer Ihr wollt.
Wir können leider (noch) nicht alle unsere (tollen)
Ideen verwirklichen, so daß wir auf das Wissen Eurer
Meinungen und Vorlieben angewiesen sind.
Und wir stecken viel Zeit und Arbeit in unsere Spiele,
wobei etwas feedback immer wieder unsere Motivation 
zu steigern weiß !


__über das Spiel________________________

Der Hauptaspekt des Spiels ist recht einfach. Bringe
drei oder mehr Blasen (Kugeln) gleicher Farbe zu-
sammen - und sie werden platzen. OK, so etwas hörte
man schon des öfteren? Sowas wie Tetri... ? Klar,
das klingt simpel, aber lies erst einmal weiter oder
spielt es, bevor Ihr es verurteilt.
Du kontrolliert die "Kanone", welche diese Blasen in
das Spielgeschehen katapultiert. Diese Kanon kann 
von Dir nur nach rechts oder links gedreht werden,
so daß Du vorsichtig zielen mußt, um dorthin zu 
treffen, wo die Blase auch hin soll. Sobald die 
abgeschossene Blase irgendwo hängen bleibt, wird die
Kanone mit der nächsten geladen.

- Ein Level ist geschafft, sobald keine Blasen mehr
  auf dem Spielfeld sind.
- Das Spiel ist beendet (Game Over), sobald zumin-
  dest eine Blase in der roten Zone am underen
  Spielfeldrand positioniert ist.

Aber das sind noch ein paar wichtige Dinge:

- Die Blasen haften nur an anderen Blasen oder am
  oberen Bildschirmrand. Also kann man schön über die
  Bande - den Seitlichen Spielfeldrand - spielen.
- Wenn drei oder mehr Blasen platzen, fallen alle Ku-
  geln, die somit keine Verbindung mehr zum oberen 
  Bildschirmrand haben natürlich auch herunter. So kann
  man mit einem gezielten Schuß manchmal ganz schön
  viele Kugeln verschwinden lassen.
- In den meisten Levels kommt die Spielfelddecke von
  Zeit zu Zeit ein wenig weiter nach unten. Dies kann
  auch Kugeln in die rote Game Over Zone drücken.
- In einigen Leveln bewegt sich die Kanone von
  rechts nach links und umgekehrt. Hierauf hat 
  der Spieler nicht den geringsten Einfluß. Du mußt
  damit leben !


__Level_________________________________

Mit Bubble Puzzle 97 führen wir unser neues Levelsystem
ein. Es werden 50 Level mitgeliefert.
Wenn Du sie alle durchgespielt hast (haha!), wirst Du
mit den altbekannten Zufallsleveln aus der Dos Version
konfrontiert.
Die Level sind als Textdateien "*.ini* in Verzeichnis 
"Level" abgespeichert. Ihr könnt eigene Level mit jedem
Texteditor erzeugen und in dem Verzeichnis ablegen.
Sie werden dann anhand ihres "stage" Wertes in die
Levelreihenfolge einsortiert. Für weitere Informationen
schau Dir den leeren Level "empty.ini" an.

Normalerweise beginnt man jedes Spiel im ersten Level,
aber Ihr könnt auch mit Eurem persönlichen Profil spielen.
In diesem Profil ist immer abgespeichert, welchen Level 
Ihr bereits erreicht habt, und genau dort startet für Euch
dann auch jedes Spiel. 

Um ein Profil zu spielen müsst Ihr nur dessen Namen in
dem Startdialog eingeben oder dort auswählen. Um ohne
Profil zu spielen (immer wieder von vorn), lasst das Profil-
feld einfach leer.

Ihr könnt Bubble Puzzle 97 auch wie Bubble Puzzle für 
DOS spielen. Dazu barucht Ihr lediglich das Verzeichnis
"Level" umzubenennen oder zu löschen.


__Highscore_____________________________

Die zehn höchsten erreichten Punktzahlen werden in der
Highscore gespeichert. Wenn Ihr einen dieser Plätze im
Spiel erreicht, wird dies durch die kleine Zahl über der
Punktezahl angezeigt. Wenn das Spiel vorbei ist (Game 
over), könnt Ihr für diesen Platz Euren Namen eingeben.

Neben Eurem Namen wird auch ein Code gespeichert,
welcher vom gespielten Levelset abhängt. Immer wenn
neue Level hinzukommen, alte geändert oder gelöscht 
werden, ändert sich dieser Code. So könnt Ihr immer
sehen, ob ein anderer Spieler seinen Highscore-eintrag
mit denselben Leveln wie Ihr, oder mit anderen (leichte-
ren?) geschafft hat.

Um die Highscore zurückzusetzen, löscht einfach die 
Datei "BubbleHighscore.ini" im Spielverzeichnis.


__Steuerung_____________________________

Das Spiel kann entweder mit Tasttatur oder Maus ge-
steuert werden.

Funtion____       _Tastatur____   _Maus____
drehen links      [Pfeil links]   Maus links
drehen rechts     [Pfeil rechts]  Maus rechts
Blase abschießen  [ctrl]          Maustaste
Pause             [P]             ---
Spiel beenden     [Esc]           ---
Namenseintrag     [a]...[z]       ---


__Systemanforderungen___________________

Ihr benötigt zumindest einen P-90 / superVGA Rechner
mit Windows 95 und DiectX 3.0.


__Das "Conmeg Config Tool"______________

Dieses Konfigurierungsprogramm erzeugt eine Konfigura-
tionsdatei für alle Conmeg Spiele ("C:\DXERRORS.INI"),
welche das "Hidden"-Attribut gesetzt hat.
Diese Konfiguration beinhaltet Basiseinstellungen, wie
z.B. die Sprache, und einige Spezialeinstellungen, um
die Performance zu erhöhen.


__Garantie und Lizensbestimmungen_________

Bubble Puzzle 97 ist urheberrechtlisch geschützt 
(1997 Conmeg Spielart:
 Oliver Pape, Frank Sander, Stefan Spittank, Marc Störzel)

Ihr erhaltet hiermit die Erlaubnis, Bubble Puzzle 97 frei
zu verbreiten und für privaten Gebrauch und Unterhaltung
zu benutzen. Ihr dürft die Software unverändert (auch auf
elektronischem Wege (Internet & co.)) weiterreichen.
Conmeg Spielart verbietet ausdrücklich die gewerblich 
Nutzung dieser Software oder seiner Dokumentation.
Es darf nicht in irgendeinem Sinne, ohne die ausdrückliche
schriftliche Genehmigung von Conmeg Spielart, dazu genutzt
werden, Profit zu machen - im besonderen nicht durch die
Verbreitung auf oder mit kommerziel vertriebenen Produkten.
Der Autor dieser Software ist in keinster Weise belangbar
für jedwelche Schäden, die - direkt oder indirekt - durch
den Gebrauch von Bubble Puzzle 97 oder seiner Dokumentation
entstehen.
Durch den Besitz oder das Benutzen von Bubble Puzzle 97 
stimmt Ihr allen obigen Punkten zu.

__Fehlerbehebung________________________

Bubble Puzzle 97 spricht die Hardware über DirectX and.
Wenn also Probleme auftreten, welche nach einem Hardware-
problem aussehen, überprüft Eure DirectX Installation
oder besorgt Euch die jeweils neuestem DirectX Treiber.

Falls andere Probleme auftreten, welche nicht auf DirectX
schließen lassen, scheut Euch nicht, uns zu kontaktieren:
email an conmeg@iname.com

Bekannte Fehler

- Beim zurückwechseln von einer anderen Anwendung zu 
  Bubble Puzzle stürzt es ab.
- Auf einigen WinNT Systemen läuft das Spiel gar nicht.

-- 
Frank Sander | Conmeg Spielart
English Version updated and corrected by Nancy J. Belford,
thank you Nancy!

visit our Homepage:
http://www.geocities.com/TimesSquare/dungeon/5885/index.html
or mail us: conmeg@iname.com

Windows 95, Windows NT and DirectX are trademarks 
owned by Microsoft (TM).
