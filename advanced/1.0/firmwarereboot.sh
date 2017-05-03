sleep 3

sudo echo "$(date +'%T'):shell start" >> /tmp/reboot.txt

sudo ps -ef | grep remote_monitoring >> /tmp/reboot.txt

sudo echo "$(date +'%T'):remote_monitoring reboot start" >> /tmp/reboot.txt

sudo chmod +x /home/pi/cmake/remote_monitoring/remote_monitoring 

sudo /home/pi/cmake/remote_monitoring/remote_monitoring

sudo ps -ef | grep remote_monitoring >> /tmp/reboot.txt

sudo echo "$(date +'%T'):remote_monitoring running" >> /tmp/reboot.txt