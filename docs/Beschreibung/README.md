# GPS-Oiler-2

## Mein GPS-OILER-2
Ich hatte mir ein neues Motorrad gekauft und wollte mir einen Mikrocontroller gesteuerten Kettenöler einbauen. 

Da ich ein Bastler bin und mit dem Arduino auch schon einiges gemacht hatte wollte ich mir einen eigenen Kettenöler bauen.

Da ich nicht an das Tachosignal gehen wollte musste ich mir etwas anderes überlegen. Einen Magneten irgendwo anzukleben und einen Reed Kontakt zu verwenden fand ich auch nicht so elegant. Aus diesem Grund habe ich mich für ein GPS Modul entschieden.

Ich wollte das ganze recht universell halten und habe das Mainboard und das GPS-Modul von einander getrennt. So kann man das Mainboard an einem Platz installieren wo kein Sat-Empfang möglich ist.

Um zu sehen ob alles vernünftig funktioniert habe ich dem Kettenöler ein kleines 0,96“ OLED Display spendiert das mir alle notwendigen Informationen anzeigt wie Geschwindigkeit, Füllstand des Öltank, bereits gefahrene km seit dem befüllen des Öltank, Richtung, Anzahl der empfangenen Satelliten, ob er gerade ölt, ob der Regenmodus aktiviert ist.

Das die Kette auch bei Regen entsprechend geölt wird habe ich einen Regenmodus eingebaut. Für die Regenerkennung benutze ich einen Regensensor der sich unter dem Bugspoiler befindet. 
Eine Individuelle Anpassung der Schwellwerte des Sensor ist im WEB Interface möglich.


Als Ölpumpe benutzte ich den Nachbau einer Webasto Dosierpumpe die 50 Pumpimpulse / ml benötigt. Es kann auch eine andere Dosierpumpe verwendet werden, die Parameter können leicht angepasst werden.

Damit ich den Kettenöler auch bedienen kann habe ich mich für den ESP 8266 Wemos entschieden. Der hat eine höhere Taktfrequenz als ein Arduino und ein WIFI Modul ist auch schon on Board. Somit habe ich für die Bedienung ein Web Interface gebaut und kann mit jedem WIFI fähigen Gerät sämtliche Einstellungen am Kettenöler vornehmen und Informationen abrufen die nicht auf dem OLED angezeigt werden. Dadurch bin ich nicht an Android oder Apple gebunden und benötige auch keine Apps dafür.

Das WIFI wird nach einer vorgegebenen Zeit abgeschaltet. Danach ist das WEB Interface bis zum nächsten Einschalten für niemanden mehr erreichbar.

Da der Wemos keinen eingebauten EEPROM besitzt kam auch ein externer EEPROM für die Speicherung aller Einstellungen und Werte zum Einsatz.

Um das Öl auf die Kette zu bekommen und ich so wenig wie möglich vom Kettenöler sehen wollte bin ich an das vordere Kettenrad gegangen und habe dafür einen Dual Injektor gebaut.

Für den Fall das der GPS Empfänger ausfällt habe ich einen Notbetrieb eingebaut der nach dem Einschalten nach einer eingestellten Zeit automatisch Aktiviert wird und eine vorgegebene Geschwindigkeit annimmt bis wieder Satellitenempfang vorhanden ist. 

Sollte der Satellitenempfang unterbrochen werden wie z.B. durch einen Tunnel, wird die letzte Geschwindigkeit so lange beibehalten bis wieder Satellitenempfang vorhanden ist.

Da der GPS-Kettenöler über Zündung (Klemme15) mit Spannung versorgt wird, kann auch keine Fehlfunktion eine Entladung der Batterie verursachen.

Ich habe zwei Startbildschirme eingebaut.
In 1. Startbildschirm wird mir die SSID des Kettenöler und die IP Adresse angezeigt (nachdem ich sie mal vergessen hatte ;-) ).
Im 2. die Version des Kettenöler und die Firmware Version. Die Anzeigezeit lässt sich im Web Interface anpassen.
