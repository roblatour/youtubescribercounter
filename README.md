# Youtube Scriber Counter

YouTube Subscriber Counter is an open source project that lets you monitor your YouTube subscription and/or view counts on an ESP32 LilyGo T-Display-S3 AMOLED.

![YouTube Subscriber Tracker on a LilyGo T-Display S3 AMOLED](/images/coverphoto.jpg)

For a short video showing the device in action, please click [here](https://www.youtube.com/watch?v=YdbTDYZ2Ues) 

## Here's what's needed to put the project together:

- a YouTube channel,
- a [Google Cloud Developer account](https://console.cloud.google.com/apis/dashboard),
- the [Arduino IDE](https://www.arduino.cc/en/software),
- the YouTubeSubscriberCounter source code from this Github repo,
- a [Lilygo T-Display-S3 AMOLED](https://s.click.aliexpress.com/e/_DdTBFBd), and
- optionally a [3D printable case](https://www.printables.com/model/566325-adjustable-case-stand-for-a-lilygo-t-display-s3-am) or [a case from LilyGo](https://www.lilygo.cc/en-ca/products/t-display-s3-shell) for it.

## Here's how the project is put together:

1\. You'll need your YouTube Channel ID. To get this go to your [YouTube Advanced Settings Account page](https://www.youtube.com/account_advanced) and beside where it says 'Channel ID' click on the copy button.  Save this someplace for use in step 4 below.

2\. You'll need a Google API key to retrieve your YouTube subscriber and view counts. If you've never created a Google API key before, here's a [quick video showing how to do that](https://youtu.be/9rw4EEA8HQM) - with each step written out in the video's comment section.  Once you have your Google API key save it someplace for use in step 4 below.

3\. Download and unzip the open source software from the GitHub link above.

4\. Use the Arduino IDE to load the Arduino sketch YouTubeSubscriberCounter.ino from step 3 above .  Once loaded into the Arduino IDE, edit the secrets\_settings.h file to include your Wi-Fi credentials, YouTube channel ID and Google API.  The general\_settings.h file may also be edited to change various other settings, such as how often the program should asks for updates from Google.

5\. The project is then built using the Arduino IDE and uploaded to the ESP32.

## How the project works:

On startup the ESP32 will connect to your network, retrieve your stats and display them.  After that the program will automatically retrieve and display your current stats at the update frequency specified in the general\_settings.h file.  If your network goes down, the program will attempt to automatically reconnect to it until successful.

**Top button**: quick pressing the top button on the LilyGo T-Display-S3 AMOLED takes you through the various display window choices, long pressing (holding the top button for 5 seconds) rotates the screen 180 degrees.  These settings will be automatically saved once they have remained unchanged for more than ten seconds.

**Bottom button**: quick pressing the bottom button causes the program to immediate retrieve and display your most currently available stats.
<br>

**I hope this project will be of good use to you!** 

## Support YouTube Subscriber Counter

To help support YouTube Subscriber Counter, or to just say thanks, you're welcome to 'Buy me a coffee'<br><br>
[<img alt="buy me  a coffee" width="200px" src="https://cdn.buymeacoffee.com/buttons/v2/default-blue.png" />](https://www.buymeacoffee.com/roblatour)
* * *
Copyright © 2024 Rob Latour
* * *   
